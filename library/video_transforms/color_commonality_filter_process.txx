/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "color_commonality_filter_process.h"

#include <utilities/point_view_to_region.h>
#include <utilities/config_block.h>

#include <vil/algo/vil_gauss_filter.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("color_commonality_filter_process");

// Process Definition
template< class InputType, class OutputType >
color_commonality_filter_process< InputType, OutputType >
::color_commonality_filter_process( std::string const& _name )
  : process( _name, "color_commonality_filter_process" ),
    input_( NULL ),
    border_( NULL ),
    color_resolution_( 512 ),
    color_resolution_per_chan_( 8 ),
    intensity_resolution_( 16 ),
    smooth_image_( false )
{
  config_.add_parameter( "color_hist_resolution_per_chan",
                         "8",
                         "Resolution of the utilized histogram (per channel) if the input "
                         "contains 3 channels." );
  config_.add_parameter( "intensity_hist_resolution",
                         "16",
                         "Resolution of the utilized histogram if the input "
                         "contains 1 channel." );
  config_.add_parameter( "grid_image",
                         "false",
                         "Instead of calculating which colors are more common "
                         "in the entire image, should we do it for smaller evenly "
                         "spaced regions?" );
  config_.add_parameter( "grid_resolution_height",
                         "5",
                         "Divide the height of the image into x regions, if enabled." );
  config_.add_parameter( "grid_resolution_width",
                         "5",
                         "Divide the width of the image into x regions, if enabled." );
  config_.add_parameter( "smooth_image",
                         "false",
                         "Should we smooth the input image before filtering?" );
  config_.add_parameter( "output_scale",
                         "0",
                         "Scale the output image (typically, values start in the range [0,1]) "
                         "by this amount. Enter 0 for type-specific default." );
}

template< class InputType, class OutputType >
color_commonality_filter_process< InputType, OutputType >
::~color_commonality_filter_process()
{
}

template< class InputType, class OutputType >
config_block
color_commonality_filter_process< InputType, OutputType >
::params() const
{
  return config_;
}

template< class InputType, class OutputType >
bool
color_commonality_filter_process< InputType, OutputType >
::initialize()
{
  return true;
}

// Quickly checks if a number is a power of 2
inline bool is_power_of_two(const unsigned num)
{
  return ( (num > 0) && ((num & (num - 1)) == 0) );
}


template< class InputType, class OutputType >
bool
color_commonality_filter_process< InputType, OutputType >
::set_params( config_block const& blk )
{
  // Read config parameters
  try
  {
    color_resolution_per_chan_ = blk.get<unsigned>( "color_hist_resolution_per_chan" );
    intensity_resolution_ = blk.get<unsigned>( "intensity_hist_resolution" );
    smooth_image_ = blk.get<bool>("smooth_image");
    filter_settings_.output_scale_factor = blk.get<unsigned>("output_scale");

    if( !is_power_of_two( color_resolution_per_chan_ ) ||
        !is_power_of_two( intensity_resolution_ ) )
    {
      throw config_block_parse_error( "Specified resolutions must be a power of 2" );
    }

    filter_settings_.grid_image = blk.get<bool>( "grid_image" );

    if( filter_settings_.grid_image )
    {
      filter_settings_.grid_resolution_width = blk.get<unsigned>( "grid_resolution_width" );
      filter_settings_.grid_resolution_height = blk.get<unsigned>( "grid_resolution_height" );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Setup histograms
  color_resolution_ = color_resolution_per_chan_ *
                      color_resolution_per_chan_ *
                      color_resolution_per_chan_;

  color_histogram_.resize( color_resolution_, 0 );
  intensity_histogram_.resize( intensity_resolution_, 0 );

  // Set internal config block
  config_.update( blk );
  return true;
}

template< class InputType, class OutputType >
bool
color_commonality_filter_process< InputType, OutputType >
::step()
{
  // Perform Basic Validation
  if( !input_ )
  {
    LOG_ERROR( this->name() << ": Invalid input images provided!" );
    return false;
  }
  if( input_->nplanes() != 1 && input_->nplanes() != 3 )
  {
    LOG_ERROR( this->name() << ": Invalid number of planes!" );
    return false;
  }

  // Reset output image
  output_image_ = vil_image_view<OutputType>( input_->ni(), input_->nj() );

  // Create an image view without the border if it was specified
  vil_image_view<InputType> input_roi = *input_;
  vil_image_view<OutputType> output_roi = output_image_;

  // Compensate for border
  if( border_ && !border_->is_empty() )
  {
    input_roi = point_view_to_region( *input_, *border_ );
    output_roi = point_view_to_region( output_image_, *border_ );
  }

  // Perform optional smoothing
  if( smooth_image_ )
  {
    vil_image_view<InputType> smoothed;
    vil_gauss_filter_2d( input_roi, smoothed, 0.5, 2 );
    input_roi = smoothed;
  }

  // Perform filtering
  if( input_roi.nplanes() == 1 )
  {
    filter_settings_.resolution_per_channel = intensity_resolution_;
    filter_settings_.histogram = &intensity_histogram_;
    color_commonality_filter( input_roi, output_roi, filter_settings_ );
  }
  else
  {
    filter_settings_.resolution_per_channel = color_resolution_per_chan_;
    filter_settings_.histogram = &color_histogram_;
    color_commonality_filter( input_roi, output_roi, filter_settings_ );
  }

  // Reset input variables
  input_ = NULL;
  border_ = NULL;

  return true;
}

// Accessors
template< class InputType, class OutputType >
void
color_commonality_filter_process< InputType, OutputType >
::set_source_image( vil_image_view<InputType> const& src )
{
  input_ = &src;
}

template< class InputType, class OutputType >
void
color_commonality_filter_process< InputType, OutputType >
::set_source_border( vgl_box_2d<int> const& rect )
{
  border_ = &rect;
}

template< class InputType, class OutputType >
vil_image_view<OutputType>
color_commonality_filter_process< InputType, OutputType >
::filtered_image() const
{
  return output_image_;
}

} // end namespace vidtk
