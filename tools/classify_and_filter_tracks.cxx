/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//classify_and_filter_tracks.cxx

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_sstream.h>
#include <vcl_iomanip.h>
#include <vcl_fstream.h>

#include <vul/vul_arg.h>
#include <vul/vul_timer.h>
#include <vnl/vnl_random.h>
#include <vnl/vnl_math.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>

#include <utilities/log.h>

#include <pipeline/sync_pipeline.h>

#include <tracking/track_reader_process.h>
#include <tracking/track_writer_process.h>

#include <classifier/platt_score_adaboost_classifier_pipeline.h>
#include <classifier/track_to_vector_of_descriptors.h>

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> config_file( "-c",
    "configuration file");
  vul_arg<vcl_string> output_pipeline( "--output-pipeline", 
    "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( "--output-config", 
    "Dump the configuration to this file", "" );

  vul_arg_parse( argc, argv );

  vidtk::sync_pipeline p;

  vidtk::track_reader_process file_reader("track_reader");
  vidtk::platt_score_adaboost_classifier_pipeline ada("ada_classifier");
  vidtk::track_writer_process file_writer("result_writer");

  p.add( &file_reader );
  p.add( &ada );
  p.add( &file_writer );

  p.connect( file_reader.tracks_port(), ada.set_source_tracks_port() );
  p.connect( ada.get_classified_tracks_port(), file_writer.set_tracks_port() );

  vidtk::config_block config;
  config.add_subblock(p.params(), "adaboost_track_classifier");
  config.add_component_to_check_list("adaboost_track_classifier");

  if( config_file.set() )
  {
    config.parse( config_file() );
  }
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
    return 0;
  }

  if( output_pipeline.set() )
  {
    p.output_graphviz_file( output_pipeline().c_str() );
    return EXIT_SUCCESS;
  }

  if( !p.set_params(config.subblock("adaboost_track_classifier")) )
  {
    vcl_cerr << "Error setting pipeline parameters\n";
    return EXIT_FAILURE;
  }

  // Process the pipeline nodes
  if( !p.initialize() )
  {
    vcl_cerr << "Failed to initialize pipeline\n";
    return EXIT_FAILURE;
  }

  unsigned int step_count = 0;
  vul_timer wall_clock;
  while( p.execute() )
  {
    ++step_count;
    vcl_cout << "               step = " << step_count << "\n"
             << "   wall clock speed = "
             << step_count*1000.0/wall_clock.real() << " steps/second\n";

  }

  return EXIT_SUCCESS;
}
