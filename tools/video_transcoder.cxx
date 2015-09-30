/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///
/// This program reads a video with the frame process and writes
/// the frames as an image list. It may also be used to do simple
/// video processing as provided by the reader process like
/// spatial or temporal cropping and temporal sub-sampling.

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_iomanip.h>

#include <vul/vul_arg.h>

#include <pipeline/async_pipeline.h>
#include <process_framework/process.h>

#include <video/generic_frame_process.h>
#include <video/image_list_writer_process.h>


using namespace vidtk;

int main( int argc, char** argv )
{

  vul_arg<char const*> config_file( "-c", "Config file" );

  vul_arg<vcl_string> output_config(
    "--output-config",
    "Dump the configuration to this file", "" );

  if( argc == 1 )
  {
    vul_arg_base::display_usage_and_exit( "No input provided." );
  }

  vul_arg_parse( argc, argv );

  async_pipeline pipeline;

  vidtk::process_smart_pointer< vidtk::generic_frame_process<vxl_byte> >
    frame_src( new vidtk::generic_frame_process<vxl_byte>( "src" ) );
  vidtk::process_smart_pointer< vidtk::image_list_writer_process<vxl_byte> >
    writer( new vidtk::image_list_writer_process<vxl_byte>( "dest" ) );

  vidtk::config_block config;
  pipeline.add( frame_src );
  pipeline.add( writer );
  pipeline.connect( frame_src->copied_image_port(),
                    writer->set_image_port() );
  pipeline.connect( frame_src->timestamp_port(),
                    writer->set_timestamp_port() );

  config.add_subblock( pipeline.params(), "video_transcoder" );

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
    vcl_cout<< "Wrote config to " << output_config() << ". Exiting.\n";
    return EXIT_SUCCESS;
  }

  if( ! pipeline.set_params( config.subblock("video_transcoder") ) )
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

  vcl_cout << "Done" << vcl_endl;

  return EXIT_SUCCESS;
}
