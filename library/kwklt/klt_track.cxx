/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_track.h"

#include <vector>
#include <tracking_data/track_util.h>

namespace vidtk
{

klt_track_ptr klt_track::extend_track(point_t pt, klt_track_ptr tail)
{
  return boost::shared_ptr<klt_track>(new klt_track(pt, tail));
}

bool klt_track::is_start() const
{
  return (tail_ == NULL);
}

int klt_track::size() const
{
  return size_;
}

klt_track::point_t klt_track::point() const
{
  return pt_;
}

klt_track_ptr klt_track::tail() const
{
  return tail_;
}

klt_track::klt_track(point_t pt, klt_track_ptr _tail)
  : pt_(pt)
  , tail_(_tail)
  , size_(_tail ? _tail->size() + 1 : 1)
{
}


void klt_track::trim_track( int count )
{
  if ( count <= 1 )
  {
    this->tail_.reset(); // delete track tail
    return;
  }

  if ( this->tail() )
  {
    this->tail_->trim_track( count - 1 );
  }
}


track_sptr convert_from_klt_track(klt_track_ptr trk)
{
  track_sptr vidtk_track(new track);

  std::vector<track_state_sptr> states;

  klt_track_ptr cur_point = trk;

  while (cur_point)
  {
    track_state_sptr state(new track_state);
    vidtk::timestamp ts;

    ts.set_frame_number(cur_point->point().frame);

    state->loc_[0] = cur_point->point().x;
    state->loc_[1] = cur_point->point().y;
    state->loc_[2] = 0;
    state->vel_[0] = 0;
    state->vel_[1] = 0;
    state->vel_[2] = 0;
    state->time_ = ts;

    states.push_back(state);

    cur_point = cur_point->tail();
  }

  vidtk_track->reset_history(states);
  reverse_history( vidtk_track );

  return vidtk_track;
}

} // end namespace vidtk

bool operator==(vidtk::klt_track::point_t p1, vidtk::klt_track::point_t p2)
{
  return (p1.x == p2.x)
      && (p1.y == p2.y);
}

bool operator==(vidtk::klt_track const& t1, vidtk::klt_track const& t2)
{
  return (t1.size() == t2.size())
      && (t1.point() == t2.point())
      && ( ( t1.tail() == t2.tail() ) || (  *t1.tail() == *t2.tail() ) );
}
