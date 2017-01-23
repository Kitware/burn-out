/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "interpolate_corners_from_shift_process.h"

#include <logger/logger.h>

VIDTK_LOGGER( "interpolate_corners_from_shift_process_txx" );

namespace vidtk
{

template< class PixType >
interpolate_corners_from_shift_process< PixType >
::interpolate_corners_from_shift_process( std::string const& _name )
  : process( _name, "interpolate_corners_from_shift_process" ),
    disabled_( false )
{
  config_.add_parameter( "disabled",
    "false",
    "Do not do anything." );

  config_.merge( interpolate_corners_settings().config() );
}

template< class PixType >
interpolate_corners_from_shift_process< PixType >
::~interpolate_corners_from_shift_process()
{
}

template< class PixType >
config_block
interpolate_corners_from_shift_process< PixType >
::params() const
{
  return config_;
}

template< class PixType >
bool
interpolate_corners_from_shift_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    this->disabled_ = blk.get< bool >( "disabled" );

    if( !this->disabled_ )
    {
      interpolate_corners_settings algorithm_settings( blk );
      algorithm_.configure( algorithm_settings );
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}

template< class PixType >
bool
interpolate_corners_from_shift_process< PixType >
::initialize()
{
  return true;
}


template< class PixType >
bool
interpolate_corners_from_shift_process< PixType >
::step()
{
  out_meta_vec_ = in_meta_vec_;

  // Simply pass along data if disabled.
  if( disabled_ )
  {
    return true;
  }

  for( unsigned i = 0; i < out_meta_vec_.size(); ++i )
  {
    algorithm_.process_packet( image_.ni(), image_.nj(), homog_, ts_, out_meta_vec_[i] );
  }

  return true;
}

template< class PixType >
void
interpolate_corners_from_shift_process< PixType >
::set_input_image( vil_image_view<PixType> const& img )
{
  image_ = img;
}

template< class PixType >
void
interpolate_corners_from_shift_process< PixType >
::set_input_timestamp( timestamp const& ts )
{
  ts_ = ts;
}

template< class PixType >
void
interpolate_corners_from_shift_process< PixType >
::set_input_metadata_vector( video_metadata::vector_t const& md )
{
  in_meta_vec_ = md;
}

template< class PixType >
void
interpolate_corners_from_shift_process< PixType >
::set_input_homography( image_to_image_homography const& h )
{
  homog_ = h;
}

template< class PixType >
video_metadata::vector_t
interpolate_corners_from_shift_process< PixType >
::output_metadata_vector() const
{
  return out_meta_vec_;
}

} // end namespace vidtk
