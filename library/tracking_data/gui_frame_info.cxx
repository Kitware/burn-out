/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "gui_frame_info.h"

namespace vidtk
{

gui_frame_info
::gui_frame_info()
{
  this->reset_variables();
}

gui_frame_info
::gui_frame_info( const gui_track_signal& sig )
{
  this->reset_variables();
  this->add_signal( sig );
}

void
gui_frame_info
::reset_variables()
{
  frame_signals_.clear();
  frames_under_realtime_ = 0;
  frames_until_signal_ = 0;
  recommended_action_ = PROCESS;
  temporal_discontinuity_flag_ = false;
}

gui_track_signal_list const&
gui_frame_info
::frame_signals() const
{
  return frame_signals_;
}

gui_frame_info::recommended_action
gui_frame_info
::action() const
{
  return recommended_action_;
}

bool
gui_frame_info
::has_frame_signals() const
{
  return !frame_signals_.empty();
}

bool
gui_frame_info
::temporal_discontinuity_flag() const
{
  return temporal_discontinuity_flag_;
}

unsigned
gui_frame_info
::frames_under_realtime() const
{
  return frames_under_realtime_;
}

unsigned
gui_frame_info
::frames_until_next_signal() const
{
  return frames_until_signal_;
}

void
gui_frame_info
::add_signal( const gui_track_signal& sig )
{
  frame_signals_.push_back( sig );
}

void
gui_frame_info
::set_action( gui_frame_info::recommended_action rec_action )
{
  recommended_action_ = rec_action;
}

void
gui_frame_info
::set_temporal_discontinuity_flag( const bool flag )
{
  temporal_discontinuity_flag_ = flag;
}

void
gui_frame_info
::set_frames_until_signal( const unsigned count )
{
  frames_until_signal_ = count;
}

void
gui_frame_info
::set_frames_under_realtime( const unsigned count )
{
  frames_under_realtime_ = count;
}

std::ostream& operator<<( std::ostream& str, const gui_frame_info& obj )
{
  str << "GUI track signals on this frame: " << obj.frame_signals().size() << " ";
  str << "Recommended action: " << static_cast<int>( obj.action() );
  return str;
}

}
