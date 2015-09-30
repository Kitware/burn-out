/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <event_detectors/event_writer.h>
#include <event_detectors/event_reading_process.h>
#include <event_detectors/event_writing_process.h>
#include <utilities/apply_function_to_vector_process.txx>
#include <utilities/merge_n_vectors_process.txx>

#include <vul/vul_arg.h>
#include <vul/vul_timer.h>

#include <pipeline/sync_pipeline.h>

unsigned int GLOBAL_id;

vidtk::event_sptr transform(vidtk::event_sptr const & e)
{
  e->set_id(GLOBAL_id);
  GLOBAL_id++;
  return e;
}

int main(int argc, char **argv)
{
  vul_arg<vcl_string> event_file_a( "--file-a", "one of the event file to merge");
  vul_arg<vcl_string> event_file_b( "--file-b", "one of the event file to merge");
  vul_arg<vcl_string> output_config( "--output",
                                     "The output event file",
                                     "apb_out.events.txt" );
  vul_arg_parse( argc, argv );
  GLOBAL_id = 1;

  vidtk::sync_pipeline pipeline;
  vidtk::config_block config;

  vidtk::event_reading_process event_read_a("event_read_a");
  vidtk::event_reading_process event_read_b("event_read_b");
  vidtk::merge_n_vectors_process<vidtk::event_sptr> merger("event_merger");
  vidtk::apply_function_to_vector_process<vidtk::event_sptr, vidtk::event_sptr, transform> update_id("update_id");
  vidtk::event_writing_process event_writer("event_writer");

  pipeline.add(&event_read_a);
  pipeline.add(&event_read_b);
  pipeline.add(&merger);
  pipeline.add(&update_id);
  pipeline.add(&event_writer);

  pipeline.connect_optional( event_read_a.get_events_port(), merger.add_vector_port() );
  pipeline.connect_optional( event_read_b.get_events_port(), merger.add_vector_port() );
  pipeline.connect( merger.v_all_port(), update_id.set_input_port() );
  pipeline.connect( update_id.get_output_port(), event_writer.set_source_events_port() );

  config = pipeline.params();
  if(! event_file_a.set() || !event_file_b.set() )
  {
    vcl_cerr << "need two event file names" << vcl_endl;
  }
  config.set("event_read_a:events_filename", event_file_a());
  config.set("event_read_b:events_filename", event_file_b());
  config.set("event_writer:filename", output_config());

  vcl_vector<vidtk::track_sptr> tracks;

  if( !pipeline.set_params(config) )
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

  event_read_a.set_source_tracks( tracks );
  event_read_b.set_source_tracks( tracks );
  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( pipeline.execute() )
  {
    event_read_a.set_source_tracks( tracks );
    event_read_b.set_source_tracks( tracks );
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
    << "   wall clock speed = "
    << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }

  return 0;
}