/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// This can generate moving object detections from a video stream.
/// There are two types of output.  The first, called "connected
/// components" is the set of connected components detected on each
/// frame after foreground image detection.  The connected components
/// are computed on a per-frame basis.  The second, called "mod" are
/// moving objects detected by processing the connected components.
/// Typically, they are computed by using a small temporal window to
/// detect movers.

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>

#include <vul/vul_arg.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_sprintf.h>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_binary_opening.h>
#include <vil/algo/vil_binary_closing.h>

#include <vgui/vgui.h>
#include <vgui/vgui_image_tableau.h>
#include <vgui/vgui_easy2D_tableau.h>
#include <vgui/vgui_viewer2D_tableau.h>
#include <vgui/vgui_text_tableau.h>
#include <vgui/vgui_composite_tableau.h>
#include <vgui/vgui_grid_tableau.h>
#include <vgui/vgui_shell_tableau.h>
#include <vgui/vgui_menu.h>

#include <pipeline/sync_pipeline.h>

#include <utilities/ring_buffer_process.h>
#include <utilities/homography_reader_process.h>

#include <video/generic_frame_process.h>

#include <tracking/sg_background_model_process.h>
#include <tracking/project_to_world_process.h>
#include <tracking/connected_component_process.h>
#include <tracking/morphology_process.h>
#include <tracking/track_initializer_process.h>
#include <tracking/tracking_keys.h>

#include <tracking_gui/image_object.h>
#include <tracking_gui/track.h>
#include <tracking/image_object.h>


bool g_pause = false;
void pause_cb()
{
  g_pause = false;
}


void write_mod_header( vcl_ostream& );
void write_mod( vcl_ostream& fstr,
                vidtk::track_state_sptr const& state );

void write_conn_comp_header( vcl_ostream& mod_fstr );
void write_conn_comp( vcl_ostream& mod_fstr,
                      vidtk::timestamp const& ts,
                      vidtk::image_object const& obj );

vcl_string g_command_line;

int main( int argc, char** argv )
{
  for( int i = 0; i < argc; ++i )
  {
    g_command_line += " \"";
    g_command_line += argv[i];
    g_command_line += "\"";
  }

  vul_arg<char const*> config_file( "-c", "Config file" );
  vul_arg<bool> show_conn_comp( "--show-conn-comp", "Show the connected components overlaid on the source frame.", false );
  vul_arg<bool> show_mod( "--show-mod", "Show the MODs overlaid on the source frame.", false );
  vul_arg<vcl_string> show_fg_image_str( "--show-fg-image", "Which foreground image to display. Set to \"none\", \"raw\" (directly from fg/bg separation, \"morph\" (after morphology)", "none" );
  vul_arg<bool> show_bg_image( "--show-bg-image", "Show the background image", false );
  vul_arg<vcl_string> output_pipeline( "--output-pipeline", "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( "--output-config", "Dump the configuration to this file", "" );
  vul_arg<vcl_string> output_mod( "--output-mod", "Output the MODs to a file.", "" );
  vul_arg<vcl_string> output_conn_comp( "--output-conn-comp", "Output the connected components to a file.", "" );

  vul_arg_parse( argc, argv );

  vgui::init( argc, argv );

  int show_fg_image; // none
  if( show_fg_image_str() == "none" )
  {
    show_fg_image = 0;
  }
  else if( show_fg_image_str() == "raw" )
  {
    show_fg_image = 1;
  }
  else if( show_fg_image_str() == "morph" )
  {
    show_fg_image = 2;
  }
  else
  {
    vcl_cerr << "\"" << show_fg_image_str() << " is an invalid argument for --show_fg_image\n";
    return EXIT_FAILURE;
  }

  vidtk::sync_pipeline p;

  vidtk::generic_frame_process<vxl_byte> frame_src( "src" );
  p.add( &frame_src );

  vidtk::homography_reader_process homog_reader( "homog" );
  p.add( &homog_reader );

  vidtk::sg_background_model_process<vxl_byte> mod( "mod" );
  p.add( &mod );
  p.connect( frame_src.image_port(), mod.set_source_image_port() );
  p.connect( homog_reader.image_to_world_homography_port(), mod.set_homography_port() );

  vidtk::morphology_process morph( "morph" );
  p.add( &morph );
  p.connect( mod.fg_image_port(), morph.set_source_image_port() );

  vidtk::connected_component_process conn_comp( "conn_comp" );
  p.add( &conn_comp );
  p.connect( morph.image_port(), conn_comp.set_fg_image_port() );

  vidtk::project_to_world_process project( "project" );
  p.add( &project );
  p.connect( conn_comp.objects_port(), project.set_source_objects_port() );

  vidtk::ring_buffer_process< vcl_vector<vidtk::image_object_sptr> > mod_buffer( "mod_buffer" );
  p.add( &mod_buffer );
  p.connect( project.objects_port(), mod_buffer.set_next_datum_port() );

  vidtk::ring_buffer_process< vidtk::timestamp > timestamp_buffer( "timestamp_buffer" );
  p.add( &timestamp_buffer );
  p.connect( frame_src.timestamp_port(), timestamp_buffer.set_next_datum_port() );

  vidtk::track_initializer_process track_init( "track_init" );
  p.add( &track_init );
  track_init.set_mod_buffer( mod_buffer );
  track_init.set_timestamp_buffer( timestamp_buffer );
  p.add_execution_dependency( &mod_buffer, &track_init );
  p.add_execution_dependency( &timestamp_buffer, &track_init );

  vidtk::config_block config = p.params();

  if( config_file.set() )
  {
    config.parse( config_file() );
  }

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  if( output_config.set() )
  {
    vcl_ofstream fout( output_config().c_str() );
    if( ! fout )
    {
      vcl_cerr << "Couldn't open " << output_config() << " for writing\n";
      return EXIT_FAILURE;
    }
    config.print( fout );
  }

  if( output_pipeline.set() )
  {
    p.output_graphviz_file( output_pipeline().c_str() );
  }

  if( ! p.set_params( config ) )
  {
    vcl_cerr << "Failed to set pipeline parameters\n";
    return EXIT_FAILURE;
  }


  if( ! p.initialize() )
  {
    vcl_cout << "Failed to initialize pipeline\n";
    return EXIT_FAILURE;
  }

  vgui_image_tableau_new img_tab;
  vgui_easy2D_tableau_new easy_tab( img_tab );
  vgui_text_tableau_new text_tab;
  vgui_composite_tableau_new comp_tab( easy_tab, text_tab );
  vgui_viewer2D_tableau_new viewer_tab( comp_tab );

  vgui_image_tableau_new img_tab2;
  vgui_easy2D_tableau_new easy_tab2( img_tab2 );
  vgui_viewer2D_tableau_new viewer_tab2( easy_tab2 );

  vgui_image_tableau_new img_tab3;
  vgui_easy2D_tableau_new easy_tab3( img_tab3 );
  vgui_viewer2D_tableau_new viewer_tab3( easy_tab3 );

  vgui_grid_tableau_new grid_tab;
  int num_imgs = 0;
  if( show_conn_comp() || show_mod() )
  {
    //grid_tab.add_column();
    grid_tab->add_next( viewer_tab );
    ++num_imgs;
  }
  if( show_fg_image > 0 )
  {
    grid_tab->add_next( viewer_tab2 );
    ++num_imgs;
  }
  if( show_bg_image() )
  {
    grid_tab->add_next( viewer_tab3 );
    ++num_imgs;
  }

  vgui_shell_tableau_new shell_tab( grid_tab );

  vgui_menu main_menu;
  vgui_menu pause_menu;
  pause_menu.add( "Pause", &pause_cb, vgui_key(' ') );
  main_menu.add( "Pause", pause_menu );

  // Only create a window and such if there are some images to show.
  if( num_imgs > 0 )
  {
    vgui::adapt( shell_tab, frame_src.ni()*num_imgs, frame_src.nj(), main_menu );
  }

  vidtk::vgui::draw_image_object obj_drawer( easy_tab );
  obj_drawer.text_ = text_tab;
  obj_drawer.show_area_ = true;

  vidtk::vgui::draw_track track_drawer( easy_tab );

  vcl_ofstream mod_fstr;
  if( output_mod.set() )
  {
    mod_fstr.open( output_mod().c_str() );
    if( ! mod_fstr )
    {
      vcl_cerr << "Couldn't open " << output_mod() << " for writing\n";
      return EXIT_FAILURE;
    }
    write_mod_header( mod_fstr );
  }

  vcl_ofstream conn_comp_fstr;
  if( output_conn_comp.set() )
  {
    conn_comp_fstr.open( output_conn_comp().c_str() );
    if( ! conn_comp_fstr )
    {
      vcl_cerr << "Couldn't open " << output_conn_comp() << " for writing\n";
      return EXIT_FAILURE;
    }
    write_conn_comp_header( conn_comp_fstr );
  }


  while( (num_imgs == 0 || ! vgui::quit_was_called() )
         && p.execute() )
  {
    vcl_cout << "Done processing time = " << frame_src.timestamp().time()
             << " (frame="<< frame_src.timestamp().frame_number() << ")\n";

    if( show_conn_comp() || show_mod() )
    {
      img_tab->set_image_view( frame_src.image() );
      img_tab->reread_image();
      img_tab->post_redraw();

      easy_tab->clear();
      text_tab->clear();
      if( show_conn_comp() )
      {
        easy_tab->set_point_radius(5);
        easy_tab->set_line_width( 1 );
        easy_tab->set_foreground(1,0,0);
        vcl_vector< vidtk::image_object_sptr > const& objs = conn_comp.objects();
        for( unsigned i = 0; i < objs.size(); ++i )
        {
          obj_drawer.draw( *objs[i] );
        }
      }

      if( show_mod() )
      {
        easy_tab->set_point_radius(5);
        easy_tab->set_line_width( 1 );
        easy_tab->set_foreground(0,1,0);
        vcl_vector< vidtk::track_sptr > const& trks = track_init.new_tracks();
        for( unsigned i = 0; i < trks.size(); ++i )
        {
          track_drawer.draw( *trks[i] );
        }
      }

      easy_tab->post_redraw();
    }

    if( show_fg_image == 1 )
    {
      img_tab2->set_image_view( mod.fg_image() );
      img_tab2->reread_image();
      img_tab2->post_redraw();
    }
    else if( show_fg_image == 2 )
    {
      img_tab2->set_image_view( morph.image() );
      img_tab2->reread_image();
      img_tab2->post_redraw();
    }

    if( show_bg_image() )
    {
      img_tab3->set_image_view( mod.bg_image() );
      img_tab3->reread_image();
      img_tab3->post_redraw();
    }


    if( output_mod.set() )
    {
      vcl_vector< vidtk::track_sptr > const& trks = track_init.new_tracks();
      for( unsigned i = 0; i < trks.size(); ++i )
      {
        write_mod( mod_fstr, trks[i]->last_state() );
      }
    }

    if( output_conn_comp.set() )
    {
      vcl_vector< vidtk::image_object_sptr > const& objs = conn_comp.objects();
      for( unsigned i = 0; i < objs.size(); ++i )
      {
        write_conn_comp( conn_comp_fstr, frame_src.timestamp(), *objs[i] );
      }
    }

    // if( save_mask() ) {
    //   vil_image_view<vxl_byte> bi;
    //   vil_convert_stretch_range( foreground_mask, bi );
    //   vil_save( bi, vul_sprintf( "fg%06d.png", cnt ).c_str() );
    //   vcl_cout << "wrote frame " << cnt << vcl_endl;
    // }

    // if( save_bg() ) {
    //   vil_save( bg_image, vul_sprintf( "bg%06d.png", cnt ).c_str() );
    //   vcl_cout << "wrote bg frame " << cnt << vcl_endl;
    // }

    // {
    //   vcl_vector< vidtk::image_object_sptr > const& objs = project.objects();
    //   for( unsigned i = 0; i < objs.size(); ++i )
    //   {
    //     mod_fstr << frame_src.frame_number() << "\t\t"
    //              << objs[i]->img_loc_ << "\t\t"
    //              << objs[i]->world_loc_ << "\n";
    //   }
    // }

    // while( g_pause && ! vgui::quit_was_called() ) {
    //   vgui::run_till_idle();
    // }
    // g_pause = false;
    if( num_imgs > 0 )
    {
      vgui::run_till_idle();
    }
  }

  mod_fstr.close();

  vcl_cout << "Done" << vcl_endl;
  if( num_imgs > 0 )
  {
    if( vgui::quit_was_called() )
    {
      return EXIT_SUCCESS;
    }
    else
    {
      return vgui::run();
    }
  }
  else
  {
    return EXIT_SUCCESS;
  }
}


void
write_mod_header( vcl_ostream& mod_fstr )
{
  mod_fstr << "# Cmd: " << g_command_line << "\n";
  mod_fstr << "# 1:Frame-number  2:Time  3:Time-for-vel  4-6:World-loc(x,y,z)  7-9:World-vel(x,y,z)  10-11:Image-loc(i,j)  12:Area   13-16:Img-bbox(i0,j0,i1,j1)\n";
}

void
write_mod( vcl_ostream& mod_fstr,
           vidtk::track_state_sptr const& state )
{
  mod_fstr << state->time_.frame_number() << "    "
           << state->time_.time() << "    "
           << state->time_.time_in_secs() << "    "
           << state->loc_ << "    "
           << state->vel_ << "    ";

  vcl_vector< vidtk::image_object_sptr > mods;
  if( ! state->data_.get( vidtk::tracking_keys::img_objs, mods ) ||
      mods.empty() )
  {
    vcl_cerr << "Failed to obtain MOD data on frame " << state->time_.frame_number() << "\n";
  }
  else
  {
    vidtk::image_object const& obj = *mods[0];
    mod_fstr << obj.img_loc_ << "    "
             << obj.area_ << "    "
             << obj.bbox_.min_x() << " " << obj.bbox_.min_y() << " "
             << obj.bbox_.max_x() << " " << obj.bbox_.max_y();
  }
  mod_fstr << "\n";
}


void
write_conn_comp_header( vcl_ostream& mod_fstr )
{
  mod_fstr << "# Cmd: " << g_command_line << "\n";
  mod_fstr << "# 1:Frame-number  2:Time  3-5:World-loc(x,y,z)  6-7:Image-loc(i,j)  8:Area   9-12:Img-bbox(i0,j0,i1,j1)\n";
}

void
write_conn_comp( vcl_ostream& mod_fstr,
                 vidtk::timestamp const& ts,
                 vidtk::image_object const& obj )
{
  mod_fstr << ts.frame_number() << "    "
           << ts.time() << "    "
           << obj.world_loc_ << "    "
           << obj.img_loc_ << "    "
           << obj.area_ << "    "
           << obj.bbox_.min_x() << " " << obj.bbox_.min_y() << " "
           << obj.bbox_.max_x() << " " << obj.bbox_.max_y()
           << "\n";
}
