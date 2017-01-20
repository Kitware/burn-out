/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>

#include <vul/vul_arg.h>

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/async_pipeline_node.h>
#include <process_framework/process.h>

#include <video_io/generic_frame_process.h>
#include <video_io/image_list_writer_process.h>
#include <video_transforms/greyscale_process.h>
#include <video_properties/border_detection_process.h>
#include <video_transforms/deep_copy_image_process.h>
#include <video_transforms/warp_image_process.h>
#include <video_properties/border_detection_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/mask_merge_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/mask_overlay_process.h>
#include <video_io/image_list_writer_process.h>
#include <video_transforms/video_enhancement_process.h>

#include <logger/logger.h>


using namespace vidtk;


int main( int argc, char** argv )
{
  vul_arg<std::string> config_file( "-c", "Config file" );
  vul_arg<std::string> output_pipeline(
    "--output-pipeline",
    "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<std::string> output_config(
    "--output-config",
    "Dump the configuration to this file", "" );

  if( argc == 1 )
  {
    vul_arg_base::display_usage_and_exit( "No input provided." );
  }

  vul_arg_parse( argc, argv );

  async_pipeline p;

  process_smart_pointer< generic_frame_process<vxl_byte> >
  proc_src( new generic_frame_process<vxl_byte>( "src" ) );

  process_smart_pointer< deep_copy_image_process<vxl_byte> >
  proc_copy( new deep_copy_image_process<vxl_byte>("deep_copy") );

  process_smart_pointer< image_list_writer_process<vxl_byte> >
  proc_writer( new image_list_writer_process<vxl_byte>( "writer" ) );

  process_smart_pointer< video_enhancement_process<vxl_byte> >
  proc_enhance( new video_enhancement_process<vxl_byte>( "enhancer" ) );

  config_block config;

  p.add( proc_src );
  p.add( proc_writer );
  p.add( proc_copy );
  p.add( proc_enhance );

  // Connect input to copier
  p.connect( proc_src->copied_image_port(),
             proc_copy->set_source_image_port() );

  // Connect RGB input image to everything else
  p.connect( proc_copy->image_port(),
             proc_enhance->set_source_image_port() );

  // Connect timestamps
  p.connect( proc_src->timestamp_port(),
             proc_writer->set_timestamp_port() );

  // Connect to output writers
  p.connect( proc_enhance->copied_output_image_port(),
             proc_writer->set_image_port() );

  // Read config file
  config.add_subblock( p.params(), "video_enhancement" );

  // Change default options on processes
  config_block defaults;
  defaults.add_parameter( "video_enhancement:" + proc_enhance->name() + ":disabled", "true", "Default override" );
  config.update( defaults );

  // Read user config file
  if( config_file.set() )
  {
    config.parse( config_file() );
  }

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  if( output_pipeline.set() )
  {
    p.output_graphviz_file( output_pipeline().c_str() );
    std::cout << "Wrote pipeline to " << output_pipeline() << ". Exiting" << std::endl;
    return EXIT_SUCCESS;
  }

  if( output_config.set() )
  {
    std::ofstream fout( output_config().c_str() );
    if( ! fout )
    {
      std::cerr << "Couldn't open " << output_config() << " for writing" << std::endl;
      return EXIT_FAILURE;
    }
    config.print( fout );
    std::cout << "Wrote config to " << output_config() << ". Exiting." << std::endl;
    return EXIT_SUCCESS;
  }

  if( !p.set_params( config.subblock("video_enhancement") ) )
  {
    std::cerr << "Failed to set pipeline parameters" << std::endl;
    return EXIT_FAILURE;
  }

  if( !p.initialize() )
  {
    std::cerr << "Failed to initialize pipeline" << std::endl;
    return EXIT_FAILURE;
  }

  p.run();

  std::cout << "Done" << std::endl;
  return EXIT_SUCCESS;
}
