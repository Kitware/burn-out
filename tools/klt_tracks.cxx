/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// This will track KLT features on a video, and optionally write out
/// these tracks to a file.

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
#include <vgl/algo/vgl_h_matrix_2d.h>

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
#endif // NOGUI

#include <kwklt/klt_track.h>
#include <kwklt/klt_tracking_process.h>
#include <kwklt/klt_pyramid_process.h>

#include <pipeline/async_pipeline.h>
#include <pipeline/async_pipeline_node.h>

#include <utilities/ring_buffer_process.h>
#include <utilities/timestamp_reader_process.h>
#include <utilities/async_observer_process.txx>

#include <video/generic_frame_process.h>
#include <video/greyscale_process.h>
#include <video/warp_image.h>

#include <tracking/homography_process.h>
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

#ifndef NOGUI
#include <tracking_gui/image_object.h>
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


vnl_vector_fixed<double,3>
project_to_image( vnl_vector_fixed<double,3> const& wld_pt,
                  const vgl_h_matrix_2d<double>& wld2img )
{
  assert( wld_pt[2] == 0 );
  vgl_homg_point_2d<double> in( wld_pt[0], wld_pt[1] );
  vgl_point_2d<double> out = wld2img * in;
  return vnl_vector_fixed<double,3>(out.x(), out.y(), 0.0 );
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
  vul_arg<bool> print_timing_tags( "--print-timing", "Print timing info in DartMeasurment tags", false );

  vul_arg<bool> show_stab( "--show-stab", "Show a preview of the stabilized frames (required outout-homog)", false );
  vul_arg<bool> show_trails( "--show-trails", "Show a trail of the track", false );
  vul_arg<bool> show_errors( "--show-errors", "Show the error of the pt vs transformed pt", false );

  // FIXME: implement show trails in world coordinates, by buffering the homographies.

  vul_arg_parse( argc, argv );

#ifndef NOGUI
  vgui::init( argc, argv );
#endif

  vidtk::async_pipeline p;

  vidtk::generic_frame_process<vxl_byte> frame_src( "src" );
  p.add( &frame_src );

  vidtk::timestamp_reader_process<vxl_byte> timestamper( "timestamper" );
  p.add( &timestamper );
  p.connect( frame_src.copied_image_port(), timestamper.set_source_image_port() );
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
  p.add( &homog );
  p.connect( tracking.created_tracks_port(), homog.set_new_tracks_port() );
  p.connect( tracking.active_tracks_port(), homog.set_updated_tracks_port() );

  vidtk::async_pipeline_node track_output_node;
  {
    track_output_node.set_name( "track_output" );
    p.add( track_output_node );
    p.connect( tracking.terminated_tracks_port(),
               vidtk::pipeline_aid::make_input< vcl_vector< vidtk::klt_track_ptr > const& >(
                 &track_output_node,
                 boost::bind( &write_out_tracks, _1, &homog ),
                 "tracks" ) );
  }

  vidtk::async_pipeline_node homog_output_node;
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

  // add timestamp observer to display approximate status
  vidtk::async_observer_process<vidtk::timestamp> timestamp_observer( "timestamp_observer" );
  p.add( &timestamp_observer );
  p.connect( timestamper.timestamp_port(), timestamp_observer.set_input_port() );

#ifndef NOGUI
  // add gui observers if in gui mode
  vidtk::async_observer_process<vil_image_view<vxl_byte> > image_observer( "image_observer" );
  p.add( &image_observer );
  p.connect( timestamper.image_port(), image_observer.set_input_port() );

  typedef vcl_vector< vidtk::klt_track_ptr > track_container;
  vidtk::async_observer_process<track_container> tracks_observer( "tracks_observer" );
  p.add( &tracks_observer );
  p.connect( tracking.active_tracks_port(), tracks_observer.set_input_port() );

  vidtk::async_observer_process<vgl_h_matrix_2d<double> > img2wld_observer( "img2wld_observer" );
  p.add( &img2wld_observer );
  p.connect( homog.image_to_world_homography_port(), img2wld_observer.set_input_port() );

  vidtk::async_observer_process<vgl_h_matrix_2d<double> > wld2img_observer( "wld2img_observer" );
  p.add( &wld2img_observer );
  p.connect( homog.world_to_image_homography_port(), wld2img_observer.set_input_port() );

#endif

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
  if( output_homog.set() && show_stab() )
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


  vidtk::timestamp ts;
  vil_image_view<vxl_byte> image;
  typedef vcl_vector< vidtk::klt_track_ptr > track_container;
  track_container trks;
  vgl_h_matrix_2d<double> img2wld, wld2img;

  // start the async pipeline running
  p.run_async();

  // get data from the pipeline observers
  bool is_running = true;
  is_running = is_running && timestamp_observer.get_data(ts);
  is_running = is_running && image_observer.get_data(image);
  is_running = is_running && tracks_observer.get_data(trks);
  is_running = is_running && img2wld_observer.get_data(img2wld);
  is_running = is_running && wld2img_observer.get_data(wld2img);

  // Run the pipeline once, so we have the first image, etc.  We can
  // then figure out the appropriate size for the windows and so on.
  if( !is_running )
  {
    vcl_cerr << "Failed to get first frame.\n";
    return EXIT_FAILURE;
  }

  vgui::adapt( shell_tab, image.ni()*mult, image.nj(), main_menu );

  vil_image_view<vxl_byte> dest;
  int dest_i0 = 0;
  int dest_j0 = 0;
  do
  {
    vcl_cout << "Done processing time = ";
    if( ts.has_time() )
    {
      vcl_cout << vul_sprintf("%.3f",ts.time_in_secs());
    }
    if( ts.has_frame_number() )
    {
      vcl_cout << " (frame="<< ts.frame_number() << ")";
    }
    vcl_cout << "\n";

    img_tab->set_image_view( image );
    img_tab->reread_image();
    img_tab->post_redraw();

    easy_tab->clear();
    text_tab->clear();

    {
      easy_tab->set_point_radius(5);
      easy_tab->set_line_width( 1 );
      easy_tab->set_foreground(0,0,1);
      typedef track_container::const_iterator iter_type;
      for( iter_type i = trks.begin(); i != trks.end(); ++i )
      {
        vidtk::track_sptr trk = convert_from_klt_track(*i);
        // FIXME: we can't use this function in an async pipeline
        if( true /*homog.is_good( *i )*/ )
        {
          track_drawer.draw_world2d( *trk );
        }
        else
        {
          bad_track_drawer.draw_world2d( *trk );
        }
      }

      easy_tab->post_redraw();
    }

    if( output_homog.set() && show_stab() )
    {
      vcl_cout << "H=\n"<<img2wld<<"\n";
      if( ! dest )
      {
        unsigned ni = image.ni();
        unsigned nj = image.nj();
        compute_output_size( ni, nj, dest_i0, dest_j0, img2wld );
        dest = vil_image_view<vxl_byte>( ni, nj, 1, image.nplanes() );
      }
      vidtk::warp_image( image, dest, wld2img, dest_i0, dest_j0 );
      //vidtk::warp_image( image, dest, wld2img );
      vcl_cout << "curr= " << image << "\n";
      vcl_cout << "dest= " << dest << "\n";
      vcl_cout << "dest pstep= " << dest.planestep() << "\n";

      img_tab2->set_image_view( dest );
      img_tab2->reread_image();
      img_tab2->post_redraw();
    }

    if( output_homog.set() && show_errors() )
    {
      easy_tab->set_point_radius(5);
      easy_tab->set_line_width( 1 );
      easy_tab->set_foreground(1,1,0);
      typedef track_container::const_iterator iter_type;
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

        vnl_vector_fixed<double,3> mapped_pt = project_to_image(init_pt, wld2img);
        easy_tab->add_line( curr_pt[0], curr_pt[1],
                            mapped_pt[0], mapped_pt[1] );
        easy_tab->add_point( mapped_pt[0], mapped_pt[1] );
      }

      easy_tab->post_redraw();
    }


    vgui::run_till_idle();
    while( g_pause && ! g_step && ! vgui::quit_was_called() )
    {
      vgui::run_till_idle();
    }
    g_step = false;

    // get next data from the pipeline observers
    is_running = is_running && timestamp_observer.get_data(ts);
    is_running = is_running && image_observer.get_data(image);
    is_running = is_running && tracks_observer.get_data(trks);
    is_running = is_running && img2wld_observer.get_data(img2wld);
    is_running = is_running && wld2img_observer.get_data(wld2img);

  } while( ! vgui::quit_was_called() && is_running );

  if( vgui::quit_was_called() )
  {
    // interrupt the pipeline
    p.cancel();
  }
  else
  {
    // wait for the pipeline to finish up
    p.wait();
  }


#else // NOGUI

  // start the async pipeline running
  p.run_async();
  vidtk::timestamp ts;
  while( timestamp_observer.get_data(ts) )
  {
    vcl_cout << "Done processing time = ";
    if( ts.has_time() )
    {
      vcl_cout << vul_sprintf("%.3f",ts.time_in_secs());
    }
    if( ts.has_frame_number() )
    {
      vcl_cout << " (frame="<< ts.frame_number() << ")";
    }
    vcl_cout << vcl_endl;
  }
  // wait for the pipeline to finish up
  p.wait();

#endif


  // Now that we're done, flush out still-active tracks

  vcl_vector< vidtk::klt_track_ptr > active_vector = tracking.active_tracks();

  write_out_tracks( active_vector, &homog );

  if (print_timing_tags())
  {
    p.output_timing_measurements();
  }

#ifndef NOGUI

  vcl_cout << "Done" << vcl_endl;
  if( vgui::quit_was_called() )
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
