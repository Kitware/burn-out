/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "wh_tracker_kalman.h"

#include <vnl/vnl_inverse.h>

#include <vcl_algorithm.h>

namespace vidtk
{

wh_tracker_kalman
::wh_tracker_kalman(const_parameters_sptr kp)
  : da_so_tracker_kalman(kp)
{
  // Observation matrix
  H_.fill( 0.0 );
  H_(0,0) = 1;
  H_(1,1) = 1;

  F_.set_identity();
  F_(0,2) = 1.0;
  F_(1,3) = 1.0;

  wh_init_dw_=kp->init_dw_;
  wh_init_dh_=kp->init_dh_;
}

wh_tracker_kalman
::~wh_tracker_kalman()
{
  params_ = NULL;
}

void
wh_tracker_kalman
::get_current_width( double & l ) const
{
  l = x_[state_elements::width];
}
void
wh_tracker_kalman
::get_current_height( double & l ) const
{
  l = x_[state_elements::height];
}
void
wh_tracker_kalman
::get_current_wh( vnl_double_2 & l ) const
{
  l[0] = x_[state_elements::width];
  l[1] = x_[state_elements::height];
}

void
wh_tracker_kalman
::get_current_width_dot( double & l ) const
{
  l = x_[state_elements::width_dot];
}

void
wh_tracker_kalman
::get_current_height_dot( double & l ) const
{
  l = x_[state_elements::height_dot];
}

void
wh_tracker_kalman
::get_current_wh_dot( vnl_double_2 & l ) const
{
  l[0] = x_[state_elements::width_dot];
  l[1] = x_[state_elements::height_dot];
}

void
wh_tracker_kalman
::initialize( track const &init_track )
{
  track_state_sptr const& init_state = init_track.last_state();
  track copy_state = init_track;

  image_object_sptr obj;
  double tw= 0.0;
  double th= 0.0;
  int cnt=0;
  for ( unsigned i=0; i< copy_state.history().size() ; i++ )
  {
    obj = copy_state.get_object( i );
    tw += obj->bbox_.width();
    th += obj->bbox_.height();
    cnt++;
  }
  tw /= cnt;
  th /= cnt;

  x_[state_elements::width] = tw;
  x_[state_elements::height] = th;
  x_[state_elements::width_dot] = wh_init_dw_;
  x_[state_elements::height_dot] = wh_init_dh_;


  P_ = params_->init_cov_;
  last_time_ = init_state->time_.time_in_secs();

  track_ = init_track.clone();
}

void
wh_tracker_kalman
::get_current_wh_covar( vnl_double_2x2 & c ) const
{
  c = P_.extract( 2, 2 );
}

da_so_tracker_sptr
wh_tracker_kalman
::clone() const
{
  return new wh_tracker_kalman( *this );
}

} //namespace vidtk

#include <vbl/vbl_smart_ptr.txx>

// Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::wh_tracker_kalman::const_parameters);
