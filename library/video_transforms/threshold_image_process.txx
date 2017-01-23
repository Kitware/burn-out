/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "threshold_image_process.h"

#include <utilities/config_block.h>

#include <video_transforms/zscore_image.h>
#include <video_transforms/percentile_image.h>

#include <vil/vil_convert.h>
#include <vil/algo/vil_threshold.h>

#include <logger/logger.h>

VIDTK_LOGGER("threshold_image_process_txx");

namespace vidtk
{


template <typename PixType>
threshold_image_process<PixType>
::threshold_image_process( std::string const& _name )
  : process( _name, "threshold_image_process" ),
    src_image_( NULL ),
    type_( ABSOLUTE ),
    condition_ ( ABOVE ),
    persist_output_( true )
{
  config_.add_parameter( "persist_output",
                         "false",
                         "If set, this process will return the last good mask if a source"
                         " image was not supplied" );
  config_.add_parameter( "type",
                         "absolute",
                         "Type of thresholding performed. Choose between absolute, "
                         "zscore, adaptive_zscore, and percentiles." );
  config_.add_parameter( "condition",
                         "above",
                         "Threshold condition. Choose between above and below. The output "
                         "binary mask will have 'true' values for pixels with the given "
                         "condition (e.g. for above, pixels above the given threshold will "
                         "have a true value)." );
  config_.add_parameter( "threshold",
                         "0",
                         "The threshold value to use. The metric of the threshold depends "
                         "upon the method used." );
  config_.add_parameter( "z_radius",
                         "200",
                         "Radius of the box in which to compute locally adaptive "
                         "z-scores. For each pixel the zscore statistics are "
                         "computed seperately within a box that is 2*radius+1 on "
                         "a side and centered on the pixel." );
  config_.add_parameter( "percentiles",
                         "",
                         "A comma seperated list of percentile thresholds in the range "
                         "0.0 to 1.0." );
}


template <typename PixType>
threshold_image_process<PixType>
::~threshold_image_process()
{
}


template <typename PixType>
config_block
threshold_image_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
threshold_image_process<PixType>
::set_params( config_block const& blk )
{
  std::string type_string;
  std::string condition_string;

  try
  {
    threshold_ = blk.get<PixType>( "threshold" );
    persist_output_ = blk.get<bool>( "persist_output" );
    z_radius_ = blk.get<int>( "z_radius" );
    type_string = blk.get<std::string>( "type" );
    condition_string = blk.get<std::string>( "condition" );

    //Equating above thresholding to "absolute", to allow for easier interfacing
    //with other processes that perform similar functionality
    if( type_string == "absolute")
    {
      type_ = ABSOLUTE;
    }
    else if( type_string == "zscore" )
    {
      type_ = Z_SCORE;
    }
    else if( type_string == "adaptive_zscore" )
    {
      type_ = ADAPTIVE_Z_SCORE;
    }
    else if( type_string == "percentiles" )
    {
      type_ = PERCENTILES;

      std::string percentile_list;
      percentile_list = blk.get<std::string>( "percentiles" );
      std::stringstream parser( percentile_list );
      double entry;
      while( parser >> entry )
      {
        percentiles_.push_back( entry );
        if( parser.peek() == ',' )
        {
          parser.ignore();
        }
      }
    }
    else
    {
      throw config_block_parse_error( "Invalid type, expecting \"percentiles\","
                                       "\"absolute\", \"zscore\", or \"adaptive_zscore\"." );
    }

    if( condition_string == "above" )
    {
      condition_ = ABOVE;
    }
    else if( condition_string == "below" )
    {
      condition_ = BELOW;
    }
    else
    {
      throw config_block_parse_error( "Invalid condition, expecting "
                                      "\"above\" or \"below\"." );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <typename PixType>
bool
threshold_image_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
bool
threshold_image_process<PixType>
::step()
{
  if( ! src_image_ )
  {
    if( persist_output_ )
    {
      return true;
    }
    else
    {
      LOG_ERROR( this->name() << ": no source image, and not asked to persist!" );
      return false;
    }
  }
  else
  {
    output_image_ = vil_image_view<bool>();

    switch( type_ )
    {
      case ABSOLUTE:
      {
        switch( condition_ )
        {
        case ABOVE:
          vil_threshold_above( *src_image_, output_image_, threshold_ );
          break;
        case BELOW:
          vil_threshold_below( *src_image_, output_image_, threshold_ );
          break;
        }
        break;
      }
      case PERCENTILES:
      {
        switch( condition_ )
        {
        case ABOVE:
          percentile_threshold_above( *src_image_, percentiles_, output_image_ );
          break;
        case BELOW:
          LOG_ERROR( this->name() << ": percentiles below thresholding not yet implemented!" );
          return false;
          break;
        }
        break;
      }
      case Z_SCORE:
      {
        switch( condition_ )
        {
        case ABOVE:
          zscore_threshold_above( *src_image_, output_image_, threshold_ );
          break;
        case BELOW:
          LOG_ERROR( this->name() << ": z_score below thresholding not yet implemented!" );
          return false;
          break;
        }
        break;
      }
      case ADAPTIVE_Z_SCORE:
      {
        switch( condition_ )
        {
        case ABOVE:
        {
          vil_image_view<float> z_score;
          vil_convert_cast( *src_image_, z_score );
          zscore_local_box( z_score, z_radius_ );
          vil_threshold_above( z_score, output_image_, static_cast<float>(threshold_) );
        }
        break;
        case BELOW:
          LOG_ERROR( this->name() << ": adaptive z_score below thresholding not yet implemented!" );
          return false;
          break;
        }
        break;
      }
    }

    // Mark the image as "used", so that we don't try to use it again
    // the next time.
    src_image_ = NULL;

    return true;
  }
}


template <typename PixType>
void
threshold_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& src )
{
  src_image_ = &src;
}


template <typename PixType>
vil_image_view<bool>
threshold_image_process<PixType>
::thresholded_image() const
{
  return output_image_;
}


} // end namespace vidtk


#define INSTANTIATE_THRESHOLD_IMAGE_PROCESS( PixType )                  \
  template                                                              \
  class vidtk::threshold_image_process< PixType >;
