/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "filter_tracks_process.h"

#include <vcl_algorithm.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{


filter_tracks_process
::filter_tracks_process( vcl_string const& name )
  : process( name, "filter_tracks_process" )
{
  config_.add( "min_speed", "-1" );
  config_.add( "max_speed", "-1" );
}


filter_tracks_process
::~filter_tracks_process()
{
}


config_block
filter_tracks_process
::params() const
{
  return config_;
}


bool
filter_tracks_process
::set_params( config_block const& blk )
{
  try
  {
    double v;

    blk.get( "min_speed", v );
    min_speed_sqr_ = v >= 0.0 ? v*v : -1;

    blk.get( "max_speed", v );
    max_speed_sqr_ = v >= 0.0 ? v*v : -1;
  }
  catch( unchecked_return_value& )
  {
    log_error( name() << ": couldn't set parameters" );
    // reset to old values
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
filter_tracks_process
::initialize()
{
  return true;
}


bool
filter_tracks_process
::step()
{
  log_assert( src_trks_ != NULL, "source tracks not provided" );

  out_trks_ = *src_trks_;
  vcl_vector< track_sptr >::iterator begin = out_trks_.begin();
  vcl_vector< track_sptr >::iterator end = out_trks_.end();

  if( min_speed_sqr_ >= 0.0 )
  {
    end = vcl_remove_if( begin, end,
                         boost::bind( &self_type::filter_out_min_speed, this, _1 ) );
  }

  if( max_speed_sqr_ >= 0.0 )
  {
    end = vcl_remove_if( begin, end,
                         boost::bind( &self_type::filter_out_max_speed, this, _1 ) );
  }

  // Actually remove the filtered out elements.
  out_trks_.erase( end, out_trks_.end() );

  src_trks_ = NULL;

  return true;
}


void
filter_tracks_process
::set_source_tracks( vcl_vector< track_sptr > const& trks )
{
  src_trks_ = &trks;
}


vcl_vector< track_sptr > const&
filter_tracks_process
::tracks() const
{
  return out_trks_;
}


bool
filter_tracks_process
::filter_out_min_speed( track_sptr const& trk ) const
{
  typedef vcl_vector< track_state_sptr >::const_iterator iter_type;

  iter_type const end = trk->history().end();

  for( iter_type it = trk->history().begin(); it != end; ++it )
  {
    if( (*it)->vel_.squared_magnitude() < min_speed_sqr_ )
    {
      return true;
    }
  }

  return false;
}
bool
filter_tracks_process
::filter_out_max_speed( track_sptr const& trk ) const
{
  typedef vcl_vector< track_state_sptr >::const_iterator iter_type;

  iter_type const end = trk->history().end();

  for( iter_type it = trk->history().begin(); it != end; ++it )
  {
    if( (*it)->vel_.squared_magnitude() > max_speed_sqr_ )
    {
      return true;
    }
  }

  return false;
}


} // end namespace vidtk
