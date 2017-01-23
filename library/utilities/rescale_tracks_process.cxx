/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "rescale_tracks_process.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "rescale_tracks_process_cxx" );

rescale_tracks_process
::rescale_tracks_process( std::string const& _name )
  : process( _name, "rescale_tracks_process" )
{
  config_.add_parameter( "disabled",
    "false",
    "Should this process be disabled and pass long it's input?" );

  config_.merge( rescale_tracks_settings().config() );
}


rescale_tracks_process
::~rescale_tracks_process()
{
}


config_block
rescale_tracks_process
::params() const
{
  return config_;
}


bool
rescale_tracks_process
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );

    if( !disabled_ )
    {
      parameters_.read_config( blk );
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
rescale_tracks_process
::initialize()
{
  inputs_ = track::vector_t();
  return true;
}


bool
rescale_tracks_process
::reset()
{
  return initialize();
}


bool
rescale_tracks_process
::step()
{
  // If disabled pass along inputs
  if( disabled_ )
  {
    outputs_ = inputs_;
    return true;
  }

  // Run algorithm
  rescale_tracks( inputs_, parameters_, outputs_ );

  // Reset input vector
  inputs_ = track::vector_t();
  return true;
}


void
rescale_tracks_process
::set_input_tracks( track::vector_t const& trks )
{
  inputs_ = trks;
}


track::vector_t
rescale_tracks_process
::output_tracks() const
{
  return outputs_;
}


} // end namespace vidtk
