/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// This program will generate object tracks from a video stream.  It
/// will use a frame-differencing algorithm to determine moving
/// objects, and then track those objects with a data association
/// tracker.

///THIS WAS FORKED FROM hadwav_frame_diff_track2_vil.cxx

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_iomanip.h>

#include <vul/vul_arg.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_sprintf.h>
#include <vsl/vsl_binary_io.h>

#include <pipeline/sync_pipeline.h>
#include <process_framework/process.h>

#include <utilities/timestamp_reader_process.h>
#include <utilities/transform_vidtk_homography_process.h>
#include <utilities/log.h>

#include <video/generic_frame_process.h>
#include <video/image_list_writer_process.h>
#include <video/deep_copy_image_process.h>
#include <video/generic_frame_process.h>

#include <tracking/gen_world_units_per_pixel_process.h>
#include <tracking/image_object_reader_process.h>
#include <tracking/track.h>
#include <tracking/diff_super_process.h>
#include <tracking/conn_comp_super_process.h>
#include <tracking/tracking_super_process.h>
#include <tracking/gui_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/stabilization_super_process.h>
#include <tracking/full_tracking_super_process.h>

#ifndef NOGUI
#include <tracking_gui/vgui_process.h>
#endif // NOGUI

using namespace vidtk;

#ifdef PRINT_DEBUG_INFO
#include <tracking/debug_tracker.h>
#endif

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
  
  vul_arg<vcl_string> output_pipeline( 
    "--output-pipeline", 
    "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( 
    "--output-config", 
    "Dump the configuration to this file", "" );
  vul_arg<unsigned int> pipeline_param(
    "--pipeline-param", 
    "0 for full, 1 for diff, 2 for blobs, 3 for tracker", 0 );
  vul_arg<bool> db_out_enable(
    "--db-out",
    "Enable database output", false );
  vul_arg<bool> print_timing_tags(
    "--print-timing",
    "Print timing info in DartMeasurment tags", false );

  if( argc == 1 )
  {
    vul_arg_base::display_usage_and_exit( "No input provided." );
  }

  vul_arg_parse( argc, argv );

  if( !( pipeline_param() <= 3 ) )
  {
    vcl_cout << "Must choose 0,1,2, or 3 for pipeline param, default is 0." 
             << vcl_endl;
    return EXIT_FAILURE;
  }

  sync_pipeline pipeline;

  typedef vidtk::super_process_pad_impl< vcl_vector< track_sptr > > tracks_pad;

  vidtk::process_smart_pointer< vidtk::gui_process >
#ifdef NOGUI
    bt_gui( new vidtk::gui_process( "bt_gui" ) ),
    vgui( new vidtk::gui_process( "vgui" ) );
#else
    bt_gui( new vidtk::vgui_process( "bt_gui" ) ),
    vgui( new vidtk::vgui_process( "vgui" ) );
#endif // NOGUI

  process_smart_pointer< full_tracking_super_process<vxl_byte> >
    full_tracking_sp( new full_tracking_super_process<vxl_byte>("full_tracking_sp" ) );
  full_tracking_sp->set_guis( vgui, bt_gui );

  //Begin Pipeline Implementation.  Create only the pipeline that's requested.
  process_smart_pointer< diff_super_process< vxl_byte > > 
    diff_sp( new diff_super_process< vxl_byte >( "diff_sp" ) );
  process_smart_pointer< conn_comp_super_process< vxl_byte > > 
    conn_comp_sp( new conn_comp_super_process<vxl_byte>( "conn_comp_sp" ) );
  process_smart_pointer< tracking_super_process< vxl_byte > >
    tracking_sp( new tracking_super_process< vxl_byte >( "tracking_sp" ) );
  process_smart_pointer< stabilization_super_process<vxl_byte> >
    stab_sp( new stabilization_super_process<vxl_byte>( "stab_sp" ) );

  vidtk::process_smart_pointer< vidtk::gen_world_units_per_pixel_process<vxl_byte> > 
    gen_gsd( new vidtk::gen_world_units_per_pixel_process<vxl_byte>( "gen_gsd" ) );
  vidtk::process_smart_pointer< vidtk::generic_frame_process<vxl_byte> > 
    frame_src( new vidtk::generic_frame_process<vxl_byte>( "src" ) );
  vidtk::process_smart_pointer< vidtk::timestamp_reader_process<vxl_byte> > 
    timestamper( new vidtk::timestamp_reader_process<vxl_byte>( "timestamper" ) );
  vidtk::process_smart_pointer< vidtk::deep_copy_image_process<vxl_byte> > 
    image_copy( new vidtk::deep_copy_image_process<vxl_byte>( "source_deep_copy" ) );
  vidtk::process_smart_pointer< vidtk::transform_vidtk_homography_process<timestamp, timestamp, timestamp, plane_ref_t> >
    trans_for_ground( new vidtk::transform_vidtk_homography_process<timestamp, timestamp, timestamp, plane_ref_t>( "trans_for_ground" ) );

  vidtk::process_smart_pointer<vidtk::generic_frame_process<bool> >
    read_from_diff(new vidtk::generic_frame_process<bool>( "input_from_diff" ) );

  vidtk::process_smart_pointer< vidtk::image_object_reader_process >
    source_objects( new vidtk::image_object_reader_process( "source_objects" ) );

  vidtk::process_smart_pointer< vidtk::image_list_writer_process<vxl_byte> >
    gui_window_writer( new vidtk::image_list_writer_process<vxl_byte>( "gui_writer" ) );

  vidtk::process_smart_pointer< vidtk::track_writer_process >
    filtered_track_writer( new vidtk::track_writer_process( "filtered_track_writer" ) );

  vidtk::process_smart_pointer< tracks_pad >
    pad_out_filtered_tracks( new tracks_pad( "filtered_tracks_out_pad" ) );
 
  vidtk::config_block config;

  switch( pipeline_param() )  //Choose which pipeline to implement.
  {
  case 0: //Run Existing Pipeline
    pipeline.add( frame_src );

    pipeline.add( full_tracking_sp );
    pipeline.add( filtered_track_writer );
    pipeline.add( pad_out_filtered_tracks );
    pipeline.connect( frame_src->image_port(),
                      full_tracking_sp->set_source_image_port() );
    pipeline.connect( frame_src->timestamp_port(),
                      full_tracking_sp->set_source_timestamp_port() );
    pipeline.connect( frame_src->metadata_port(),
                      full_tracking_sp->set_source_metadata_port()); 
    pipeline.connect( full_tracking_sp->filtered_tracks_port(),
                      filtered_track_writer->set_tracks_port() );
    pipeline.connect( full_tracking_sp->filtered_tracks_port(),
                      pad_out_filtered_tracks->set_value_port() );
    ///TODO: connect meta data port in other cases below, Juda is being lazy.
    break;

  case 1: // Diff Only
    pipeline.add( frame_src );
    pipeline.add( timestamper );
    pipeline.connect( frame_src->image_port(),
                      timestamper->set_source_image_port() );
    pipeline.connect( frame_src->timestamp_port(),
                      timestamper->set_source_timestamp_port() );
    pipeline.add( stab_sp );
    pipeline.add( image_copy );
    pipeline.connect( timestamper->image_port(),
                      image_copy->set_source_image_port() );
    pipeline.add( trans_for_ground );
    pipeline.connect( stab_sp->src_to_ref_homography_port(),
                      trans_for_ground->set_source_homography_port() );

    pipeline.add( gen_gsd );
    pipeline.connect( timestamper->image_port(),
                      gen_gsd->set_image_port() );
    pipeline.connect( trans_for_ground->homography_port(),
                      gen_gsd->set_source_vidtk_homography_port() );

    pipeline.add( diff_sp );
    pipeline.connect( stab_sp->src_to_ref_homography_port(),
                      diff_sp->set_src_to_ref_homography_port() );
    pipeline.connect( image_copy->image_port(),
                      diff_sp->set_source_image_port() );
    pipeline.connect( timestamper->timestamp_port(),
                      diff_sp->set_source_timestamp_port() );
    pipeline.connect( gen_gsd->world_units_per_pixel_port(),
                      diff_sp->set_world_units_per_pixel_port() );

    pipeline.add( vgui );
    pipeline.connect( timestamper->image_port(),
                      vgui->set_image_port() );
    pipeline.connect( diff_sp->fg_out_port(),
                      vgui->set_fg_image_port() );
    pipeline.connect( diff_sp->diff_out_port(),
                      vgui->set_diff_image_port() );
    pipeline.connect( trans_for_ground->inv_bare_homography_port(),
                      vgui->set_world_homog_port() );
    pipeline.connect( tracking_sp->tracker_gate_sigma_port(),
                      vgui->set_tracker_gate_sigma_port() );

    pipeline.add( gui_window_writer );
    pipeline.connect( timestamper->timestamp_port(),
                      gui_window_writer->set_timestamp_port() );
    pipeline.connect( vgui->gui_image_out_port(),
                      gui_window_writer->set_image_port() );

    break;

  case 2: //Blobs Only
    pipeline.add( frame_src );
    pipeline.add( timestamper );
    pipeline.connect( frame_src->image_port(),
                      timestamper->set_source_image_port() );
    pipeline.connect( frame_src->timestamp_port(),
                      timestamper->set_source_timestamp_port() );
    pipeline.add( stab_sp );
    pipeline.add( trans_for_ground );
    pipeline.connect( stab_sp->src_to_ref_homography_port(),
                      trans_for_ground->set_source_homography_port() );
    pipeline.add( read_from_diff );

    pipeline.add( conn_comp_sp );
    pipeline.connect( read_from_diff->image_port() ,
                      conn_comp_sp->set_fg_image_port() );
    pipeline.connect( timestamper->timestamp_port(),
                      conn_comp_sp->set_source_timestamp_port() );
    pipeline.connect( trans_for_ground->homography_port(),
                      conn_comp_sp->set_src_to_wld_homography_port() );
    pipeline.connect( gen_gsd->world_units_per_pixel_port(),
                      conn_comp_sp->set_world_units_per_pixel_port() );

    pipeline.add( vgui );
    pipeline.add( gui_window_writer );
    pipeline.connect( vgui->gui_image_out_port(),
                      gui_window_writer->set_image_port() );
    //Outputs above... inputs below...
    pipeline.connect( timestamper->timestamp_port(),
                      vgui->set_source_timestamp_port() );
    pipeline.connect( timestamper->image_port(),
                      vgui->set_image_port() );
    pipeline.connect( conn_comp_sp->projected_objects_port(),
                      vgui->set_objects_port() );
    pipeline.connect( conn_comp_sp->output_objects_port(),
                      vgui->set_filter1_objects_port() );
    pipeline.connect( conn_comp_sp->morph_image_port(),
                      vgui->set_morph_image_port() );
    pipeline.connect( trans_for_ground->inv_bare_homography_port(),
                      vgui->set_world_homog_port() );
    pipeline.connect( timestamper->timestamp_port(),
                      gui_window_writer->set_timestamp_port() );
    break;

  case 3: // Tracking Only
    pipeline.add( frame_src );
    pipeline.add( timestamper );
    pipeline.connect( frame_src->image_port(),
                      timestamper->set_source_image_port() );
    pipeline.connect( frame_src->timestamp_port(),
                      timestamper->set_source_timestamp_port() );
    pipeline.add( stab_sp );
    pipeline.add( image_copy );
    pipeline.connect( timestamper->image_port(),
                      image_copy->set_source_image_port() );
    pipeline.add( trans_for_ground );
    pipeline.connect( stab_sp->src_to_ref_homography_port(),
                      trans_for_ground->set_source_homography_port() );
    pipeline.add( source_objects );
    pipeline.connect( timestamper->timestamp_port(),
                      source_objects->set_timestamp_port() );

    pipeline.add( tracking_sp );
    pipeline.connect( timestamper->timestamp_port(),
                      tracking_sp->set_timestamp_port() );
    pipeline.connect( trans_for_ground->homography_port(),
                      tracking_sp->set_src_to_wld_homography_port() );
    pipeline.connect( trans_for_ground->inv_homography_port(),
                      tracking_sp->set_wld_to_src_homography_port() );
    pipeline.connect( source_objects->objects_port(),
                      tracking_sp->set_source_objects_port() );
    pipeline.connect( image_copy->image_port(),
                      tracking_sp->set_source_image_port() );

    pipeline.add( vgui );
    pipeline.add( gui_window_writer );
    pipeline.connect( vgui->gui_image_out_port(),
                      gui_window_writer->set_image_port() );
    //Outputs above... inputs below...
    pipeline.connect( timestamper->timestamp_port(),
                      vgui->set_source_timestamp_port() );
    pipeline.connect( timestamper->image_port(),
                      vgui->set_image_port() );
    pipeline.connect( tracking_sp->amhi_image_port(),
                      vgui->set_amhi_image_port() );
    pipeline.connect( tracking_sp->init_tracks_port(),
                      vgui->set_init_tracks_port() );
    pipeline.connect( tracking_sp->filtered_tracks_port(),
                      vgui->set_filter2_tracks_port() );
    pipeline.connect( tracking_sp->active_tracks_port(),
                      vgui->set_active_tracks_port() );
    pipeline.connect( tracking_sp->active_trackers_port(),
                      vgui->set_active_trkers_port() );
    pipeline.connect( trans_for_ground->inv_bare_homography_port(),
                      vgui->set_world_homog_port() );
    pipeline.connect( timestamper->timestamp_port(),
                      gui_window_writer->set_timestamp_port() );

    break;

  default:

    vcl_cout << "Error picking pipeline to generate...  Exiting..." 
             << vcl_endl;
    return EXIT_FAILURE;
  }
  
  config.add_subblock( pipeline.params(), "detect_and_track" );

  // If there is a config file specified as a parameter, then parse it.
  if( config_file.set() )
  {
    if ( ! config.parse( config_file() ) )
    {
      return EXIT_FAILURE;
    }
  }

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  if( output_pipeline.set() )
  {
    // Temporary hack here to *build* full_tracking_sp::pipeline_ for
    // output_pipeline options.
    if( ! full_tracking_sp->set_params( config.subblock("detect_and_track:" + full_tracking_sp->name() ) ) )
    {
      vcl_cerr << "Failed to set "<< full_tracking_sp->name() <<" parameters\n";
      return EXIT_FAILURE;
    }

    pipeline.output_graphviz_file( output_pipeline().c_str() );
    vcl_cout << "Wrote pipeline to " << output_pipeline() << ". Exiting\n";
    return EXIT_SUCCESS;
  }

  if( output_config.set() )
  {
    vcl_ofstream fout( output_config().c_str() );
    if( ! fout )
    {
      vcl_cerr << "Couldn't open " << output_config() << " for writing\n";
      return EXIT_FAILURE;
    }
    config.print( fout );
    vcl_cout<< "Wrote config to " << output_config() << ". Exiting.\n";
    return EXIT_SUCCESS;
  }

  if( ! pipeline.set_params( config.subblock("detect_and_track") ) )
  {
    vcl_cerr << "Failed to set pipeline parameters\n";
    return EXIT_FAILURE;
  }

  if( ! pipeline.initialize() )
  {
    vcl_cout << "Failed to initialize pipeline\n";
    return EXIT_FAILURE;
  }

  while( pipeline.execute() )
  {
  }

  if (print_timing_tags())
  {
    pipeline.output_timing_measurements();
  }

  vcl_cout << "Done" << vcl_endl;

  return EXIT_SUCCESS;
}
