/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file
///

#include <vcl_iostream.h>
#include <vcl_fstream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>

#include <vul/vul_arg.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_sprintf.h>

#include <vil/vil_image_view.h>

#include <utilities/ring_buffer_process.h>

#include <video/generic_frame_process.h>
#include <video/vidl_ffmpeg_writer_process.h>

#include <pipeline/sync_pipeline.h>


int main( int argc, char** argv )
{
  vul_arg<vcl_string> config_file( "-c", "Config file" );

  vul_arg<vcl_string> output_pipeline( "--output-pipeline", "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config( "--output-config", "Dump the configuration to this file", "" );

  vul_arg_parse( argc, argv );

  vidtk::sync_pipeline p;

  vidtk::generic_frame_process<vxl_byte> frame_src( "src" );
  p.add( &frame_src );

  vidtk::vidl_ffmpeg_writer_process writer( "writer" );
  p.add( &writer );
  p.connect( frame_src.image_port(), writer.set_frame_port() );

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


  while( p.execute() )
  {
    vcl_cout << "Done processing time = " << frame_src.timestamp().time()
             << " (frame="<< frame_src.timestamp().frame_number() << ")\n";
  }

  return EXIT_SUCCESS;
}
