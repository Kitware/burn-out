/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/track_reader_process.h>
#include <event_detectors/event_reading_process.h>
#include <event_detectors/event_writing_process.h>

#include <vul/vul_arg.h>
#include <vul/vul_timer.h>

#include <pipeline/sync_pipeline.h>

#include <vcl_iostream.h>
#include <vcl_fstream.h>
#include <vcl_string.h>

int main(int argc, char **argv)
{
  vul_arg<vcl_string> config_file( "-c", "configuration file");
  vul_arg<vcl_string> output_config( "--output-config",
                                     "Dump the configuration to this file", "" );
  vul_arg_parse( argc, argv );

  vidtk::sync_pipeline pipeline;
  vidtk::config_block config;

  vidtk::track_reader_process tracks_reader("track_reader");
  pipeline.add(&tracks_reader);

  vidtk::event_reading_process event_reader("event_reader");
  pipeline.add(&event_reader);

  vidtk::event_writing_process event_writer("event_writer");
  pipeline.add(&event_writer);

  config.add_subblock(pipeline.params(), "read_write_event");
  config.add_component_to_check_list("read_write_event");

  config.add_parameter( "read_tracks_in_one_step", "false",
                        "  false: progressive mode, read tracks in steps.\n"
                        "  true : batch mode, read tracks in one step.\n");

  if( config_file.set() )
  {
    config.parse( config_file() );
  }
  config.parse_arguments( argc, argv );

  bool read_tracks_in_one_step;
  config.get("read_tracks_in_one_step", read_tracks_in_one_step);
  if( read_tracks_in_one_step )
  {
    tracks_reader.set_batch_mode(true);
  }

  pipeline.connect_optional( tracks_reader.tracks_port(),
                    event_reader.set_source_tracks_port() );
  pipeline.connect( event_reader.get_events_port(),
                    event_writer.set_source_events_port() );

  if( output_config.set() )
  {
    vcl_ofstream fout( output_config().c_str() );
    if( ! fout )
    {
      vcl_cerr << "Couldn't open " << output_config() << " for writing\n";
      return false;
    }
    vcl_cerr << "output config" << vcl_endl;
    config.print( fout );
    return false;
  }

  if( !pipeline.set_params(config.subblock("read_write_event")) )
  {
    vcl_cerr << "Error setting pipeline parameters\n";
    return false;
  }

  // Process the pipeline nodes
  if( !pipeline.initialize() )
  {
    vcl_cerr << "Failed to initialize pipeline\n";
    return false;
  }

  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( pipeline.execute() )
  {
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
             << "   wall clock speed = "
             << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }
  return 0;
}
