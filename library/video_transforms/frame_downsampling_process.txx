/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "frame_downsampling_process.h"

#include <sstream>

namespace vidtk
{

template< typename PixType >
frame_downsampling_process< PixType >
::frame_downsampling_process( std::string const& _name )
  : downsampling_process( _name ),
    retain_metadata_frames_( false )
{
  params_.add_parameter( "retain_metadata_frames",
                         "false",
                         "Should we not drop any frames containing metadata?" );
}

template< typename PixType >
bool
frame_downsampling_process< PixType >
::set_params( vidtk::config_block const& blk )
{
  try
  {
    retain_metadata_frames_ = blk.get<bool>( "retain_metadata_frames" );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  vidtk::config_block base_configs = blk;

  base_configs.remove( params_ );

  return downsampling_process::set_params( base_configs );
}

template< typename PixType >
vidtk::config_block
frame_downsampling_process< PixType >
::params() const
{
  vidtk::config_block unioned_block;
  unioned_block.merge( params_ );
  unioned_block.merge( downsampling_process::params() );
  return unioned_block;
}

template< typename PixType >
vidtk::timestamp
frame_downsampling_process< PixType >
::current_timestamp()
{
  return timestamp_data;
}

template< typename PixType >
bool
frame_downsampling_process< PixType >
::is_frame_critical()
{
  return ( !in_burst_break() && retain_metadata_frames_ && metadata_data.is_valid() );
}

}
