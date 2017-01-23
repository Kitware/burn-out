/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ocv_hog_detector_process.h"

#include <utilities/config_block.h>
#include <utilities/video_modality.h>

#include <vil/vil_resample_bilin.h>

#include <string>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "ocv_hog_detector_process" );


ocv_hog_detector_process
::ocv_hog_detector_process( std::string const& _name )
 : process( _name, "ocv_hog_detector_process" ),
   disabled_( false ),
   scale_( -1.0 ),
   burst_frame_count_( 1 ),
   skip_frame_count_( 0 ),
   skip_mode_( false ),
   frame_counter_( 0 ),
   target_scale_( -1.0 ),
   min_scale_factor_( 0.001 ),
   max_scale_factor_( 100.0 )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Completely disable this process and pass no outputs." );
  config_.add_parameter(
    "burst_frame_count",
    "1",
    "If skip_frame_count is not 0, the number of frames to run the "
    "detector for (in bursts) before skipping a few frames." );
  config_.add_parameter(
    "skip_frame_count",
    "0",
    "After running the detector for burst_frame_count frames, don't"
    "run the detection algorithm for this many frames." );
  config_.add_parameter(
    "target_scale",
    "-1.0",
    "If set, resize the input image to this scale before running "
    "the detection algorithm on it." );
  config_.add_parameter(
    "min_scale_factor",
    "0.001",
    "Smallest allowed scale factor, if target scale set." );
  config_.add_parameter(
    "max_scale_factor",
    "100.0",
    "Largest allowed scale factor, if target scale set." );

  config_.merge( ocv_hog_detector_settings().config() );
}


ocv_hog_detector_process
::~ocv_hog_detector_process()
{
}


config_block
ocv_hog_detector_process
::params() const
{
  return config_;
}


bool
ocv_hog_detector_process
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    LOG_INFO( this->name() << ": Loading Parameters" );

    disabled_ = blk.get<bool>( "disabled" );

    if( disabled_ )
    {
      return true;
    }

    ocv_hog_detector_settings dpm_options;
    dpm_options.read_config( blk );

    if( !detector_.configure( dpm_options ) )
    {
      throw config_block_parse_error( "Unable to configure ocv dpm model classifier." );
    }

    burst_frame_count_ = blk.get<unsigned>( "burst_frame_count" );
    skip_frame_count_ = blk.get<unsigned>( "skip_frame_count" );

    target_scale_ = blk.get<double>( "target_scale" );
    max_scale_factor_ = blk.get<double>( "max_scale_factor" );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Set internal config block
  config_.update( blk );
  return true;
}


bool
ocv_hog_detector_process
::initialize()
{
  return true;
}


bool
ocv_hog_detector_process
::step()
{
  image_objects_.clear();

  if( disabled_ )
  {
    return true;
  }

  // Handle internal frame downsamplers if enabled
  if( skip_frame_count_ > 0 )
  {
    frame_counter_++;

    if( skip_mode_ && frame_counter_ <= skip_frame_count_ )
    {
      return true;
    }
    else if( skip_mode_ )
    {
      frame_counter_ = 1;
      skip_mode_ = false;
    }
    else if( frame_counter_ >= burst_frame_count_ )
    {
      frame_counter_ = 0;
      skip_mode_ = true;
    }
  }

  // Run detector on either the input image or a resized version of it if enabled
  if( target_scale_ > 0.0 && scale_ > 0.0 )
  {
    double scale_factor = std::min( scale_ / target_scale_, max_scale_factor_ );
    scale_factor = std::max( scale_factor, min_scale_factor_ );

    //image_objects_ = detector_.detect( image_, scale_factor );
  }
  else
  {
    image_objects_ = detector_.detect( image_ );
  }

  return true;
}


void
ocv_hog_detector_process
::set_source_image( vil_image_view<vxl_byte> const& image )
{
  image_ = image;
}


void
ocv_hog_detector_process
::set_source_scale( double scale )
{
  scale_ = scale;
}


std::vector<image_object_sptr>
ocv_hog_detector_process
::image_objects() const
{
  return image_objects_;
}


} // end namespace vidtk
