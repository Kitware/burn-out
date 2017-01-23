/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "shot_break_flags_process.h"

#include <tracking_data/tracking_keys.h>

namespace vidtk
{

shot_break_flags_process
::shot_break_flags_process( std::string const& _name )
  : process( _name, "shot_break_flags_process" ),
    got_data( false )
{
  this->initialize();
}


shot_break_flags_process
::~shot_break_flags_process()
{
}


config_block
shot_break_flags_process
::params() const
{
  return config_block();
}


bool
shot_break_flags_process
::set_params( config_block const& /*blk*/ )
{
  return true;
}


bool
shot_break_flags_process
::initialize()
{
  is_tracker_sb = false;
  is_homog_sb = false;

  return true;
}


bool
shot_break_flags_process
::reset()
{
  return this->initialize();
}


bool
shot_break_flags_process
::step()
{
  if ( !got_data )
  {
    return false;
  }

  flags_ = shot_break_flags();

  if (is_tracker_sb || is_homog_sb)
  {
    flags_.set_shot_end();
  }
  else
  {
    flags_.set_frame_usable();
  }

  is_tracker_sb = false;
  is_homog_sb = false;
  got_data = false;

  return true;
}


void
shot_break_flags_process
::set_is_tracker_shot_break( bool t_sb )
{
  is_tracker_sb = t_sb;
  got_data = true;
}


void
shot_break_flags_process
::set_is_homography_shot_break( bool h_sb )
{
  is_homog_sb = h_sb;
  got_data = true;
}


shot_break_flags
shot_break_flags_process
::flags() const
{
  return flags_;
}


} // end namespace vidtk
