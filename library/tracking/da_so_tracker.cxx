/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "da_so_tracker.h"
#include <vnl/vnl_double_2x1.h>
#include <vnl/vnl_double_2x2.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_inverse.h>

#include <vcl_algorithm.h>

namespace vidtk
{


da_so_tracker
::da_so_tracker() :
  last_time_( -1 ),
  missed_count_( 0 )
{
}


void
da_so_tracker
::update()
{
  ++missed_count_;
}

void 
da_so_tracker
::update( double const& ts,
          vnl_vector_fixed<double,2> const& loc,
          vnl_matrix_fixed<double,2,2> const& cov )
{
  missed_count_ = 0;
  this->update_impl(ts,loc,cov);
}

void
da_so_tracker
::update( timestamp const& ts, image_object & obj,
          vnl_matrix_fixed<double,2,2> const& cov )
{
  vnl_double_2 loc( obj.world_loc_[0], obj.world_loc_[1] );
  this->update( ts.time_in_secs(), loc, cov );
}


void
da_so_tracker
::update_wh( timestamp const& ts, image_object & obj,
          vnl_matrix_fixed<double,2,2> const& cov )
{
    vnl_double_2 wh( obj.bbox_.width(), obj.bbox_.height() );

  this->update( ts.time_in_secs(), wh, cov );
}

vnl_double_2
da_so_tracker
::get_current_location() const
{
  vnl_double_2 r;
  this->get_current_location(r);
  return r;
}


vnl_double_2
da_so_tracker
::get_current_velocity() const
{
  vnl_double_2 r;
  this->get_current_velocity(r);
  return r;
}


vnl_double_2x2
da_so_tracker
::get_current_location_covar( ) const
{
  vnl_double_2x2 r;
  this->get_current_location_covar(r);
  return r;
}


double
da_so_tracker
::get_current_width() const
{
  double r;
  this->get_current_width(r);
  return r;
}

double
da_so_tracker
::get_current_height() const
{
  double r;
  this->get_current_height(r);
  return r;
}

unsigned
da_so_tracker
::missed_count() const
{
  return missed_count_;
}


double
da_so_tracker
::last_update_time() const
{
  return last_time_;
}


track_sptr
da_so_tracker
::get_track() const
{
  return track_;
}

} // end namespace vidtk


#include <vbl/vbl_smart_ptr.hxx>

// Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::da_so_tracker );
