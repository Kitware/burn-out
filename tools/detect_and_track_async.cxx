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

///THIS WAS FORKED FROM detect_and_track.cxx

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_iomanip.h>

#include <vul/vul_arg.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_sprintf.h>
#include <vsl/vsl_binary_io.h>

#include <pipeline/async_pipeline.h>
#include <pipeline/sync_pipeline.h>
#include <process_framework/process.h>

#include <utilities/timestamp_reader_process.h>
#include <utilities/transform_homography_process.h>

#include <video/generic_frame_process.h>

#include <tracking/gui_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/full_tracking_super_process.h>

#ifndef NOGUI
#include <tracking_gui/vgui_process.h>
#endif // NOGUI

using namespace vidtk;

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



  vidtk::process_smart_pointer< vidtk::gui_process >
#ifdef NOGUI
    bt_gui( new vidtk::gui_process( "bt_gui" ) ),
    vgui( new vidtk::gui_process( "vgui" ) );
#else
    bt_gui( new vidtk::vgui_process( "bt_gui" ) ),
    vgui( new vidtk::vgui_process( "vgui" ) );
#endif // NOGUI


  vidtk::process_smart_pointer< vidtk::generic_frame_process<vxl_byte> >
    frame_src( new vidtk::generic_frame_process<vxl_byte>( "src" ) );

  process_smart_pointer< full_tracking_super_process<vxl_byte> >
    full_tracking_sp( new full_tracking_super_process<vxl_byte>("full_tracking_sp" ) );
  full_tracking_sp->set_guis( vgui, bt_gui );

  vidtk::process_smart_pointer< vidtk::track_writer_process >
    track_writer( new vidtk::track_writer_process( "track_writer" ) );


  async_pipeline pipeline;

  pipeline.add( frame_src );

  pipeline.add( full_tracking_sp );
  pipeline.add( track_writer );
  pipeline.connect( frame_src->copied_image_port(),
                    full_tracking_sp->set_source_image_port() );
  pipeline.connect( frame_src->timestamp_port(),
                    full_tracking_sp->set_source_timestamp_port() );
  pipeline.connect( frame_src->metadata_port(),
		    full_tracking_sp->set_source_metadata_port());

  vidtk::config_block config;

  // Get parameters registered from pipeline.
  config.add_subblock( pipeline.params(), "detect_and_track" );

  // enable async pipeline by default
  config.set_value("detect_and_track:full_tracking_sp:run_async", true);

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

  bool trk_writer_disabled;
  config.get( "detect_and_track:" + track_writer->name() + ":disabled", trk_writer_disabled );
  if( !trk_writer_disabled )
  {
    pipeline.connect( full_tracking_sp->finalized_tracks_port(),
                      track_writer->set_tracks_port() );
    pipeline.connect( full_tracking_sp->finalized_timestamp_port(),
                      track_writer->set_timestamp_port() );
  }

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

  pipeline.run();

  if (print_timing_tags())
  {
  std::ofstream timing_file( "pipeline_timing.txt" );
  if (! timing_file)
  {
    vcl_cout << "Could not open process timing file";
  }
  else
  {
    pipeline.output_timing_measurements(timing_file);
    timing_file.close();
  }

//    pipeline.output_timing_measurements();
  }

  vcl_cout << "Done" << vcl_endl;

  return EXIT_SUCCESS;
}
