/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef tracker_cost_function_h
#define tracker_cost_function_h

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

#include <vcl_limits.h>
#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <tracking/da_so_tracker.h>
#include <tracking/track.h>
#include <tracking/image_object.h>

#include <utilities/timestamp.h>

namespace vidtk
{

class tracker_cost_function : public vbl_ref_count
{
public:

  tracker_cost_function()
    : INF_(vcl_numeric_limits<double>::infinity())
  {}

  virtual ~tracker_cost_function()
  {}
  
  virtual void set( da_so_tracker_sptr tracker, track_sptr track, 
                    timestamp const* cur_ts ) = 0;

  virtual double cost( image_object_sptr obj ) = 0;

  double operator()(image_object_sptr obj)
  { return this->cost(obj); }

  void operator()( da_so_tracker_sptr tracker, 
                   track_sptr track, 
                   timestamp const* cur_ts )
  {
    this->set(tracker, track, cur_ts);
  }

  double inf() const
  { return INF_; }

  virtual void set_location(unsigned int, unsigned int) 
  { /*NO OPERATION*/ }

  virtual void set_size(unsigned int, unsigned int) 
  { /*NO OPERATION*/ }

  virtual void output_training(unsigned int, unsigned int, vcl_ofstream & )
  { /*NO OPERATION*/ }

protected:
  const double INF_;
};

typedef vbl_smart_ptr<tracker_cost_function> tracker_cost_function_sptr;

} //namespace vidtk

#endif
