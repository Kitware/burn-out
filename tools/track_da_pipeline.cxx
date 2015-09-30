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
#include <vgui/vgui_utils.h>

#include <pipeline/sync_pipeline.h>
#include <pipeline/sync_pipeline_node.h>

#include <utilities/ring_buffer_process.h>
#include <utilities/timestamp_reader_process.h>

#include <video/generic_frame_process.h>
#include <video/image_list_writer_process.h>

#include <tracking/sg_background_model_process.h>
#include <tracking/project_to_world_process.h>
#include <tracking/connected_component_process.h>
#include <tracking/morphology_process.h>
#include <tracking/track_initializer_process.h>
#include <tracking/tracking_keys.h>
#include <tracking/da_tracker_process.h>
#include <tracking/filter_image_objects_process.h>
#include <tracking/filter_tracks_process.h>
#include <tracking/image_object.h>
#include <tracking/image_object_writer_process.h>
#include <tracking/da_so_tracker_generator_process.h>

#include <tracking/image_difference_process.h>

#include <tracking_gui/image_object.h>
#include <tracking_gui/track.h>


bool g_pause = false;
bool g_step = false;

void step_once()
{
  g_step = true;
}

void pause_cb()
{
  g_pause = ! g_pause;
}


vcl_ofstream g_trk_str;

void
write_out_tracks( vcl_vector< vidtk::track_sptr > const& trks )
{
  if( ! g_trk_str )
  {
    return;
  }

  unsigned N = trks.size();
  for( unsigned i = 0; i < N; ++i )
  {
    vcl_vector< vidtk::track_state_sptr > const& hist = trks[i]->history();
    unsigned M = hist.size();
    for( unsigned j = 0; j < M; ++j )
    {
      g_trk_str << trks[i]->id() << " ";
      g_trk_str << M << " ";
      g_trk_str << hist[j]->time_.frame_number() << " ";
      g_trk_str << hist[j]->loc_(0) << " ";
      g_trk_str << hist[j]->loc_(1) << " ";
      g_trk_str << hist[j]->vel_(0) << " ";
      g_trk_str << hist[j]->vel_(1) << " ";
      vcl_vector<vidtk::image_object_sptr> objs;
      if( ! hist[j]->data_.get( vidtk::tracking_keys::img_objs, objs ) || objs.empty() )
      {
        log_error( "no MOD for track " << trks[i]->id() << ", state " << j << "\n" );
        continue;
      }
      vidtk::image_object const& o = *objs[0];
      g_trk_str << o.img_loc_ << " ";
      g_trk_str << o.bbox_.min_x() << " ";
      g_trk_str << o.bbox_.min_y() << " ";
      g_trk_str << o.bbox_.max_x() << " ";
      g_trk_str << o.bbox_.max_y() << " ";
      g_trk_str << o.area_ << " ";
      g_trk_str << o.world_loc_ << " ";
      g_trk_str << vul_sprintf("%.3f",hist[j]->time_.time_in_secs()) << " ";
      g_trk_str << "\n";
    }
  }
  g_trk_str.flush();
}


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
  vul_arg<bool> show_world_conn_comp( "--show-world-conn-comp", "Show the connected components in world coordinates.", false );
  vul_arg<bool> show_filtered_conn_comp( "--show-filtered-conn-comp", "Show the connected components after filtering.", false );
  vul_arg<bool> show_world_filtered_conn_comp( "--show-world-filtered-conn-comp", "Show the filtered connected components in world coordinates.", false );
  vul_arg<bool> show_mod( "--show-mod", "Show the MODs overlaid on the source frame.", false );
  vul_arg<bool> show_world_mod( "--show-world-mod", "Show the MODs in world coordinates.", false );
  vul_arg<vcl_string> show_fg_image_str( "--show-fg-image", "Which foreground image to display. Set to \"none\", \"raw\" (directly from fg/bg separation, \"morph\" (after morphology)", "none" );
  vul_arg<bool> show_tracks( "--show-tracks", "Show the tracks overlaid on the source frame.", false );
  vul_arg<bool> show_world_tracks( "--show-world-tracks", "Show the tracks in world coordinates.", false );
  vul_arg<bool> show_bg_image( "--show-bg-image", "Show the background image", false );
  vul_arg<vcl_string> output_pipeline( "--output-pipeline", "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( "--output-config", "Dump the configuration to this file", "" );
  vul_arg<vcl_string> output_tracks( "--output-tracks", "Output the tracks to a file.", "" );
  vul_arg<vcl_string> output_mod_filename( "--output-mods", "Output the MODs to a file.", "" );

  vul_arg<vcl_string> fg_process_str( "--fg_process", "Which fg process? [gmm, diff]", "gmm");
  vul_arg<unsigned int> pause_at( "--pause-at", "run until frame N, then pause" );

  vul_arg<bool> persist_gui_at_end( "--persist-gui-at-end", "Keep the GUI running after processing" );
  vul_arg<bool> no_show_area( "--no-show-area", "Don't show the area of connected components", false );

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
    vcl_cerr << "\"" << show_fg_image_str() << "\" is an invalid argument for --show_fg_image\n";
    return EXIT_FAILURE;
  }

  bool show_source_image =
    show_conn_comp() ||
    show_filtered_conn_comp() ||
    show_mod() ||
    show_tracks();
  bool show_world =
    show_world_conn_comp() ||
    show_world_filtered_conn_comp() ||
    show_world_mod() ||
    show_world_tracks();

  vidtk::sync_pipeline p;

  vidtk::fg_image_process<vxl_byte>* fg_process = 0;
  if ( fg_process_str() == "gmm" )
  {
    fg_process = new vidtk::sg_background_model_process<vxl_byte>( "gmm_mod");
  } 
  else if (fg_process_str() == "diff" )
  {
    fg_process = new vidtk::image_difference_process<vxl_byte>( "image_diff" );
  } 
  else {
    vcl_cerr << "\"" << fg_process_str() << "\" is not a valid choice for --fg_process\n";
    return EXIT_FAILURE;
  }

  vidtk::generic_frame_process<vxl_byte> frame_src( "src" );
  p.add( &frame_src );

  vidtk::timestamp_reader_process<vxl_byte> timestamper( "timestamper" );
  p.add( &timestamper );
  p.connect( frame_src.image_port(), timestamper.set_source_image_port() );
  p.connect( frame_src.timestamp_port(), timestamper.set_source_timestamp_port() );

  p.add( fg_process );
  p.connect( timestamper.image_port(), fg_process->set_source_image_port() );

  vidtk::morphology_process morph1( "morph1" );
  p.add( &morph1 );
  p.connect( fg_process->fg_image_port(), morph1.set_source_image_port() );

  vidtk::morphology_process morph( "morph" );
  p.add( &morph );
  p.connect( morph1.image_port(), morph.set_source_image_port() );

  vidtk::image_list_writer_process<bool> morph_image_writer( "fg_image_writer" );
  p.add( &morph_image_writer );
  p.connect( morph.image_port(), morph_image_writer.set_image_port() );
  p.connect( timestamper.timestamp_port(), morph_image_writer.set_timestamp_port() );

  vidtk::connected_component_process conn_comp( "conn_comp" );
  p.add( &conn_comp );
  p.connect( morph.image_port(), conn_comp.set_fg_image_port() );

  vidtk::project_to_world_process project( "project" );
  p.add( &project );
  p.connect( conn_comp.objects_port(), project.set_source_objects_port() );

  vidtk::filter_image_objects_process< vxl_byte > filter1( "filter1" );
  p.add( &filter1 );
  p.connect( project.objects_port(), filter1.set_source_objects_port() );

  vidtk::image_object_writer_process filter1_writer( "filter1_writer" );
  p.add( &filter1_writer );
  p.connect( filter1.objects_port(), filter1_writer.set_image_objects_port() );
  p.connect( timestamper.timestamp_port(), filter1_writer.set_timestamp_port() );

  vidtk::track_initializer_process track_init( "track_init" );
  p.add( &track_init );
  // the connections to this node are set below.

  vidtk::filter_tracks_process trk_filter1( "trk_filter1" );
  p.add( &trk_filter1 );
  // the connections to this node are set below.

  vidtk::da_so_tracker_generator_process tracker_initializer("tracker_initializer");
  p.connect_without_dependency(trk_filter1.tracks_port(), tracker_initializer.set_new_tracks_port());

  vidtk::da_tracker_process tracker( "tracker" );
  p.add( &tracker );
  p.connect( filter1.objects_port(), tracker.set_objects_port() );
  p.connect( timestamper.timestamp_port(), tracker.set_timestamp_port() );
  p.connect_without_dependency( tracker_initializer.new_trackers_port(), tracker.set_new_trackers_port() );

  vidtk::filter_tracks_process trk_filter2( "trk_filter2" );
  p.add( &trk_filter2 );
  p.connect( tracker.terminated_tracks_port(), trk_filter2.set_source_tracks_port() );

  vidtk::ring_buffer_process< vcl_vector<vidtk::image_object_sptr> > mod_buffer( "init_mod_buffer" );
  p.add( &mod_buffer );
  p.connect( tracker.unassigned_objects_port(), mod_buffer.set_next_datum_port() );

  vidtk::ring_buffer_process< vidtk::timestamp > timestamp_buffer( "init_timestamp_buffer" );
  p.add( &timestamp_buffer );
  p.connect( timestamper.timestamp_port(), timestamp_buffer.set_next_datum_port() );

  track_init.set_mod_buffer( mod_buffer );
  track_init.set_timestamp_buffer( timestamp_buffer );
  p.add_execution_dependency( &mod_buffer, &track_init );
  p.add_execution_dependency( &timestamp_buffer, &track_init );

  p.connect( track_init.new_tracks_port(), trk_filter1.set_source_tracks_port() );


  vidtk::sync_pipeline_node track_output_node;
  {
    track_output_node.set_name( "track_output" );
    p.add( track_output_node );
    p.connect( trk_filter2.tracks_port(),
               vidtk::pipeline_aid::make_input< vcl_vector< vidtk::track_sptr > const& >(
                 &track_output_node,
                 boost::bind( &write_out_tracks, _1 ),
                 "tracks" ) );
  }


  vidtk::image_list_writer_process<vxl_byte> gui_window_writer( "gui_writer" );
  p.add( &gui_window_writer );
  p.connect( timestamper.timestamp_port(), gui_window_writer.set_timestamp_port() );


  vidtk::config_block config = p.params();

  // Set some defaults for this application before we read the user options.
  config.set( "gui_writer:disabled", "true" );
  config.set( "gui_writer:skip_unset_images", "true" );
  config.set( "fg_image_writer:disabled", "true" );

  if( output_pipeline.set() )
  {
    p.output_graphviz_file( output_pipeline().c_str() );
  }

  if( config_file.set() )
  {
    config.parse( config_file() );
    config.print( vcl_cout );
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

  if( output_tracks.set() )
  {
    g_trk_str.open( output_tracks().c_str() );
    if( ! g_trk_str )
    {
      vcl_cout << "Couldn't open " << output_tracks() << " for writing\n";
      return EXIT_FAILURE;
    }
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

  vgui_image_tableau_new img_tab4;
  vgui_easy2D_tableau_new easy_tab4( img_tab4 );
  vgui_text_tableau_new text_tab4;
  vgui_composite_tableau_new comp_tab4( easy_tab4, text_tab4 );
  vgui_viewer2D_tableau_new viewer_tab4( comp_tab4 );

  vgui_grid_tableau_new grid_tab;
  int num_imgs = 0;
  if( show_source_image )
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
  if( show_world )
  {
    grid_tab->add_next( viewer_tab4 );
    ++num_imgs;
  }

  vgui_shell_tableau_new shell_tab( grid_tab );

  vgui_menu main_menu;
  vgui_menu pause_menu;
  pause_menu.add( "Step once", &step_once, vgui_key(' ') );
  pause_menu.add( "Pause/unpause", &pause_cb, vgui_key('p') );
  main_menu.add( "Pause", pause_menu );

  // Only create a window and such if there are some images to show.
  if( num_imgs > 0 )
  {
    vgui::adapt( shell_tab, frame_src.ni()*num_imgs, frame_src.nj(), main_menu );
  }

  vidtk::vgui::draw_image_object obj_drawer( easy_tab );
  obj_drawer.text_ = text_tab;
  obj_drawer.show_area_ = ! no_show_area();

  vidtk::vgui::draw_image_object filtered_obj_drawer( easy_tab );
  filtered_obj_drawer.text_ = text_tab;
  filtered_obj_drawer.show_area_ = true;
  filtered_obj_drawer.loc_color_ = vil_rgba<float>( 1, 1, 0, 1 );
  filtered_obj_drawer.bbox_color_ = vil_rgba<float>( 1, 1, 0, 1 );
  filtered_obj_drawer.boundary_color_ = vil_rgba<float>( 1, 1, 0, 1 );

  vidtk::vgui::draw_image_object world_obj_drawer( easy_tab4 );
  world_obj_drawer.text_ = text_tab4;
  world_obj_drawer.show_area_ = true;

  vidtk::vgui::draw_image_object world_filtered_obj_drawer( easy_tab4 );
  world_filtered_obj_drawer.text_ = text_tab4;
  world_filtered_obj_drawer.show_area_ = true;
  world_filtered_obj_drawer.loc_color_ = vil_rgba<float>( 1, 1, 0, 1 );

  vidtk::vgui::draw_track mod_drawer( easy_tab );

  vidtk::vgui::draw_track world_mod_drawer( easy_tab4 );

  vidtk::vgui::draw_track track_drawer( easy_tab );
  track_drawer.text_ = text_tab;
  track_drawer.trail_color_ = vil_rgba<float>( 0, 0, 1 );
  track_drawer.current_color_ = vil_rgba<float>( 0, 0, 1 );
  track_drawer.past_color_ = vil_rgba<float>( 0.9f, 0.9f, 1 );

  vidtk::vgui::draw_track world_track_drawer( easy_tab4 );
  world_track_drawer.trail_color_ = vil_rgba<float>( 0, 0, 1 );
  world_track_drawer.current_color_ = vil_rgba<float>( 0, 0, 1 );
  world_track_drawer.past_color_ = vil_rgba<float>( 0.9f, 0.9f, 1 );


  vcl_ofstream mod_fstr( output_mod_filename().c_str() );

  while( (num_imgs == 0 || ! vgui::quit_was_called() )
         && p.execute() )
  {
	vcl_vector< vidtk::image_object_sptr > const& objs = conn_comp.objects();

	for( unsigned i = 0; i < objs.size(); ++i )
	{
		 mod_fstr << timestamper.timestamp().frame_number() << "    "
				  << timestamper.timestamp().time() << "    "
				  << objs[i]->world_loc_ << "    "
				  << objs[i]->img_loc_ << "    "
				  << objs[i]->area_ << "    "
				  << objs[i]->bbox_.min_x() << " " << objs[i]->bbox_.min_y() << " "
				  << objs[i]->bbox_.max_x() << " " << objs[i]->bbox_.max_y()
				  << "\n";
	}

    {
      vidtk::timestamp const& t = timestamper.timestamp();
      vcl_cout << "Done processing time = " << vul_sprintf("%.3f",t.has_time()?t.time()/1e6:-1.0)
               << " (frame="<< (t.has_frame_number()?t.frame_number():-1) << ")\n";
    }
    // vcl_cout << "Done processing time A = " << frame_src.timestamp().time()
    //          << " (frame="<< frame_src.timestamp().frame_number() << ")\n";
    // vcl_cout << "Done processing time = " << timestamper.timestamp().time()
    //          << " (frame="<< timestamper.timestamp().frame_number() << ")\n";

    if( show_source_image )
    {
      img_tab->set_image_view( timestamper.image() );
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

      if( show_filtered_conn_comp() )
      {
        easy_tab->set_point_radius(5);
        easy_tab->set_line_width( 1 );
        easy_tab->set_foreground(1,1,0);
        vcl_vector< vidtk::image_object_sptr > const& objs = filter1.objects();
        for( unsigned i = 0; i < objs.size(); ++i )
        {
          filtered_obj_drawer.draw( *objs[i] );
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
          mod_drawer.draw( *trks[i] );
        }
      }

      if( show_tracks() )
      {
        easy_tab->set_point_radius(5);
        easy_tab->set_line_width( 1 );
        easy_tab->set_foreground(0,0,1);
        vcl_vector< vidtk::track_sptr > const& trks = tracker.active_tracks();
        for( unsigned i = 0; i < trks.size(); ++i )
        {
          track_drawer.draw( *trks[i] );
        }
      }

      easy_tab->post_redraw();
    }

    if( show_fg_image == 1 )
    {
      img_tab2->set_image_view( fg_process->fg_image() );
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
      img_tab3->set_image_view( fg_process->bg_image() );
      img_tab3->reread_image();
      img_tab3->post_redraw();
    }


    if( show_world )
    {
      easy_tab4->clear();
      text_tab4->clear();

      if( show_world_conn_comp() )
      {
        easy_tab4->set_point_radius(5);
        easy_tab4->set_line_width( 1 );
        easy_tab4->set_foreground(1,0,0);
        text_tab4->set_colour(1,0,0);
        vcl_vector< vidtk::image_object_sptr > const& objs = conn_comp.objects();
        for( unsigned i = 0; i < objs.size(); ++i )
        {
          world_obj_drawer.draw_world2d( *objs[i] );
        }
      }

      if( show_world_filtered_conn_comp() )
      {
        easy_tab4->set_point_radius(5);
        easy_tab4->set_line_width( 1 );
        easy_tab4->set_foreground(1,0,0);
        text_tab4->set_colour(1,1,0);
        vcl_vector< vidtk::image_object_sptr > const& objs = filter1.objects();
        for( unsigned i = 0; i < objs.size(); ++i )
        {
          world_filtered_obj_drawer.draw_world2d( *objs[i] );
        }
      }

      if( show_world_mod() )
      {
        easy_tab4->set_point_radius(5);
        easy_tab4->set_line_width( 1 );
        easy_tab4->set_foreground(0,1,0);
        vcl_vector< vidtk::track_sptr > const& trks = track_init.new_tracks();
        for( unsigned i = 0; i < trks.size(); ++i )
        {
          world_mod_drawer.draw_world2d( *trks[i] );
        }
      }

      if( show_world_tracks() )
      {
        easy_tab4->set_point_radius(5);
        easy_tab4->set_line_width( 1 );
        easy_tab4->set_foreground(0,0,1);
        vcl_vector< vidtk::track_sptr > const& trks = tracker.active_tracks();
        for( unsigned i = 0; i < trks.size(); ++i )
        {
          world_track_drawer.draw_world2d( *trks[i] );
        }

        vcl_vector< vidtk::da_so_tracker_sptr > const& trkers = tracker.active_trackers();
        for( unsigned i = 0; i < trkers.size(); ++i )
        {
          vnl_double_2 loc;
          vnl_double_2x2 cov;
          if( trkers[i]->last_update_time() == timestamper.timestamp().time_in_secs() )
          {
            loc = trkers[i]->get_current_location();
            cov = trkers[i]->get_current_location_covar();
            world_track_drawer.trail_color_ = vil_rgba<float>( 0, 0, 1 );
            text_tab4->set_colour(0,0,1);
          }
          else
          {
            trkers[i]->predict( timestamper.timestamp().time_in_secs(), loc, cov );
            world_track_drawer.trail_color_ = vil_rgba<float>( .7f, .7f, 1 );
            text_tab4->set_colour(0.7f,0.7f,1);
          }
          world_track_drawer.draw_gate( loc, cov, 3 );
          text_tab4->add( loc[0], loc[1], vul_sprintf("%d(%d)",trks[i]->id(),i) );
        }
      }
    }

    if ((pause_at.set() && (timestamper.timestamp().frame_number() == pause_at())))
    {
      g_pause = true;
    }
       

    if( num_imgs > 0 )
    {
      vgui::run_till_idle();
      // don't grab the GUI unless we need to
      if( ! config.get<bool>( "gui_writer:disabled" ) )
      {
        vil_image_view<vxl_byte> img = vgui_utils::colour_buffer_to_view();
        gui_window_writer.set_image( img );
        gui_window_writer.step();
      }
      while( g_pause && ! g_step && ! vgui::quit_was_called() )
      {
        vgui::run_till_idle();
      }
      g_step = false;
    }

  }

  vcl_cout << "Done" << vcl_endl;
  if( num_imgs > 0 )
  {
    if( vgui::quit_was_called() ||
        ! persist_gui_at_end() )
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
