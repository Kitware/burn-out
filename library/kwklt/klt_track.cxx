/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_track.h"

#include <vector>

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

klt_track::klt_track(point_t pt, klt_track_ptr tail)
  : pt_(pt)
  , tail_(tail)
  , size_(tail ? tail->size() + 1 : 1)
{
}

}

bool operator==(vidtk::klt_track::point_t p1, vidtk::klt_track::point_t p2)
{
  return (p1.x == p2.x)
      && (p1.y == p2.y);
}

bool operator==(vidtk::klt_track const& t1, vidtk::klt_track const& t2)
{
  if (t1.is_start() && t2.is_start())
  {
    return true;
  }
  if (t1.is_start() || t2.is_start())
  {
    return false;
  }
  return (t1.size() == t2.size())
      && (t1.point() == t2.point())
      && (*t1.tail() == *t2.tail());
}
