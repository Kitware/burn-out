/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/async_pipeline_node.h>

#include <process_framework/process.h>

#include <utilities/folder_manipulation.h>

#include <pipelines/remove_burnin_pipeline.h>

#include <video_io/generic_frame_process.h>

#include <vul/vul_arg.h>

#include <iostream>
#include <fstream>
#include <string>

using namespace vidtk;

const std::string file_source_id = "source";
const std::string pipeline_id = "remove_metadata_burnin";
const std::string detector_id = pipeline_id + ":md_mask_sp";

void construct_pipeline( async_pipeline& p )
{
  vidtk::process_smart_pointer< vidtk::generic_frame_process< vxl_byte > >
    frame_source( new vidtk::generic_frame_process< vxl_byte >( file_source_id ) );
  vidtk::process_smart_pointer< vidtk::remove_burnin_pipeline< vxl_byte > >
    burnin_remover( new vidtk::remove_burnin_pipeline< vxl_byte >( pipeline_id ) );

  p.add( frame_source );
  p.add( burnin_remover );

  p.connect( frame_source->copied_image_port(),
             burnin_remover->set_image_port() );
  p.connect( frame_source->timestamp_port(),
             burnin_remover->set_timestamp_port() );
}

int run_pipeline( async_pipeline& p, config_block& config )
{
  if( !p.set_params( config ) )
  {
    std::cerr << "Failed to set pipeline parameters" << std::endl;
    return EXIT_FAILURE;
  }

  if( !p.initialize() )
  {
    std::cout << "Failed to initialize pipeline" << std::endl;
    return EXIT_FAILURE;
  }

  p.run();

  std::cout << "Done" << std::endl;
  return EXIT_SUCCESS;
}

int main( int argc, char** argv )
{
  // Command line options settings
  vul_arg<char const*> config_file(
    "-c",
    "Config file" );
  vul_arg<std::string> output_pipeline(
    "--output-pipeline",
    "Dump the pipeline graph to this file (graphviz .dot file)",
    "" );
  vul_arg<std::string> output_config(
    "--output-config",
    "Dump the configuration to this file",
    "" );
  vul_arg<std::string> extract_features_dir(
    "--extract-features",
    "Location of directory containing formatted groundtruth used for "
    "raw feature extraction.",
    "" );
  vul_arg<std::string> feature_type(
    "--feature-type",
    "Type of features we are extracting. Can be pixel1, pixel2, or "
    "type for level 1 and 2 pixel classification features, and the "
    "type classifier respectively. See \"Real-Time Heads-Up Display "
    "Detection\" paper for more information.",
    "pixel1" );
  vul_arg<std::string> gt_image_type(
    "--gt-image-type",
    "Image format of groundtruthed frames used in feature extraction",
    "png" );
  vul_arg<std::string> output_feature_file(
    "--feature-file",
    "Output features file to write training data to",
    "" );

  if( argc == 1 )
  {
    vul_arg_base::display_usage_and_exit( "No input provided." );
  }

  vul_arg_parse( argc, argv );

  // Setup pipeline
  async_pipeline p;
  construct_pipeline( p );
  config_block config = p.params();

  // Enable inpainted writer by default
  config.set( pipeline_id + ":inpainted_writer:disabled", "false" );

  // Read config file
  if( config_file.set() )
  {
    config.parse( config_file() );
  }

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  if( output_pipeline.set() )
  {
    p.output_graphviz_file( output_pipeline().c_str() );
    std::cout << "Wrote pipeline to " << output_pipeline() << ". Exiting." << std::endl;
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
    std::cout<< "Wrote config to " << output_config() << ". Exiting." << std::endl;
    return EXIT_SUCCESS;
  }

  // Check if in feature extraction mode
  if( !extract_features_dir().empty() )
  {
    // Check feature type
    enum{ PIXEL1, PIXEL2, TYPE } target_feature_type;

    if( feature_type() == "pixel1" )
    {
      target_feature_type = PIXEL1;
    }
    else if( feature_type() == "pixel2" )
    {
      target_feature_type = PIXEL2;
    }
    else if( feature_type() == "type" )
    {
      target_feature_type = TYPE;
    }
    else
    {
      std::cerr << "Invalid feature type: " << feature_type() << std::endl;
      return EXIT_FAILURE;
    }

    // Get list of sub folders or files in the supplied directory
    std::vector< std::string > entries;

    if( target_feature_type != TYPE )
    {
      list_all_subfolders( extract_features_dir(), entries );
    }
    else
    {
      list_files_in_folder( extract_features_dir(), entries );
    }

    if( entries.empty() )
    {
      std::cerr << "Folder: " << extract_features_dir() << " has no data." << std::endl;
      return EXIT_FAILURE;
    }

    // Flush output file, all invocations of run_pipeline will append data to it
    std::ofstream ftest( output_feature_file().c_str() );

    if( !ftest.is_open() )
    {
      std::cerr << "Unable to open file for writing: " << output_feature_file() << std::endl;
      return EXIT_FAILURE;
    }

    ftest.close();

    // Set any required options for feature extraction
    if( target_feature_type == PIXEL1 )
    {
      config.set( file_source_id + ":type", "image_list" );
      config.set( file_source_id + ":image_list:ignore_substring", "gt" );
      config.set( detector_id + ":mask_detector:is_training_mode", "true" );
      config.set( detector_id + ":mask_detector:output_filename", output_feature_file() );
    }
    else if( target_feature_type == PIXEL2 )
    {
      config.set( file_source_id + ":type", "image_list" );
      config.set( file_source_id + ":image_list:ignore_substring", "gt" );
      config.set( detector_id + ":mask_detector:is_training_mode", "false" );
      config.set( detector_id + ":mask_refiner1:training_file", output_feature_file() );
    }
    else
    {
      config.set( file_source_id + ":type", "vidl_ffmpeg" );
      config.set( detector_id + ":mask_detector:is_training_mode", "false" );
      config.set( detector_id + ":mask_recognizer:descriptor_output_file", output_feature_file() );
    }

    config.set( detector_id + ":mask_detector:reset_buffer_length", "0" );
    config.set( detector_id + ":mask_refiner1:reset_buffer_length", "0" );
    config.set( detector_id + ":mask_refiner2:reset_buffer_length", "0" );

    // Disable process which are not required
    config.set( pipeline_id + ":inpainter:disabled", "true" );
    config.set( detector_id + ":overlay_writer:disabled", "true" );

    // Remove any existing filelist configurations
    config_block to_remove;
    if( config.has( file_source_id + ":image_list:file" ) )
    {
      to_remove.add_parameter( file_source_id + ":image_list:file", "", "" );
    }
    if( config.has( file_source_id + ":image_list:list" ) )
    {
      to_remove.add_parameter( file_source_id + ":image_list:list", "", "" );
    }
    config.remove( to_remove );

    // Run pipeline on all training samples
    for( unsigned i = 0; i < entries.size(); i++ )
    {
      std::string entry_str;

      if( target_feature_type != TYPE )
      {
        entry_str = entries[i] + "/*." + gt_image_type();
        config.set( file_source_id + ":image_list:glob", entry_str );
        config.set( detector_id + ":mask_detector:groundtruth_dir", entries[i] );
        config.set( detector_id + ":mask_refiner1:groundtruth_dir", entries[i] );
      }
      else
      {
        entry_str = entries[i];
        config.set( file_source_id + ":vidl_ffmpeg:filename", entry_str );
        config.set( detector_id + ":mask_detector:groundtruth_dir", entries[i] );
      }

      std::cout << "Processing entry " << entry_str << std::endl;

      async_pipeline to_run;
      construct_pipeline( to_run );

      if( run_pipeline( to_run, config ) == EXIT_FAILURE )
      {
        return EXIT_FAILURE;
      }
    }

    return EXIT_SUCCESS;
  }

  // Run the pipeline normally
  return run_pipeline( p, config );
}
