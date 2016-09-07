/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "da_so_tracker_kalman.h"

#include <vnl/vnl_inverse.h>

#include <vcl_algorithm.h>

namespace vidtk
{

da_so_tracker_kalman
::da_so_tracker_kalman(const_parameters_sptr kp)
  : da_so_tracker(), 
    params_(kp)
{
  // Observation matrix
  H_.fill( 0.0 );
  H_(0,0) = 1;
  H_(1,1) = 1;

  F_.set_identity();
  F_(0,2) = 1.0;
  F_(1,3) = 1.0;
}

da_so_tracker_kalman
::~da_so_tracker_kalman()
{
  params_ = NULL;
}

void
da_so_tracker_kalman
::set_state( vnl_vector_fixed<double,4> state,
             vnl_matrix_fixed<double,4,4> covar,
             double time )
{
  x_ = state;
  P_ = covar;
  last_time_ = time;
}

vnl_vector_fixed<double,4> const&
da_so_tracker_kalman
::state() const
{
  return x_;
}


vnl_matrix_fixed<double,4,4> const&
da_so_tracker_kalman
::state_covar() const
{
  return P_;
}

void
da_so_tracker_kalman
::get_current_location( vnl_double_2 & l ) const
{
  l[0] = x_[state_elements::loc_x];
  l[1] = x_[state_elements::loc_y];
}

void
da_so_tracker_kalman
::get_current_velocity( vnl_double_2 & v ) const
{
  v[0] = x_[state_elements::vel_x];
  v[1] = x_[state_elements::vel_y];
}

void
da_so_tracker_kalman
::get_current_location( vnl_vector_fixed< double, 3 > & l ) const
{
  l[0] = x_[state_elements::loc_x];
  l[1] = x_[state_elements::loc_y];
  l[2] = 0;
}

void
da_so_tracker_kalman
::get_current_velocity( vnl_vector_fixed< double, 3 > & v ) const
{
  v[0] = x_[state_elements::vel_x];
  v[1] = x_[state_elements::vel_y];
  v[2] = 0;
}

void
da_so_tracker_kalman
::initialize( track const & init_track )
{
  track_state_sptr const& init_state = init_track.last_state();
  x_[state_elements::loc_x] = init_state->loc_[0];
  x_[state_elements::loc_y] = init_state->loc_[1];
  x_[state_elements::vel_x] = init_state->vel_[0];
  x_[state_elements::vel_y] = init_state->vel_[1];

  P_ = params_->init_cov_;
  last_time_ = init_state->time_.time_in_secs();

  track_ = init_track.clone();
}

void
da_so_tracker_kalman
::update_impl( double const& ts,
               vnl_vector_fixed<double,2> const& loc,
               vnl_matrix_fixed<double,2,2> const& cov )
{
  // Equations straight out of wikipedia
  // http://en.wikipedia.org/wiki/Kalman_filter

  // Predict
  vnl_double_4 pred_x;
  vnl_double_4x4 pred_P;
  this->predict( ts, pred_x, pred_P );

  // Update

  vnl_double_2 y = loc - vnl_double_2( pred_x(state_elements::loc_x), pred_x(state_elements::loc_y) );
  vnl_double_2x2 S = pred_P.extract( 2, 2 ) + cov;
  vnl_matrix_fixed<double,4,2> K = pred_P * H_.transpose() * vnl_inverse( S );

  vnl_double_4x4 eye;
  eye.set_identity();

  x_ = pred_x + K * y;
  P_ = (eye - K * H_) * pred_P;

  last_time_ = ts;
}

void
da_so_tracker_kalman
::predict( double const& pred_ts,
           vnl_vector_fixed<double,2>& pred_pos,
           vnl_matrix_fixed<double,2,2>& pred_cov ) const
{
  vnl_double_4 pred_x;
  vnl_double_4x4 pred_P;

  this->predict( pred_ts, pred_x, pred_P );

  pred_pos(0) = pred_x(state_elements::loc_x);
  pred_pos(1) = pred_x(state_elements::loc_y);

  // can replace in a couple of months with
  //   pred_P_.extract( pred_cov );
  pred_cov = pred_P.extract(2,2);
}


void
da_so_tracker_kalman
::predict( double const& pred_ts,
           vnl_vector_fixed<double,4>& state,
           vnl_matrix_fixed<double,4,4>& covar ) const
{
  // This is an extension to the standard Kalman filter formulation
  // that allows for non-unit time steps.  Not sure that the process
  // noise is being handled correctly, though.
  double delta = pred_ts - last_time_;

  F_(0,2) = delta;
  F_(1,3) = delta;

  state = F_ * x_;

  covar = F_ * P_ * F_.transpose();

  if( params_->use_fixed_process_noise_ )
  {
    covar += params_->process_cov_;
  }
  else
  {
    //use current velocity to update the process noise Q.
    vnl_double_2x2 P;
    vnl_double_2x2 D(0.);
    double s = vcl_sqrt( state(state_elements::vel_y)*state(state_elements::vel_y)+
                         state(state_elements::vel_x)*state(state_elements::vel_x));
    P(0,0) =  state(state_elements::vel_x)/s;
    P(1,0) =  state(state_elements::vel_y)/s;
    P(0,1) = -state(state_elements::vel_y)/s;
    P(1,1) =  state(state_elements::vel_x)/s;
    D(0,0) = s*params_->process_noise_speed_scale_[0];
    D(1,1) = vcl_min( (1./s)*params_->process_noise_speed_scale_[1],
                      s*params_->process_noise_speed_scale_[1] );
    vnl_double_2x2 vel_cov = P*D*P.transpose();
    vnl_double_4x4 Qv = params_->process_cov_;
    Qv.update(params_->process_cov_.extract(2,2,2,2) + vel_cov, 2,2);
    covar += Qv;
  }
}

void
da_so_tracker_kalman
::get_current_location_covar( vnl_double_2x2 & c ) const
{
  c = P_.extract( 2, 2 );
}

da_so_tracker_sptr 
da_so_tracker_kalman
::clone() const
{
  return new da_so_tracker_kalman( *this );
}

void
da_so_tracker_kalman::const_parameters
::add_parameters_defaults(config_block & blk)
{
  blk.add_parameter( "process_noise_covariance", 
                     "0 0 0 0  0 0 0 0  0 0 0.001 0  0 0 0 0.001",
                     "The process noise" );
  blk.add_parameter( "initial_state_covariance",
                     "1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1",
                     "The initial corvariance for kalman tracking" );
  blk.add_parameter( "use_fixed_process_noise", "true",
                     "True: use a fixed process noice.  "
                     "False: use a variable process noise" );
  blk.add_parameter( "process_noise_speed_scale", "1.0 1.0",
                     "A multipler appled to the velocity used when "
                     "use_fixed_process_noise is false" );
  blk.add_parameter( "init_dw", "0.0",
                     "Initial bbox width change speed" );
  blk.add_parameter( "init_dh", "0.0",
                     "Initial bbox height change speed" );
}

void
da_so_tracker_kalman::const_parameters
::set_parameters(config_block const & blk)
{
  //vnl_double_4x4 init_cov_;
  blk.get( "initial_state_covariance", init_cov_ );
  //vnl_double_4x4 process_cov_;
  blk.get( "process_noise_covariance", process_cov_ );
  //bool use_fixed_process_noise_
  use_fixed_process_noise_ = blk.get<bool>("use_fixed_process_noise");
  process_noise_speed_scale_ = blk.get<vnl_double_2>("process_noise_speed_scale");

  init_dw_=blk.get<double>("init_dw");
  init_dh_=blk.get<double>("init_dh");
  //vnl_double_2 process_noise_speed_scale_
}

da_so_tracker_kalman::const_parameters
::const_parameters()
{
  init_cov_.set_identity();
  process_cov_.set_identity();
  use_fixed_process_noise_ = true;
  process_noise_speed_scale_ = vnl_double_2(1.,1.);
}

} //namespace vidtk

#include <vbl/vbl_smart_ptr.hxx>

// Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::da_so_tracker_kalman::const_parameters);
