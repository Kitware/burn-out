/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "video_enhancement_process.h"

#include <utilities/config_block.h>

#include <video_transforms/video_transform_helper_functors.h>
#include <video_transforms/invert_image_values.h>

#include <vil/algo/vil_gauss_filter.h>

#include <limits>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

VIDTK_LOGGER("video_enhancement_process_txx");

namespace vidtk
{

// Process Definition
template <typename PixType>
video_enhancement_process<PixType>
::video_enhancement_process( std::string const& _name )
  : process( _name, "video_enhancement_process" ),
    src_image_( NULL ),
    mask_image_( NULL ),
    reset_flag_( false ),
    enable_flag_( true ),
    process_disabled_( false ),
    masking_enabled_( false ),
    smoothing_enabled_( false ),
    std_dev_( 0.6 ),
    half_width_( 2 ),
    inversion_enabled_( false ),
    awb_enabled_( true ),
    awb_settings_(),
    illumination_mode_(INVALID)
{
  // General process settings (which filters to enable)
  config_.add_parameter( "disabled",
                         "false",
                         "Completely disable this process and pass the input image" );
  config_.add_parameter( "smoothing_enabled",
                         "false",
                         "Perform extra internal smoothing on the input" );
  config_.add_parameter( "masking_enabled",
                         "false",
                         "Whether or not we should only use certain parts of the scene "
                         "when computing properties such as white balancing shifts." );
  config_.add_parameter( "auto_white_balance",
                         "true",
                         "Whether or not auto-white balancing is enabled" );
  config_.add_parameter( "normalize_brightness",
                         "true",
                         "If enabled, will attempt to stabilize video illumination" );
  config_.add_parameter( "inversion_enabled",
                         "false",
                         "Should we invert the input image?" );
  config_.add_parameter( "normalize_brightness_mode", "sampled_mean",
                         "The algorithm to use for normalizing brightness.  "
                         "Options are (sampled_mean): the image is normalized using a mean value "
                         "calculated from a uniform sampling of the pixels  "
                         "(median): the image ins normalized by median of the pixel values." );

  // Image smoothing settings, used if enabled
  config_.add_parameter( "smoothing_std_dev",
                         "0.6",
                         "Std dev for internal gaussian smoothing" );
  config_.add_parameter( "smoothing_half_width",
                         "2",
                         "Half width for internal gaussian smoothing" );

  // Auto-white balancing settings
  config_.add_parameter( "white_scale_factor",
                         "0.90",
                         "A measure of how much to over or under correct white "
                         "reference points. A value near 1.0 will attempt to make "
                         "the whitest thing in the image very close to pure white." );
  config_.add_parameter( "black_scale_factor",
                         "0.70",
                         "A measure of how much to over or under correct black "
                         "reference points. A value near 1.0 will attempt to make "
                         "the blackest thing in the image very close to pure black." );
  config_.add_parameter( "exp_history_factor",
                         "0.25",
                         "The exponential averaging factor for correction matrices" );
  config_.add_parameter( "correction_matrix_resolution",
                         "8",
                         "The resolution of the correction matrix" );

  // Mean-illumination norm settings
  config_.add_parameter( "sampling_rate",
                         "2",
                         "The sampling rate used when approximating the mean scene illumination." );
  config_.add_parameter( "brightness_history_length",
                         "10",
                         "Attempt to stabilize the brightness using data from the last "
                         "x frames (used for sampled_mean mode)." );
  config_.add_parameter( "min_percent_brightness",
                         "0.10",
                         "The minimum allowed average brightness for an image (used for sampled_mean mode). "
                         "Normalized to [0,1] range so that it is type independent." );
  config_.add_parameter( "max_percent_brightness",
                         "0.90",
                         "The maximum allowed average brightness for an image (used for sampled_mean mode). "
                         "Normalized to [0,1] range so that it is type independent." );

  // Median-illumination norm settings
  config_.add_parameter( "reference_median_mode", "fixed",
                         "Used with normalize_brightness_mode is median.  "
                         "Used control how to calculate the reference median.  "
                         "(fixed) normalizes the illumination using a fixed median value.  "
                         "(first) uses the first frame after reset as the reference median" );
  config_.add_parameter( "fixed_reference_median",
                         boost::lexical_cast<std::string>(std::numeric_limits<PixType>::max()*0.5),
                         "Used when reference_median_mode is fix.  The image is relight to have this median" );
}


template <typename PixType>
video_enhancement_process<PixType>
::~video_enhancement_process()
{
}


template <typename PixType>
config_block
video_enhancement_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
video_enhancement_process<PixType>
::set_params( config_block const& blk )
{
  bool norm_bright = false;

  // Load internal settings
  try
  {
    process_disabled_ = blk.get<bool>("disabled");
    if(!process_disabled_)
    {
      masking_enabled_ = blk.get<bool>("masking_enabled");

      // Load smoothing options
      smoothing_enabled_ = blk.get<bool>("smoothing_enabled");
      if( smoothing_enabled_ )
      {
        std_dev_ = blk.get<double>("smoothing_std_dev");
        half_width_ = blk.get<unsigned>("smoothing_half_width");
      }

      // Load inversion options
      inversion_enabled_ = blk.get<bool>("inversion_enabled");

      // Load white balancing options
      awb_enabled_ = blk.get<bool>("auto_white_balance");
      if( awb_enabled_ )
      {
        awb_settings_.white_traverse_factor = blk.get<double>( "white_scale_factor" );
        awb_settings_.black_traverse_factor = blk.get<double>( "black_scale_factor" );
        awb_settings_.exp_averaging_factor  = blk.get<double>( "exp_history_factor" );
        awb_settings_.correction_matrix_res = blk.get<unsigned>( "correction_matrix_resolution"  );

        // Validate parameters
        if( awb_settings_.exp_averaging_factor > 1.0 )
        {
          throw config_block_parse_error("Invalid exponential averaging weight" );
        }
        if( awb_settings_.correction_matrix_res > 200 )
        {
          throw config_block_parse_error( "Correction matrix resolution is too large!" );
        }

        awb_engine_.configure( awb_settings_ );
      }

      // Load illumination norm options
      norm_bright = blk.get<bool>( "normalize_brightness" );
      if(norm_bright)
      {
        std::string mode = blk.get<std::string>("normalize_brightness_mode");
        if(mode == "sampled_mean")
        {
          illumination_mode_ = MEAN;
          unsigned sampling_rate = blk.get<unsigned>( "sampling_rate" );
          unsigned in_avg_length = blk.get<unsigned>( "brightness_history_length" );
          double per_illum_min = blk.get<double>( "min_percent_brightness" );
          double per_illum_max = blk.get<double>( "max_percent_brightness" );

          if( sampling_rate < 1 )
          {
            throw config_block_parse_error( "Invalid sampling rate!" );
          }

          illum_function_.reset( new mean_illumination_normalization<PixType>( in_avg_length,
                                                                               sampling_rate,
                                                                               per_illum_min,
                                                                               per_illum_max ) );
        }
        else if( mode == "median" )
        {
          illumination_mode_ = MEDIAN;
          std::string median_mode = blk.get<std::string>("reference_median_mode");
          PixType value = blk.get<PixType>("fixed_reference_median");
          illum_function_.reset( new median_illumination_normalization<PixType>(median_mode == "fixed", value) );
        }
        else
        {
          throw config_block_parse_error( "Invalid normalization algorithm string: " + mode );
        }
      }
      else
      {
        illumination_mode_ = NONE;
      }
    }
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


template <typename PixType>
bool
video_enhancement_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
bool
video_enhancement_process<PixType>
::step()
{
  // Perform Basic Validation
  if( !src_image_ )
  {
    LOG_ERROR( this->name() << ": No valid input image provided!" );
    return false;
  }
  if( process_disabled_ || !enable_flag_ )
  {
    output_image_ = *src_image_;
    return true;
  }
  if( masking_enabled_ && !mask_image_ )
  {
    LOG_ERROR( this->name() << ": No valid mask image provided!" );
    return false;
  }

  // Make sure output image is allocated and is seperate from the input
  // (Due to having the enable flag, output_image and src_image could poententially
  // point to the same buffer and we don't want to overwrite the input contents)
  if( *src_image_ == output_image_ )
  {
    output_image_.clear();
  }
  output_image_.deep_copy( *src_image_ );

  // Handle resets
  if( reset_flag_ == true )
  {
    if( awb_enabled_ )
    {
      awb_engine_.reset();
    }
    if( illumination_mode_ != NONE )
    {
      (*illum_function_).reset();
    }
  }

  // Perform any optional inverting
  if( inversion_enabled_ )
  {
    invert_image( output_image_ );
  }

  // Perform any optional smoothing
  if( smoothing_enabled_ )
  {
    vil_gauss_filter_2d( output_image_, output_image_, std_dev_, half_width_ );
  }

  // Perform any optional white balancing
  if( awb_enabled_ && src_image_->nplanes() == 3 )
  {
    awb_engine_.apply( output_image_ );
  }

  // Perform any optional illumination/brightness normalization
  switch(illumination_mode_)
  {
    case MEAN:
    case MEDIAN:
      output_image_ = (*illum_function_)(output_image_);
      break;
    case NONE:
      //DO NOTHING
      break;
    case INVALID:
      LOG_ERROR( this->name() << ": The method for illumination normalization is invalid.  Did you call set_params" );
      return false;
    default:
      LOG_ERROR( this->name() << ": Unknown illumination mode." );
      return false;
  }

  // Reset input variables
  src_image_ = NULL;
  mask_image_ = NULL;
  reset_flag_ = false;
  enable_flag_ = true;

  return true;
}

// Accessor functions
template <typename PixType>
void
video_enhancement_process<PixType>
::set_source_image( vil_image_view<PixType> const& src )
{
  src_image_ = &src;
}

template <typename PixType>
void
video_enhancement_process<PixType>
::set_mask_image( vil_image_view<bool> const& mask )
{
  mask_image_ = &mask;
}

template <typename PixType>
void
video_enhancement_process<PixType>
::set_reset_flag( bool flag )
{
  reset_flag_ = flag;
}

template <typename PixType>
void
video_enhancement_process<PixType>
::set_enable_flag( bool flag )
{
  enable_flag_ = flag;
}

template <typename PixType>
vil_image_view<PixType>
video_enhancement_process<PixType>
::output_image() const
{
  return output_image_;
}

template <typename PixType>
vil_image_view<PixType>
video_enhancement_process<PixType>
::copied_output_image() const
{
  copied_output_image_.clear();
  copied_output_image_.deep_copy( output_image_ );
  return copied_output_image_;
}

} // end namespace vidtk
