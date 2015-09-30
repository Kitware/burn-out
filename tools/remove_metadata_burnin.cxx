/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_string.h>

#include <vul/vul_arg.h>

#include <pipeline/async_pipeline.h>
#include <pipeline/async_pipeline_node.h>
#include <process_framework/process.h>

#include <video/generic_frame_process.h>
#include <video/metadata_mask_super_process.h>
#include <video/image_list_writer_process.h>
#include <video/metadata_inpainting_process.h>

using namespace vidtk;

int main( int argc, char** argv )
{
  vul_arg<char const*> config_file( "-c", "Config file" );
  vul_arg<vcl_string> output_pipeline(
    "--output-pipeline",
    "Dump the pipeline graph to this file (graphviz .dot file)", "" );
  vul_arg<vcl_string> output_config(
    "--output-config",
    "Dump the configuration to this file", "" );

  if( argc == 1 )
  {
    vul_arg_base::display_usage_and_exit( "No input provided." );
  }

  vul_arg_parse( argc, argv );

  async_pipeline p;

  vidtk::process_smart_pointer< vidtk::generic_frame_process<vxl_byte> >
    frame_src( new vidtk::generic_frame_process<vxl_byte>( "src" ) );
  vidtk::process_smart_pointer< vidtk::metadata_mask_super_process<vxl_byte> >
    metadata_mask( new vidtk::metadata_mask_super_process<vxl_byte>( "md_mask_sp" ) );
  vidtk::process_smart_pointer< vidtk::image_list_writer_process<bool> >
    mask_writer( new vidtk::image_list_writer_process<bool>( "mask_writer" ) );
  vidtk::process_smart_pointer< vidtk::metadata_inpainting_process >
    inpaint( new vidtk::metadata_inpainting_process( "inpaint" ) );
  vidtk::process_smart_pointer< vidtk::image_list_writer_process<vxl_byte> >
    writer( new vidtk::image_list_writer_process<vxl_byte>( "writer" ) );

  vidtk::config_block config;

  p.add( frame_src );
  p.add( metadata_mask );
  p.add( mask_writer );
  p.add( inpaint );
  p.add( writer );

  p.connect( frame_src->copied_image_port(),
             metadata_mask->set_source_image_port() );
  p.connect( frame_src->timestamp_port(),
             metadata_mask->set_source_timestamp_port() );
  p.connect( frame_src->copied_image_port(),
             inpaint->set_source_image_port() );
  p.connect( frame_src->timestamp_port(),
             mask_writer->set_timestamp_port() );
  p.connect( metadata_mask->get_mask_port(),
             mask_writer->set_image_port() );
  p.connect( metadata_mask->get_mask_port(),
             inpaint->set_mask_port() );
  p.connect( frame_src->timestamp_port(),
             writer->set_timestamp_port() );
  p.connect( inpaint->inpainted_image_port(),
             writer->set_image_port() );

  config.add_subblock( p.params(), "remove_metadata_burnin" );

  if( config_file.set() )
  {
    config.parse( config_file() );
  }

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  if( output_pipeline.set() )
  {
    p.output_graphviz_file( output_pipeline().c_str() );
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

  if( !p.set_params( config.subblock("remove_metadata_burnin") ) )
  {
    vcl_cerr << "Failed to set pipeline parameters\n";
    return EXIT_FAILURE;
  }

  if( !p.initialize() )
  {
    vcl_cout << "Failed to initialize pipeline\n";
    return EXIT_FAILURE;
  }

  p.run();

  vcl_cout << "Done" << vcl_endl;

  return EXIT_SUCCESS;
}
