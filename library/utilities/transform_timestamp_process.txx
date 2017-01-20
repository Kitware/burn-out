/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "transform_timestamp_process.h"

#include <limits>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "transform_timestamp_process" );


template <class PixType>
transform_timestamp_process<PixType>
::transform_timestamp_process( std::string const& _name )
  : process( _name, "transform_timestamp_process" ),
    input_md_( NULL ),
    last_ts_(),
    config_(),
    use_safety_checks_( true )
{
  config_.add_parameter(
    "use_safety_checks",
    "true",
    "Should we enable all safety checks performed on the "
    "timestamp? Should only be disabled for debugging purposes "
    "or when we know a timestamp in some incoming stream could "
    "potentially reset." );
}


template <class PixType>
transform_timestamp_process<PixType>
::~transform_timestamp_process()
{
}


template <class PixType>
config_block
transform_timestamp_process<PixType>
::params() const
{
  config_block blk = generator_.params();
  blk.merge( config_ );
  return blk;
}


template <class PixType>
bool
transform_timestamp_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    use_safety_checks_ = blk.get< bool >( "use_safety_checks" );
    config_block generator_blk = blk;
    generator_blk.remove( config_ );
    generator_.set_params( generator_blk );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( name() << ": " << e.what() );
    return false;
  }
  return true;
}


template <class PixType>
bool
transform_timestamp_process<PixType>
::initialize()
{
  return generator_.initialize();
}

template <class PixType>
process::step_status
transform_timestamp_process<PixType>
::step2()
{
  // If the TS is not valid, that means that we have not received any
  // input since the last step.
  if (! input_ts_.is_valid() )
  {
    return FAILURE;
  }

  if( input_md_ )
  {
    working_md_vect_.push_back( *input_md_ );
    output_md_ = *input_md_;
  }
  else
  {
    output_md_ = video_metadata();
  }

  ts_ = input_ts_;

  step_status p_status = generator_.provide_timestamp( ts_ );

  if( p_status == FAILURE )
  {
    return FAILURE;
  }
  else if( p_status == SUCCESS )
  {
    // Data will be passed downstream only in this case.

    // Sanity check: Time should only increase.
    if( use_safety_checks_
        && ts_.has_time() && last_ts_.time() >= ts_.time() )
    {
      LOG_ERROR( this->name() << " Time should only increase from "
                              << last_ts_ << " to " << ts_ );
      return FAILURE;
    }
    last_ts_ = ts_;

    output_md_vect_ = working_md_vect_;
    working_md_vect_.clear();
  }

  // invalidate input data.
  input_md_ = NULL;
  input_ts_ = vidtk::timestamp();

  return p_status;
}


// ----------------------------------------------------------------
// -- Inputs --

template < class PixType >
void
transform_timestamp_process< PixType >
::set_source_timestamp( vidtk::timestamp const& ts )
{
  input_ts_ = ts;
}


template < class PixType >
void
transform_timestamp_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  img_ = img;
}


template< class PixType >
void transform_timestamp_process< PixType >
::set_source_metadata(video_metadata const& md)
{
  input_md_ = &md;
}


// ----------------------------------------------------------------
// -- Outputs --

template < class PixType >
vidtk::timestamp
transform_timestamp_process< PixType >
::timestamp() const
{
  return ts_;
}


template < class PixType >
vil_image_view< PixType >
transform_timestamp_process< PixType >
::image() const
{
  return img_;
}


template< class PixType >
video_metadata
transform_timestamp_process< PixType >
::metadata() const
{
  return ( output_md_ );
}


template< class PixType >
std::vector< video_metadata >
transform_timestamp_process< PixType >
::metadata_vec() const
{
  return ( output_md_vect_ );
}


} // end namespace vidtk
