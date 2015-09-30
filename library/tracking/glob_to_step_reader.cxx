/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "glob_to_step_reader.h"

namespace vidtk
{

glob_to_step_reader
::glob_to_step_reader(track_reader * r, bool d)
  : reader_(r), frame_count_(0), del_here_(d)
{
}

glob_to_step_reader
::~glob_to_step_reader()
{
  if(del_here_ && reader_)
  {
    delete reader_;
  }
  reader_ = NULL;
}

bool
glob_to_step_reader
::read(vcl_vector<track_sptr>& trks)
{
  if(terminated_at_.size() == 0)
  {
    if(!setup_steps())
    {
      return false;
    }
  }
  trks.clear();
  if(current_terminated_ == terminated_at_.end())
  {
    return false;
  }
  if(frame_count_++ == current_terminated_->first)
  {
    trks = current_terminated_->second;
    current_terminated_++;
  }
  return true;
}

bool
glob_to_step_reader
::setup_steps()
{
  if(!reader_)
  {
    return false;
  }
  reader_->set_filename(filename_.c_str());
  reader_->set_ignore_occlusions(ignore_occlusions_);
  reader_->set_ignore_partial_occlusions(ignore_partial_occlusions_);
  terminated_at_.clear();
  frame_count_ = 0;
  vcl_vector<track_sptr> trks;
  reader_->read(trks);
  for( unsigned int i = 0; i < trks.size(); ++i )
  {
    if(!trks[i]->history().size())
    {
      continue;
    }
    terminated_at_[trks[i]->last_state()->time_.frame_number()].push_back(trks[i]);
  }
  current_terminated_ = terminated_at_.begin();
  return true;
}

}
