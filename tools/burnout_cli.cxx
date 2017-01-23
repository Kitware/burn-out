/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/async_pipeline_node.h>

#include <process_framework/process.h>

#include <utilities/folder_manipulation.h>

#include <pipelines/remove_burnin_pipeline.h>

#include <video_io/generic_frame_process.h>
#include <video_io/qt_ffmpeg_writer_process.h>
#include <video_io/image_list_writer_process.h>

#include <video_transforms/frame_downsampling_process.h>
#include <video_transforms/video_enhancement_process.h>
#include <video_transforms/rescale_image_process.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <vul/vul_arg.h>

#ifdef WITH_MAPTK_ENABLED
#include <vital/algorithm_plugin_manager.h>
#include <vital/util/get_paths.h>

#include <kwiversys/SystemTools.hxx>
#include <kwiversys/Directory.hxx>
#endif

#ifdef USE_CAFFE
#include <glog/logging.h>
#endif

#include <logger/logger.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_burnout_cli_cxx__
VIDTK_LOGGER( "burnout_cli" );

using namespace vidtk;
namespace fs = boost::filesystem;

const std::string file_source_id = "source";
const std::string downsampler_id = "downsampler";
const std::string writer_id = "writer";
const std::string remover_id = "remove_metadata_burnin";
const std::string detector_id = remover_id + ":md_mask_sp";

const double default_frame_rate = 29.97;

bool estimate_properties( const std::string video_name,
                          double& frame_rate,
                          unsigned& ni, unsigned& nj,
                          const bool map_to_known = true )
{
  frame_rate = 0.0;
  ni = 0; nj = 0;

  process_smart_pointer< generic_frame_process< vxl_byte > > frame_source(
    new generic_frame_process< vxl_byte >( file_source_id ) );

  config_block config = frame_source->params();

  config.set( "type", "vidl_ffmpeg" );
  config.set( "vidl_ffmpeg:filename", video_name );

  if( !frame_source->set_params( config ) || !frame_source->initialize() )
  {
    LOG_ERROR( "Failed to set video property reader parameters" );
    return false;
  }

  if( frame_source->step2() == process::FAILURE )
  {
    LOG_ERROR( "Failed to step video property reader source" );
    return false;
  }

  if( frame_source->step2() == process::SUCCESS )
  {
    ni = frame_source->image().ni();
    nj = frame_source->image().nj();
  }
  else
  {
    LOG_ERROR( "Failed to step video property reader source" );
    return false;
  }

  frame_rate = frame_source->frame_rate();

  if( frame_rate <= 0 || frame_rate != frame_rate )
  {
    const unsigned test_itr = 60;

    frame_rate_estimator fre;
    frame_rate_estimator_output fr;

    for( unsigned f = 0; f < test_itr; ++f )
    {
      if( frame_source->step2() == process::FAILURE )
      {
        break;
      }

      fr = fre.step( frame_source->timestamp() );
    }

    frame_rate = fr.source_fps;

    if( map_to_known )
    {
      std::vector< double > known_fps;

      known_fps.push_back( 10.00 );
      known_fps.push_back( 15.00 );
      known_fps.push_back( 24.00 );
      known_fps.push_back( 29.97 );
      known_fps.push_back( 30.00 );
      known_fps.push_back( 59.96 );
      known_fps.push_back( 60.00 );

      double min_dist = std::numeric_limits<double>::max();
      unsigned min_ind = 0;

      for( unsigned i = 0; i < known_fps.size(); ++i )
      {
        double dist = std::abs( known_fps[i]-frame_rate );

        if( dist < min_dist )
        {
          min_ind = i;
          min_dist = dist;
        }
      }

      frame_rate = known_fps[min_ind];
    }
  }

  return true;
}

int main( int argc, char** argv )
{
  // Command line options settings
  vul_arg< std::string > input_data(
    "--input",
    "Input video file, directory containing only images, or image list txt file.",
    "" );
  vul_arg< std::string > output_data(
    "--output",
    "Output video file or directory for output images.",
    "" );
  vul_arg< std::string > sensor_type(
    "--sensor",
    "Video sensor type if known, possible values are: auto-detect, "
    "predator, reaper, mts, raytheon, l3, gen-dyn, gd, mx, v14, "
    "blue-devil, and flir. Several of these entries map to the same "
    "configuration.",
    "auto-detect" );
  vul_arg< std::string > detector_type(
    "--detector",
    "Detection method, can be either moving or static; for detectors "
    "aimed at detecting burn-in on moving vs static cameras respectivly. "
    "Without using a GPU, the moving detector is generally faster, while "
    "the static detector functions better on stationary cameras. Warning: "
    "this is not always true and occasionally one detector will work "
    "better than others on certain datasets. The static detector is also "
    "relatively new and untested.",
    "moving" );
  vul_arg< std::string > detector_sensitivity(
    "--sensitivity",
    "Detection sensitivity, can be tight-fit, default, or aggressive. "
    "Tighter masks can help improve inpainting quality, at the cost of "
    "possibly missing on-screen display components.",
    "default" );
  vul_arg< std::string > inpainting_type(
    "--inpainter",
    "Inpainting method, can either be fast, intermediate, quality, "
    "or high_quality. Generally, this improves the output video quality "
    "at the cost of speed.",
    "quality" );
  vul_arg< double > frame_rate(
    "--frame-rate",
    "Output video frame rate. If this rate is lower, less frames "
    "will be processed so the removal process will run faster.",
    default_frame_rate );
  vul_arg< std::string > retain_center_opt(
    "--retain-center",
    "Option for what to do in the center of images. Can either "
    "be disabled (to inpaint detected crosshairs), enabled (to "
    "adaptively estimate how much center should be left unmodified), "
    "or two values specified as a pair \"h:w\" where h is percentage "
    "of pixels in the height dimension to leave untouched, and w for "
    "width (e.g. a value of 0.5:0.5 will leave the inner half of the "
    "image unmodified).",
    "disabled" );
  vul_arg< std::string > show_encoding(
    "--show-encoding",
    "Whether or not to output video ffmpeg encoding details to console.",
    "false" );
  vul_arg< std::string > encoding_args(
    "--encoding-args",
    "Output video codec and bit rate override, default will be "
    "guessed based on the output file extension. Must be specified "
    "in the same format as ffmpeg, see ffmpeg documentation.",
    "" );
  vul_arg< std::string > mosaic_dir(
    "--mosaic-directory",
    "An optional folder to dump mosaics generated from the video "
    "into, if using quality or high_quality inpainting.",
    "" );
  vul_arg< std::string > mask_dir(
    "--mask-directory",
    "An optional folder to dump on-screen display detection masks "
    "into.",
    "" );
  vul_arg< bool > use_gpu(
    "--use-gpu",
    "An experimental flag which will attempt to automatically use "
    "the best GPU in the system for certain operations, such as "
    "static detection mode or component detection for certain types of "
    "burn-in. WARNING: This option requires a decent NVIDIA GPU and may "
    "fail, we are still improving support for multiple types of GPU.",
    false );
  vul_arg< std::string > config_root(
    "--config-root",
    "A pointer to the root config_mappings.ini file. This only needs "
    "to be specified if the file can't be found in a default location.",
    "" );

  if( argc == 1 )
  {
    vul_arg_base::display_usage_and_exit( "No input provided." );
  }

  vul_arg_parse( argc, argv );

  // Initialize google glog level
#ifdef USE_CAFFE
  google::InitGoogleLogging( "--minloglevel=2" );
#endif

  // Register plugin path
#ifdef WITH_MAPTK_ENABLED
  std::string rel_plugin_path = kwiver::vital::get_executable_path() + "/../lib/maptk";
  kwiver::vital::algorithm_plugin_manager::instance().add_search_path( rel_plugin_path );
  kwiver::vital::algorithm_plugin_manager::instance().register_plugins();
#endif

  // Insert extra space for pretty logging
  std::cout << std::endl << "Initializing Processing Pipeline" << std::endl;

  // Don't load the input file but guess if it's a file or a directory
  bool input_is_dir = false;
  bool input_is_list = false;
  bool ffmpeg_source = true;

  if( !input_data().empty() )
  {
    fs::path data_path( input_data() );

    if( !fs::exists( data_path ) )
    {
      LOG_ERROR( "Input file does not exist." );
      return EXIT_FAILURE;
    }

    input_is_dir = fs::is_directory( data_path );
    input_is_list = ( data_path.extension() == ".txt" );

    ffmpeg_source = ( !input_is_dir && !input_is_list );
  }
  else
  {
    LOG_ERROR( "An input video or folder name must be set" );
    return EXIT_FAILURE;
  }

  // Setup pipeline
  async_pipeline p;

  process_smart_pointer< generic_frame_process< vxl_byte > > frame_source(
    new generic_frame_process< vxl_byte >( file_source_id ) );
  process_smart_pointer< frame_downsampling_process< vxl_byte > > downsampler(
    new frame_downsampling_process< vxl_byte >( downsampler_id ) );
  process_smart_pointer< remove_burnin_pipeline< vxl_byte > > burnin_remover(
    new remove_burnin_pipeline< vxl_byte >( remover_id ) );
  process_smart_pointer< image_list_writer_process< vxl_byte > > image_writer(
    new image_list_writer_process< vxl_byte >( writer_id ) );
  process_smart_pointer< qt_ffmpeg_writer_process > video_writer(
    new qt_ffmpeg_writer_process( writer_id ) );

  p.add( frame_source );
  p.add( burnin_remover );

  if( !frame_rate() == default_frame_rate )
  {
    p.connect( frame_source->copied_image_port(),
               burnin_remover->set_image_port() );
    p.connect( frame_source->timestamp_port(),
               burnin_remover->set_timestamp_port() );
  }
  else
  {
    p.add( downsampler );

    p.connect( frame_source->copied_image_port(),
               downsampler->set_input_image_port() );
    p.connect( frame_source->timestamp_port(),
               downsampler->set_input_timestamp_port() );
    p.connect( downsampler->get_output_image_port(),
               burnin_remover->set_image_port() );
    p.connect( downsampler->get_output_timestamp_port(),
               burnin_remover->set_timestamp_port() );
  }

  if( ffmpeg_source )
  {
    p.add( video_writer );

    p.connect( burnin_remover->inpainted_image_port(),
               video_writer->set_frame_port() );
  }
  else
  {
    p.add( image_writer );

    p.connect( burnin_remover->inpainted_image_port(),
               image_writer->set_image_port() );
    p.connect( burnin_remover->timestamp_port(),
               image_writer->set_timestamp_port() );
  }

  config_block config = p.params();

  // Disable inpainted writer by default
  config.set( remover_id + ":inpainted_writer:disabled", "false" );

  // Find location of config file set
  fs::path config_dir;

  if( !config_root().empty() )
  {
    config_dir = fs::path( config_root() ).stem();
  }
  else
  {
    fs::path binary_path( fs::initial_path< fs::path >() );
    binary_path = fs::system_complete( fs::path( argv[0] ) );
    fs::path working_dir( fs::current_path() );

    std::vector< fs::path > search_dirs;
    search_dirs.push_back( binary_path.parent_path() );
    search_dirs.push_back( binary_path.parent_path() / fs::path("osd_detection"));
    search_dirs.push_back( binary_path.parent_path().branch_path() / fs::path("osd_detection"));
    search_dirs.push_back( binary_path.parent_path().branch_path() / fs::path("share"));
    search_dirs.push_back( binary_path.parent_path().branch_path() / fs::path("etc"));
    search_dirs.push_back( search_dirs.back() / fs::path( "osd_detection" ) );
    search_dirs.push_back( working_dir.parent_path() );
    search_dirs.push_back( working_dir.parent_path() / fs::path("osd_detection"));
    search_dirs.push_back( fs::path( "/home/matt/Dev/configs-ec/fmv_configs/common/osd_detection" ) );

    if( ffmpeg_source )
    {
      config.set( writer_id + ":ffmpeg_location",
                  ( binary_path.parent_path() / fs::path( "ffmpeg" ) ).string() );
    }

    bool found = false;

    for( unsigned i = 0; i < search_dirs.size(); ++i )
    {
      if( fs::exists( search_dirs[i] / fs::path( "config_mappings.ini" ) ) )
      {
        config_dir = search_dirs[i];
        found = true;
        break;
      }
    }

    if( !found )
    {
      LOG_ERROR( "Could not locate burn-out configuration files" );
      return EXIT_FAILURE;
    }
  }

  // Read sensor type to get correct config file
  std::string sensor_type_lc = sensor_type();
  boost::algorithm::to_lower( sensor_type_lc );
  std::map< std::string, std::string > sensor_mappings;

  sensor_mappings[ "auto" ]        = "default";
  sensor_mappings[ "auto-detect" ] = "default";
  sensor_mappings[ "default" ]     = "default";
  sensor_mappings[ "predator" ]    = "mts";
  sensor_mappings[ "reaper" ]      = "mts";
  sensor_mappings[ "mts" ]         = "mts";
  sensor_mappings[ "raytheon" ]    = "mts";
  sensor_mappings[ "l3" ]          = "mx";
  sensor_mappings[ "mx" ]          = "mx";
  sensor_mappings[ "wescam" ]      = "mx";
  sensor_mappings[ "gd" ]          = "v14";
  sensor_mappings[ "gen-dyn" ]     = "v14";
  sensor_mappings[ "v14" ]         = "v14";
  sensor_mappings[ "blue-devil" ]  = "v14";
  sensor_mappings[ "bd" ]          = "v14";
  sensor_mappings[ "flir" ]        = "flir";

  if( sensor_mappings.find( sensor_type_lc ) == sensor_mappings.end() )
  {
    LOG_ERROR( "Invalid sensor type " << sensor_type() );
    return EXIT_FAILURE;
  }

  // Read config file
  fs::path config_fn( "remove_burnin_" + sensor_mappings[ sensor_type_lc ] + ".conf" );
  config.parse( ( config_dir / config_fn ).string() );

  // Parse the remaining arguments as additions to the config file
  config.parse_arguments( argc, argv );

  // Apply all of our special command line options
  double video_fr;
  unsigned ni, nj;

  if( !ffmpeg_source )
  {
    config.set( file_source_id + ":type", "image_list" );

    if( input_is_list )
    {
      config.set( file_source_id + ":image_list:file", input_data() );
    }
    else
    {
      config.set( file_source_id + ":image_list:glob", input_data() + "/*.*" );
    }

    video_fr = default_frame_rate;
  }
  else
  {
    config.set( file_source_id + ":type", "vidl_ffmpeg" );
    config.set( file_source_id + ":vidl_ffmpeg:filename", input_data() );

    if( !estimate_properties( input_data(), video_fr, ni, nj ) )
    {
      LOG_ERROR( "Unable to retrive video properties" );
      return EXIT_FAILURE;
    }
  }

  if( !output_data().empty() )
  {
    if( ffmpeg_source )
    {
      config.set( writer_id + ":filename", output_data() );
      fs::path output_folder = fs::path( output_data() ).parent_path();

      if( !output_folder.empty() && !fs::exists( output_folder ) )
      {
        LOG_ERROR( "Output directory: " <<
                   output_folder.string() <<
                   " does not exist for writing" );

        return EXIT_FAILURE;
      }
    }
    else
    {
      fs::path dir_to_make( output_data() );

      if( fs::exists( dir_to_make ) )
      {
        if( fs::is_directory( dir_to_make ) )
        {
          LOG_WARN( "Output directory " << output_data() << " already exists, "
                    "outputting images regardless." );
        }
        else
        {
          LOG_ERROR( "When processing a folder of images, output must be a folder "
                     "to dump processed images into" );

          return EXIT_FAILURE;
        }
      }
      else if( !fs::create_directory( dir_to_make ) )
      {
        LOG_ERROR( "Unable to create output directory " << output_data() );
        return EXIT_FAILURE;
      }

      config.set( writer_id + ":pattern", output_data() + "/output%06d.png" );
    }
  }
  else if( mosaic_dir().empty() ) // If mosaic dir is set but no output video, that's okay
  {                               // for the use case where we just want to generate mosaics
    LOG_ERROR( "An output video name must be set" );
    return EXIT_FAILURE;
  }

  if( use_gpu() )
  {
    config.set( detector_id + ":mask_recognizer:use_gpu_override", "auto" );
#ifdef USE_CAFFE
    config.set( detector_id + ":cnn_detector:use_gpu", "auto" );
#endif
  }
  else
  {
    config.set( detector_id + ":mask_recognizer:use_gpu_override", "no" );
#ifdef USE_CAFFE
    config.set( detector_id + ":cnn_detector:use_gpu", "no" );
#endif
  }

  double downsampling_rate = 1.0;
  double throttle_fr = ( frame_rate() < video_fr ? frame_rate() : video_fr );

  if( ffmpeg_source )
  {
    config.set( writer_id + ":frame_rate", throttle_fr );

    config.set( downsampler_id + ":disabled", false );
    config.set( downsampler_id + ":rate_limiter_enabled", true );
    config.set( downsampler_id + ":rate_threshold", throttle_fr );

    downsampling_rate = video_fr / throttle_fr;

    if( !encoding_args().empty() )
    {
      config.set( writer_id + ":encoding_args", encoding_args() );
    }
  }
  else
  {
    config.set( downsampler_id + ":disabled", true );
  }

  if( !mosaic_dir().empty() )
  {
    config.set( remover_id + ":inpainter:mosaic_output_dir", mosaic_dir() );
  }

  if( !mask_dir().empty() )
  {
    fs::path dir_to_make( mask_dir() );
    fs::create_directory( dir_to_make );

    config.set( remover_id + ":mask_writer:disabled", false );
    config.set( remover_id + ":mask_writer:pattern", mask_dir() + "/mask%2$04d.pbm" );
  }

  if( detector_type() == "moving" )
  {
    config.set( detector_id + ":detection_mode", "pixel_classifier" );
  }
  else if( detector_type() == "static" )
  {
#ifdef USE_CAFFE
    config.set( detector_id + ":detection_mode", "cnn_classifier" );
    config.set( detector_id + ":mask_recognizer:use_initial_approximation", true );
    config.set( detector_id + ":mask_recognizer:full_dilation_amount", 4 );
    config.set( detector_id + ":mask_recognizer:static_dilation_amount", 4 );
    config.set( detector_id + ":mask_recognizer:classifier_threshold", -0.99 );
    config.set( detector_id + ":feature_sp:feature_code", 0 );
    config.set( detector_id + ":burnin_detect1:disabled", "forced" );
    config.set( detector_id + ":burnin_detect2:disabled", "forced" );
    config.set( detector_id + ":mask_refiner1:disabled", true );
    config.set( detector_id + ":mask_refiner2:disabled", true );
#else
    // Requires a build with caffe
    LOG_ERROR( "A build with caffe is required to run the static detector" );
    return EXIT_FAILURE;
#endif
  }
  else
  {
    LOG_ERROR( "Unknown detector type: " << detector_type() );
    return EXIT_FAILURE;
  }

  if( detector_sensitivity() == "tight-fit" )
  {
    config.set( detector_id + ":mask_detector:interval_1_adjustment", 0.000 );
    config.set( detector_id + ":mask_detector:interval_2_adjustment", 0.015 );
    config.set( detector_id + ":mask_detector:interval_3_adjustment", 0.030 );
    config.set( detector_id + ":mask_refiner1:closing_radius", 1.0 );
    config.set( detector_id + ":mask_refiner1:dilation_radius", 1.0 );
    config.set( detector_id + ":mask_refiner1:clfr_adjustment", 0.000 );
    config.set( detector_id + ":mask_refiner2:closing_radius", 2.0 );
    config.set( detector_id + ":mask_refiner2:dilation_radius", 0.0 );
    config.set( detector_id + ":mask_refiner2:clfr_adjustment", 0.000 );

    if( detector_type() == "static" )
    {
      config.set( detector_id + ":mask_recognizer:classifier_threshold", -0.100 );
    }
  }
  else if( detector_sensitivity() == "default" )
  {
    config.set( detector_id + ":mask_detector:interval_1_adjustment", 0.007 );
    config.set( detector_id + ":mask_detector:interval_2_adjustment", 0.017 );
    config.set( detector_id + ":mask_detector:interval_3_adjustment", 0.037 );
    config.set( detector_id + ":mask_refiner1:closing_radius", 2.0 );
    config.set( detector_id + ":mask_refiner1:dilation_radius", 2.0 );
    config.set( detector_id + ":mask_refiner1:clfr_adjustment", 0.000 );
    config.set( detector_id + ":mask_refiner2:closing_radius", 2.0 );
    config.set( detector_id + ":mask_refiner2:dilation_radius", 3.0 );
    config.set( detector_id + ":mask_refiner2:clfr_adjustment", 0.000 );

    if( detector_type() == "static" )
    {
      config.set( detector_id + ":mask_recognizer:classifier_threshold", -0.990 );
    }
  }
  else if( detector_sensitivity() == "aggressive" )
  {
    config.set( detector_id + ":mask_detector:interval_1_adjustment", 0.005 );
    config.set( detector_id + ":mask_detector:interval_2_adjustment", 0.030 );
    config.set( detector_id + ":mask_detector:interval_3_adjustment", 0.060 );
    config.set( detector_id + ":mask_refiner1:closing_radius", 5.0 );
    config.set( detector_id + ":mask_refiner1:dilation_radius", 5.0 );
    config.set( detector_id + ":mask_refiner1:clfr_adjustment", 0.001 );
    config.set( detector_id + ":mask_refiner2:closing_radius", 5.0 );
    config.set( detector_id + ":mask_refiner2:dilation_radius", 4.0 );
    config.set( detector_id + ":mask_refiner2:clfr_adjustment", 0.001 );
    config.set( detector_id + ":mask_refiner1:apply_fills", true );
    config.set( detector_id + ":mask_refiner2:apply_fills", true );

    if( detector_type() == "static" )
    {
      config.set( detector_id + ":mask_recognizer:classifier_threshold", -0.995 );
    }
  }
  else
  {
    LOG_ERROR( "Invalid detector sensitivity: " << detector_sensitivity() );
    return EXIT_FAILURE;
  }

  if( inpainting_type() == "fast" )
  {
    config.set( remover_id + ":inpainter:algorithm", "nearest" );
    config.set( remover_id + ":inpainter:stab_image_factor", 0.00 );
    config.set( remover_id + ":inpainter:unstab_image_factor", 0.30 );
    config.set( remover_id + ":inpainter:use_mosaic", false );
    config.set( remover_id + ":inpainter:max_buffer_size", 0 );
    config.set( remover_id + ":inpainter:mosaic_method", "use_latest" );
    config.set( remover_id + ":stab_sp:mode", "disabled" );
    config.set( remover_id + ":use_motion", "false" );
  }
  else if( inpainting_type() == "intermediate" )
  {
    config.set( remover_id + ":inpainter:algorithm", "telea" );
    config.set( remover_id + ":inpainter:stab_image_factor", 0.90 );
    config.set( remover_id + ":inpainter:unstab_image_factor", 0.00 );
    config.set( remover_id + ":inpainter:use_mosaic", false );
    config.set( remover_id + ":inpainter:max_buffer_size", 0 );
    config.set( remover_id + ":inpainter:mosaic_method", "use_latest" );
    config.set( remover_id + ":stab_sp:mode", "compute" );
    config.set( remover_id + ":use_motion", "false" );
  }
  else if( inpainting_type() == "quality" )
  {
    config.set( remover_id + ":inpainter:algorithm", "telea" );
    config.set( remover_id + ":inpainter:stab_image_factor", 0.90 );
    config.set( remover_id + ":inpainter:unstab_image_factor", 0.00 );
    config.set( remover_id + ":inpainter:use_mosaic", true );
    config.set( remover_id + ":inpainter:max_buffer_size", 40 );
    config.set( remover_id + ":inpainter:mosaic_method", "exp_average" );
    config.set( remover_id + ":stab_sp:mode", "compute" );
    config.set( remover_id + ":use_motion", "true" );
  }
  else if( inpainting_type() == "high_quality" )
  {
    config.set( remover_id + ":inpainter:algorithm", "telea" );
    config.set( remover_id + ":inpainter:stab_image_factor", 0.90 );
    config.set( remover_id + ":inpainter:unstab_image_factor", 0.00 );
    config.set( remover_id + ":inpainter:use_mosaic", true );
    config.set( remover_id + ":inpainter:max_buffer_size", 400 );
    config.set( remover_id + ":inpainter:mosaic_method", "exp_average" );
    config.set( remover_id + ":inpainter:illum_trigger", "90" );
    config.set( remover_id + ":stab_sp:mode", "compute_maptk" );
    config.set( remover_id + ":use_motion", "true" );
  }
  else
  {
    LOG_ERROR( "Invalid inpainter type: " << inpainting_type() );
    return EXIT_FAILURE;
  }

  if( retain_center_opt() != "disabled" && retain_center_opt() != "disable" && retain_center_opt() != "false" )
  {
    config.set( remover_id + ":inpainter:retain_center", retain_center_opt() );

    if( retain_center_opt() == "enabled" || retain_center_opt() == "enable" || retain_center_opt() == "true" )
    {
      config.set( detector_id + ":burnin_detect1:disabled", "forced" );
      config.set( detector_id + ":burnin_detect2:disabled", "forced" );
    }
  }

  if( ffmpeg_source )
  {
    config.set( writer_id + ":output_encoding", show_encoding() );
  }

  // Run the pipeline normally
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

  std::cout << std::endl << "Processing Video Data" << std::endl << std::endl;

  video_writer->set_frame_count( frame_source->nframes() / downsampling_rate );

  p.run();

  std::cout << std::endl << "Processing Complete" << std::endl;
  return EXIT_SUCCESS;
}
