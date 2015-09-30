/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// This will track KLT features on a video, and compute homographies
/// based on those tracks.  It can also
/// - write out these tracks to a file.
/// - write out the homographies to a file.
///
/// In the GUI version, it can display the tracks on screen, and show
/// a preview of the warped images.

#include <vcl_iostream.h>
#include <vcl_fstream.h>
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

#include <pipeline/sync_pipeline.h>
#include <pipeline/sync_pipeline_node.h>

#include <utilities/ring_buffer_process.h>
#include <utilities/timestamp_reader_process.h>

#include <kwklt/klt_pyramid_process.h>
#include <kwklt/klt_tracking_process.h>
#include <kwklt/klt_track.h>

#include <video/generic_frame_process.h>
#include <video/greyscale_process.h>
#include <video/warp_image.h>
#include <video/image_list_writer_process.h>

#include <tracking/homography_process.h>


#ifndef NOGUI
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

#include <tracking_gui/track.h>
#endif // NOGUI

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
vcl_ofstream g_homog_str;

void
write_out_tracks( vcl_vector< vidtk::klt_track_ptr > const& trks,
                  vidtk::homography_process const* h )
{
  if( ! g_trk_str )
  {
    return;
  }

  const size_t N = trks.size();
  int id = 0;
  for( unsigned i = 0; i < N; ++i )
  {
    vidtk::klt_track_ptr cur_point = trks[i];
    while ( cur_point )
    {
      g_trk_str << id << " ";
      g_trk_str << trks[i]->size() << " ";
      g_trk_str << -1 << " ";
      g_trk_str << cur_point->point().x << " ";
      g_trk_str << cur_point->point().y << " ";
      g_trk_str << 0 << " ";
      g_trk_str << 0 << " ";
      g_trk_str << h->is_good( trks[i] ) << " ";
      g_trk_str << "\n";

      cur_point = cur_point->tail();
    }
  }
  g_trk_str.flush();
}


void
write_out_homog( vgl_h_matrix_2d<double> const& H )
{
  if( ! g_homog_str )
  {
    return;
  }

  g_homog_str.precision( 20 );
  g_homog_str << "\n" << H << "\n";
  g_homog_str.flush();
}





void
compute_output_size( unsigned& ni,
                     unsigned& nj,
                     int& i0,
                     int& j0,
                     vgl_h_matrix_2d<double> const& homog )
{
  double minx = 0;
  double maxx = ni;
  double miny = 0;
  double maxy = nj;

  vcl_vector< vgl_homg_point_2d<double> > corners;
  corners.push_back( vgl_homg_point_2d<double>( minx, miny ) );
  corners.push_back( vgl_homg_point_2d<double>( maxx, miny ) );
  corners.push_back( vgl_homg_point_2d<double>( minx, maxy ) );
  corners.push_back( vgl_homg_point_2d<double>( maxx, maxy ) );

  vgl_box_2d<double> box;
  for( unsigned j = 0; j < corners.size(); ++j )
  {
    vgl_point_2d<double> p = homog * corners[j];
    box.add( p );
  }

  i0 = int( vcl_floor( box.min_x() ) );
  j0 = int( vcl_floor( box.min_y() ) );
  ni = unsigned( vcl_ceil( box.width() ) );
  nj = unsigned( vcl_ceil( box.height() ) );

  // Make ni and nj a multiple of 16, to make subsequent video computation easier
  ni = ((ni+15)/16)*16;
  nj = ((nj+15)/16)*16;

  vcl_cout << "Output image is " << ni << "x" << nj;
  if( i0 >= 0 ) vcl_cout << "+";
  vcl_cout << i0;
  if( j0 >= 0 ) vcl_cout << "+";
  vcl_cout << j0 << "\n";
}

void load_and_set_filter_mask(
  vidtk::homography_process& homog,
  const vcl_string& mask_filename,
  bool dark_means_remove )
{
  vil_image_view<vxl_byte> img = vil_load( mask_filename.c_str() );
  if ( ! img )
  {
    vcl_cerr << "Couldn't load mask image '" << mask_filename << "'; continuing\n";
    return;
  }
  vil_image_view<bool> mask_img;
  unsigned remove_count = 0;
  mask_img.set_size( img.ni(), img.nj(), 1 );
  for (unsigned i=0; i<img.ni(); ++i)
  {
    for (unsigned j=0; j<img.nj(); ++j)
    {
      // true values identify areas to be REMOVED from consideration
      bool input_is_dark = ( img(i,j) < 127 );
      bool remove_me = ( dark_means_remove ) ? input_is_dark : !input_is_dark;
      mask_img(i, j) = remove_me;
      if (remove_me) remove_count++;
    }
  }
  double removed_pcent = 100.0*remove_count / (img.ni() * img.nj());
  vcl_cerr << "Mask will remove " << removed_pcent << "% of image from consideration\n";

  homog.set_mask_image( mask_img );
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
  //vul_arg<bool> show_tracks( "--show-tracks", "Show the tracks overlaid on the source frame.", true );
  vul_arg<vcl_string> output_pipeline( "--output-pipeline", "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( "--output-config", "Dump the configuration to this file", "" );
  vul_arg<vcl_string> output_tracks( "--output-tracks", "Output the tracks to a file.", "" );
  vul_arg<vcl_string> output_homog( "--output-homog", "Output the homographies to a file.", "" );

  vul_arg<bool> show_stab( "--show-stab", "Show a preview of the stabilized frames", false );
  vul_arg<bool> show_trails( "--show-trails", "Show a trail of the track", false );
  vul_arg<bool> show_errors( "--show-errors", "Show the error of the pt vs transformed pt", false );

  vul_arg<vcl_string> mask_filename_arg( "--mask", "Filename of mask image for filtering KLT features" );
  vul_arg<vcl_string> mask_sense_arg( "--mask-sense",
                                      "(bright,dark): filter removes (bright, dark) areas ",
                                      "dark" );
  vul_arg<bool> persist_gui_at_end( "--persist-gui-at-end", "Keep the GUI running after processing" );

  // FIXME: implement show trails in world coordinates, by buffering the homographies.

  vul_arg_parse( argc, argv );

#ifndef NOGUI
  vgui::init( argc, argv );
#endif

  // compute the homographies if we need to
  bool const compute_homographies = output_homog.set() || show_stab();



  vidtk::sync_pipeline p;

  vidtk::generic_frame_process<vxl_byte> frame_src( "src" );
  p.add( &frame_src );

  vidtk::timestamp_reader_process<vxl_byte> timestamper( "timestamper" );
  p.add( &timestamper );
  p.connect( frame_src.image_port(), timestamper.set_source_image_port() );
  p.connect( frame_src.timestamp_port(), timestamper.set_source_timestamp_port() );

  vidtk::greyscale_process<vxl_byte> grey( "grey" );
  p.add( &grey );
  p.connect( timestamper.image_port(), grey.set_image_port() );

  vidtk::klt_pyramid_process pyramid( "pyramid" );
  p.add( &pyramid );
  p.connect( grey.image_port(), pyramid.set_image_port() );

  vidtk::klt_tracking_process tracking( "tracking" );
  p.add( &tracking );
  p.connect( timestamper.timestamp_port(), tracking.set_timestamp_port() );
  p.connect( pyramid.image_pyramid_port(), tracking.set_image_pyramid_port() );
  p.connect( pyramid.image_pyramid_gradx_port(), tracking.set_image_pyramid_gradx_port() );
  p.connect( pyramid.image_pyramid_grady_port(), tracking.set_image_pyramid_grady_port() );

  vidtk::homography_process homog( "homog" );
  if( compute_homographies )
  {
    p.add( &homog );
    p.connect( tracking.created_tracks_port(), homog.set_new_tracks_port() );
    p.connect( tracking.active_tracks_port(), homog.set_updated_tracks_port() );
  }

  if ( mask_filename_arg.set() )
  {
    if ( ! ( (mask_sense_arg() == "bright") || (mask_sense_arg() == "dark")))
    {
      vcl_cerr << "mask_sense must be one of 'bright' or 'dark'; exiting\n";
      return EXIT_FAILURE;
    }
    load_and_set_filter_mask( homog, mask_filename_arg(), (mask_sense_arg() == "dark") );
  }

  vidtk::sync_pipeline_node track_output_node;
  {
    track_output_node.set_name( "track_output" );
    p.add( track_output_node );
    p.connect( tracking.terminated_tracks_port(),
               vidtk::pipeline_aid::make_input< vcl_vector< vidtk::klt_track_ptr > const& >(
                 &track_output_node,
                 boost::bind( &write_out_tracks, _1, &homog ),
                 "tracks" ) );
  }

  vidtk::sync_pipeline_node homog_output_node;
  if( output_homog.set() )
  {
    homog_output_node.set_name( "homog_output" );
    p.add( homog_output_node );
    p.connect( homog.image_to_world_homography_port(),
               vidtk::pipeline_aid::make_input< vgl_h_matrix_2d<double> const& >(
                 &homog_output_node,
                 boost::bind( &write_out_homog, _1 ),
                 "homographies" ) );
  }

  vidtk::image_list_writer_process<vxl_byte> gui_window_writer( "gui_writer" );
  p.add( &gui_window_writer );
  p.connect( timestamper.timestamp_port(), gui_window_writer.set_timestamp_port() );

  vidtk::config_block config = p.params();

  // Set some defaults for this application before we read the user options.
  config.set( "gui_writer:disabled", "true" );
  config.set( "gui_writer:skip_unset_images", "true" );

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

  if( output_tracks.set() )
  {
    g_trk_str.open( output_tracks().c_str() );
    if( ! g_trk_str )
    {
      vcl_cout << "Couldn't open " << output_tracks() << " for writing\n";
      return EXIT_FAILURE;
    }
  }

  if( output_homog.set() )
  {
    g_homog_str.open( output_homog().c_str() );
    if( ! g_homog_str )
    {
      vcl_cout << "Couldn't open " << output_homog() << " for writing\n";
      return EXIT_FAILURE;
    }
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

#ifndef NOGUI
  vgui_image_tableau_new img_tab;
  vgui_easy2D_tableau_new easy_tab( img_tab );
  vgui_text_tableau_new text_tab;
  vgui_composite_tableau_new comp_tab( easy_tab, text_tab );
  vgui_viewer2D_tableau_new viewer_tab( comp_tab );

  vgui_image_tableau_new img_tab2;
  vgui_viewer2D_tableau_new viewer_tab2( img_tab2 );

  vgui_grid_tableau_new grid_tab;
  grid_tab->add_next( viewer_tab );

  unsigned mult = 1;
  if( show_stab() )
  {
    mult = 2;
    grid_tab->add_next( viewer_tab2 );
  }

  vgui_shell_tableau_new shell_tab( grid_tab );

  vgui_menu main_menu;
  vgui_menu pause_menu;
  pause_menu.add( "Step once", &step_once, vgui_key(' ') );
  pause_menu.add( "Pause/unpause", &pause_cb, vgui_key('p') );
  main_menu.add( "Pause", pause_menu );

  vidtk::vgui::draw_track track_drawer( easy_tab );
  track_drawer.text_ = text_tab;
  track_drawer.trail_color_ = vil_rgba<float>( 0.0f, 1.0f, 0.0f );
  track_drawer.current_color_ = vil_rgba<float>( 0.0f, 1.0f, 0.0f );
  track_drawer.past_color_ = vil_rgba<float>( 1.0f, 0.9f, 0.9f );

  vidtk::vgui::draw_track bad_track_drawer( easy_tab );
  bad_track_drawer.text_ = text_tab;
  bad_track_drawer.trail_color_ = vil_rgba<float>( 1.0f, 0.0f, 0.0f );
  bad_track_drawer.current_color_ = vil_rgba<float>( 1.0f, 0.0f, 0.0f );
  bad_track_drawer.past_color_ = vil_rgba<float>( 1.0f, 0.9f, 0.9f );

  if( ! show_trails() )
  {
    track_drawer.cutoff_ = 1;
    track_drawer.draw_point_at_end_ = true;
    bad_track_drawer.cutoff_ = 1;
    bad_track_drawer.draw_point_at_end_ = true;
  }

  // Run the pipeline once, so we have the first image, etc.  We can
  // then figure out the appropriate size for the windows and so on.
  if( ! p.execute() )
  {
    vcl_cerr << "Initial step failed.\n";
    return EXIT_FAILURE;
  }

  vgui::adapt( shell_tab, timestamper.image().ni()*mult, timestamper.image().nj(), main_menu );

  vil_image_view<vxl_byte> dest;
  int dest_i0 = 0;
  int dest_j0 = 0;

  // this needs to hang around while the pipeline is running, so it
  // can't be too local a variable.
  vil_image_view<vxl_byte> gui_image;

  do
  {
    vcl_cout << "Done processing time = ";
    if( timestamper.timestamp().has_time() )
    {
      vcl_cout << vul_sprintf("%.3f",timestamper.timestamp().time_in_secs());
    }
    if( timestamper.timestamp().has_frame_number() )
    {
      vcl_cout << " (frame="<< timestamper.timestamp().frame_number() << ")";
    }
    vcl_cout << "\n";

    img_tab->set_image_view( timestamper.image() );
    img_tab->reread_image();
    img_tab->post_redraw();

    easy_tab->clear();
    text_tab->clear();

    {
      easy_tab->set_point_radius(5);
      easy_tab->set_line_width( 1 );
      easy_tab->set_foreground(0,0,1);
      typedef vcl_vector< vidtk::klt_track_ptr > track_container;
      typedef track_container::const_iterator iter_type;
      track_container const& trks = tracking.active_tracks();
      for( iter_type i = trks.begin(); i != trks.end(); ++i )
      {
        if( homog.is_good( *i ) )
        {
          track_drawer.draw_world2d( *convert_from_klt_track(*i) );
        }
        else
        {
          bad_track_drawer.draw_world2d( *convert_from_klt_track(*i) );
        }
      }
      easy_tab->post_redraw();
    }

    if( show_stab() )
    {
      vil_image_view<vxl_byte> const& curr = timestamper.image();
      vcl_cout << "H=\n"<<homog.image_to_world_homography()<<"\n";
      if( ! dest )
      {
        unsigned ni = curr.ni();
        unsigned nj = curr.nj();
        compute_output_size( ni, nj, dest_i0, dest_j0, homog.image_to_world_homography() );
        dest = vil_image_view<vxl_byte>( ni, nj, 1, curr.nplanes() );
      }
      vidtk::warp_image( curr, dest, homog.world_to_image_homography(), dest_i0, dest_j0 );
      //vidtk::warp_image( curr, dest, homog.world_to_image_homography() );
      vcl_cout << "curr= " << curr << "\n";
      vcl_cout << "dest= " << dest << "\n";
      vcl_cout << "dest pstep= " << dest.planestep() << "\n";

      img_tab2->set_image_view( dest );
      img_tab2->reread_image();
      img_tab2->post_redraw();
    }

    if( show_errors() )
    {
      easy_tab->set_point_radius(5);
      easy_tab->set_line_width( 1 );
      easy_tab->set_foreground(1,1,0);
      typedef vcl_vector< vidtk::klt_track_ptr > track_container;
      typedef track_container::const_iterator iter_type;
      track_container const& trks = tracking.active_tracks();
      for( iter_type i = trks.begin(); i != trks.end(); ++i )
      {
        vnl_vector_fixed<double,3> init_pt;
        vnl_vector_fixed<double,3> curr_pt;

        vidtk::klt_track_ptr cur_point = *i;
        while ( cur_point->tail() )
        {
          cur_point = cur_point->tail();
        }

        init_pt[0] = cur_point->point().x;
        init_pt[1] = cur_point->point().y;
        init_pt[2] = 0;
        curr_pt[0] = (*i)->point().x;
        curr_pt[1] = (*i)->point().y;
        curr_pt[2] = 0;

        vnl_vector_fixed<double,3> mapped_pt = homog.project_to_image( init_pt );
        easy_tab->add_line( curr_pt[0], curr_pt[1],
                            mapped_pt[0], mapped_pt[1] );
        easy_tab->add_point( mapped_pt[0], mapped_pt[1] );
      }

      easy_tab->post_redraw();
    }


    vgui::run_till_idle();
    if( ! config.get<bool>( "gui_writer:disabled" ) )
    {
      gui_image = vgui_utils::colour_buffer_to_view();
      gui_window_writer.set_image( gui_image );
      //gui_window_writer.step();
    }
    while( g_pause && ! g_step && ! vgui::quit_was_called() )
    {
      vgui::run_till_idle();
    }
    g_step = false;

  } while( ! vgui::quit_was_called()
           && p.execute() );


#else // NOGUI

  while( p.execute() )
  {
    vcl_cout << "Done processing time = ";
    if( timestamper.timestamp().has_time() )
    {
      vcl_cout << vul_sprintf("%.3f",timestamper.timestamp().time_in_secs());
    }
    if( timestamper.timestamp().has_frame_number() )
    {
      vcl_cout << " (frame="<< timestamper.timestamp().frame_number() << ")";
    }
    vcl_cout << "\n";
  }

#endif


  // Now that we're done, flush out still-active tracks

  vcl_vector< vidtk::klt_track_ptr > active_vector = tracking.active_tracks();

  write_out_tracks( active_vector, &homog );

  vcl_cout << "Done" << vcl_endl;

#ifndef NOGUI

  if( vgui::quit_was_called() ||
        ! persist_gui_at_end() )
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return vgui::run();
  }

#else

  return EXIT_SUCCESS;

#endif

}
