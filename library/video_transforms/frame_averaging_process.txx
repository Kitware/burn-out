/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "frame_averaging_process.h"

#include <utilities/config_block.h>

#include <logger/logger.h>


VIDTK_LOGGER("frame_averaging_process_txx");


namespace vidtk
{


// Process Definition
template <typename PixType, typename BufferType>
frame_averaging_process<PixType, BufferType>
::frame_averaging_process( std::string const& _name )
  : process( _name, "frame_averaging_process" ),
    src_image_( NULL ),
    reset_flag_( false ),
    estimate_variance_( false )
{
  config_.add_parameter( "type",
                         "window",
                         "Operating mode of this filter, possible values: cumulative, window,"
                         " or exponential." );
  config_.add_parameter( "compute_variance",
                         "false",
                         "If set, will compute an estimated variance for each pixel which "
                         "will be outputted as a seperate (double) image on a seperate port." );
  config_.add_parameter( "window_size",
                         "10",
                         "The window size if computing a windowed moving average." );
  config_.add_parameter( "exp_weight",
                         "0.3",
                         "Exponential averaging coefficient if computing an exp average." );
  config_.add_parameter( "round",
                         "false",
                         "Should we spend a little extra time rounding when possible?" );
}


template <typename PixType, typename BufferType>
frame_averaging_process<PixType, BufferType>
::~frame_averaging_process()
{
}


template <typename PixType, typename BufferType>
config_block
frame_averaging_process<PixType, BufferType>
::params() const
{
  return config_;
}


template <typename PixType, typename BufferType>
bool
frame_averaging_process<PixType, BufferType>
::set_params( config_block const& blk )
{
  // Load internal settings
  std::string type_string;
  bool should_round;

  try
  {
    type_string = blk.get< std::string >( "type" );
    estimate_variance_ = blk.get< bool >( "compute_variance" );
    should_round = blk.get< bool >( "round" );

    // Possible functions for this node
    if( type_string == "window")
    {
      unsigned moving_window_size = blk.get< unsigned >( "window_size" );

      averager_.reset( new windowed_frame_averager< PixType >( should_round, moving_window_size ) );
    }
    else if( type_string == "cumulative" )
    {
      averager_.reset( new cumulative_frame_averager< PixType >( should_round ) );
    }
    else if( type_string == "exponential" )
    {
      double exp_weight = blk.get< double >( "exp_weight" );

      if( exp_weight <= 0 || exp_weight >= 1 )
      {
        throw config_block_parse_error( "Invalid exponential averaging coefficient!" );
      }

      averager_.reset( new exponential_frame_averager< PixType >( should_round, exp_weight ) );
    }
    else
    {
      throw config_block_parse_error( "Invalid averaging type!" );
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


template <typename PixType, typename BufferType>
bool
frame_averaging_process<PixType, BufferType>
::initialize()
{
  return true;
}


template <typename PixType, typename BufferType>
bool
frame_averaging_process<PixType, BufferType>
::step()
{
  // Perform Basic Validation
  if( !src_image_ )
  {
    LOG_ERROR( this->name() << ": No valid input image provided!" );
    return false;
  }

  // Handle reset case
  if( reset_flag_ )
  {
    averager_->reset();
  }

  // Reset output images
  output_avg_image_ = vil_image_view< PixType >();

  // Perform update
  if( !estimate_variance_ )
  {
    averager_->process_frame( *src_image_,
                              output_avg_image_ );
  }
  else
  {
    output_var_image_ = vil_image_view< double >();
    averager_->process_frame( *src_image_,
                              output_avg_image_,
                              output_var_image_ );
  }

  // Reset input variables
  src_image_ = NULL;
  reset_flag_ = false;
  return true;
}


// Set internal states / get outputs of the process
template <typename PixType, typename BufferType>
void
frame_averaging_process<PixType, BufferType>
::set_source_image( vil_image_view<PixType> const& src )
{
  src_image_ = &src;
}

template <typename PixType, typename BufferType>
void
frame_averaging_process<PixType, BufferType>
::set_reset_flag( bool flag )
{
  reset_flag_ = flag;
}

template <typename PixType, typename BufferType>
vil_image_view<PixType>
frame_averaging_process<PixType, BufferType>
::averaged_image() const
{
  return output_avg_image_;
}

template <typename PixType, typename BufferType>
vil_image_view<double>
frame_averaging_process<PixType, BufferType>
::variance_image() const
{
  return output_var_image_;
}

} // end namespace vidtk
