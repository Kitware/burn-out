/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "da_so_tracker_generator_process.h"

#include <tracking/da_so_tracker_kalman.h>
#include <tracking/tracking_keys.h>

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{

da_so_tracker_generator_process
::da_so_tracker_generator_process( vcl_string name )
  : process( name, "da_so_tracker_generator_process" ),
    disabled_(false), new_trks_(NULL)
{
  config_.add( "disabled", "false" );
  config_block blk1;
  generator_.add_default_parameters( blk1 );
  config_.add_subblock( blk1, "kinematics_filter" );
  config_block blk2;
  wh_generator_.add_default_parameters( blk2 );
  config_.add_subblock( blk2, "wh_filter" );
  config_.add_parameter( "wh_filter:disabled", "true",
    "Whether or not to use the box width and height filter." );
}

da_so_tracker_generator_process
::~da_so_tracker_generator_process()
{
}

config_block
da_so_tracker_generator_process
::params() const
{
  return config_;
}

bool
da_so_tracker_generator_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", disabled_ );
    if(!disabled_)
    {
      if(!generator_.update_params(blk.subblock("kinematics_filter")))
      {
        return false;
      }
      blk.get( "wh_filter:disabled", wh_disabled_ );

      if(!wh_disabled_ && !wh_generator_.update_params(blk.subblock("wh_filter")))
      {
        return false;
      }
    }
  }
  catch( unchecked_return_value& e)
  {
    log_error( name() << ": couldn't set parameters: "<< e.what() <<"\n" );
    return false;
  }

  config_.update( blk );
  return true;
}

bool
da_so_tracker_generator_process
::initialize()
{
  new_trks_ = NULL;
  trackers_.clear();
  wh_trackers_.clear();
  return true;
}

bool
da_so_tracker_generator_process
::reset()
{
  return this->initialize();
}

bool
da_so_tracker_generator_process
::step()
{
  if( disabled_ ) return true;
  trackers_.clear();
  wh_trackers_.clear();
  if(!new_trks_) return true;
    
  for( unsigned int i = 0; i < new_trks_->size(); ++i )
  {
    trackers_.push_back(generator_.generate((*new_trks_)[i]));
    if (!wh_disabled_)
    {
      wh_trackers_.push_back(wh_generator_.generate((*new_trks_)[i]));
    }
  }
  new_trks_ = NULL;
  return true;
}

void
da_so_tracker_generator_process
::set_new_tracks( vcl_vector< track_sptr > const& tracks )
{
  new_trks_ = &tracks;
}

vcl_vector< da_so_tracker_sptr > const&
da_so_tracker_generator_process
::new_trackers() const
{
  return trackers_;
}

vcl_vector< da_so_tracker_sptr > const&
da_so_tracker_generator_process
::new_wh_trackers() const
{
  return wh_trackers_;
}



} //namespace vidtk
