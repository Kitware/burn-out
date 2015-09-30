/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "extended_kalman_functions.h"

#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>

namespace vidtk
{

namespace extended_kalman_functions
{

circular_fun
::circular_fun()
{
  params_ = vnl_vector<double>(number_of_state_params,0);
  covar_  = vnl_matrix<double>(number_of_state_params,number_of_state_params,0);
  covar_.fill(0);

  params_[0] = 0;
  params_[1] = 0;
  params_[2] = 0.5;
  params_[3] = 1;
  params_[4] = 0;
}

vnl_vector< double >
circular_fun
::operator()(double d) const
{
  vnl_vector< double > pred(number_of_state_params);
  double cos_t = vcl_cos(params_[loc_theta]);
  double cos_tped = vcl_cos(params_[loc_theta]+params_[loc_eta]*d);
  double sin_t = vcl_sin(params_[loc_theta]);
  double sin_tped = vcl_sin(params_[loc_theta]+params_[loc_eta]*d);
  double r = params_[loc_r];
  pred[loc_x]     = params_[loc_x] + r*(cos_tped-cos_t);
  pred[loc_y]     = params_[loc_y] + r*(sin_tped-sin_t);
  pred[loc_r]     = r;
  pred[loc_theta] = params_[loc_theta]+params_[loc_eta]*d;
  pred[loc_eta]   = params_[loc_eta];
  return pred;
}

vnl_matrix<double>
circular_fun
::jacobian( double d ) const
{
  vnl_matrix<double> j(number_of_state_params,number_of_state_params);
  double cos_t = vcl_cos(params_[loc_theta]);
  double sin_t = vcl_sin(params_[loc_theta]);
  double dtheta_cos_t = -sin_t;
  double dtheta_sin_t =  cos_t;
  double cos_tped = vcl_cos(params_[loc_theta]+params_[loc_eta]*d);
  double sin_tped = vcl_sin(params_[loc_theta]+params_[loc_eta]*d);
  double dtheta_cos_tped = -sin_tped;
  double deta_cos_tped = -d*sin_tped;
  double dtheta_sin_tped = cos_tped;
  double deta_sin_tped = d*cos_tped;
  double r = params_[loc_r];
  j(loc_x,loc_x)=1; j(loc_x,loc_y)=0;
  j(loc_x,loc_r)=(cos_tped-cos_t);
  j(loc_x,loc_theta)=r*(dtheta_cos_tped-dtheta_cos_t);
  j(loc_x,loc_eta)=r*(deta_cos_tped);

  j(loc_y,loc_x)=0; j(loc_y,loc_y)=1;
  j(loc_y,loc_r)=(sin_tped-sin_t);
  j(loc_y,loc_theta)=r*(dtheta_sin_tped-dtheta_sin_t);
  j(loc_y,loc_eta)=r*deta_sin_tped;

  j(loc_r,loc_x)=0; j(loc_r,loc_y)=0; j(loc_r,loc_r)=1;
  j(loc_r,loc_theta)=0; j(loc_r,loc_eta)=0;

  j(loc_theta,loc_x)=0; j(loc_theta,loc_y)=0;
  j(loc_theta,loc_r)=0; j(loc_theta,loc_theta)=1; j(loc_theta,loc_eta)=d;

  j(loc_eta,loc_x)=0; j(loc_eta,loc_y)=0; j(loc_eta,loc_r)=0;
  j(loc_eta,loc_theta)=0;   j(loc_eta,loc_eta)= 1;
  return j;
}

vnl_matrix<double>
circular_fun
::observation_model(vnl_vector<double> /*pred*/)
{
  vnl_matrix<double> H(2,number_of_state_params);
  H.fill( 0.0 );
  H(loc_x,loc_x) = 1;
  H(loc_y,loc_y) = 1;
  return H;
}

vnl_matrix<double> const &
circular_fun
::current_cov() const
{
  return covar_;
}

vnl_vector<double> const &
circular_fun
::current_state() const
{
  return params_;
}

vnl_vector<double>
circular_fun
::to_observation(vnl_vector<double> const & x_pred)const
{
  vnl_vector<double> r(2);
  r[0] = x_pred[loc_x];
  r[1] = x_pred[loc_y];
  return r;
}

void
circular_fun
::set_state(vnl_vector<double> const & new_state)
{
  params_ = new_state;
}

void
circular_fun
::set_cov( vnl_matrix<double> const & new_cov )
{
  covar_ = new_cov;
}

void
circular_fun
::current_location(vnl_double_2 & loc) const
{
  loc[0] = params_[loc_x];
  loc[1] = params_[loc_y];
}

void
circular_fun
::current_location_covar( vnl_double_2x2 & lcov ) const
{
  lcov(0,0) = covar_(loc_x,loc_x); lcov(0,1) = covar_(loc_x,loc_y);
  lcov(1,0) = covar_(loc_y,loc_x); lcov(1,1) = covar_(loc_y,loc_y);
}

void
circular_fun
::current_velocity( vnl_double_2 & v ) const
{
  v[0] = -params_[loc_r]*vcl_sin(params_[loc_theta])*params_[loc_eta];
  v[1] =  params_[loc_r]*vcl_cos(params_[loc_theta])*params_[loc_eta];
}

void
circular_fun
::initialize( track const & /*init_track*/, vnl_matrix<double> const & /*init_covar*/ )
{
  //TODO:  Implement me!!!!
}

void
circular_fun
::state_to_loc( vnl_vector<double> const & state,
                vnl_matrix<double> const & covar,
                vnl_vector_fixed<double,2>& pred_pos,
                vnl_matrix_fixed<double,2,2>& pred_cov) const
{
  pred_pos[0] = state[loc_x];
  pred_pos[1] = state[loc_y];
  pred_cov(0,0) = covar(loc_x,loc_x); pred_cov(0,1) = covar(loc_x,loc_y);
  pred_cov(1,0) = covar(loc_y,loc_x); pred_cov(1,1) = covar(loc_y,loc_y);
}


linear_fun
::linear_fun()
{
  params_ = vnl_vector<double>(number_of_state_params,0);
  covar_  = vnl_matrix<double>(number_of_state_params,number_of_state_params,0);
  covar_.fill(0);
  params_[0] = 0;
  params_[1] = 0;
  params_[2] = 0;
  params_[3] = 1;
}

vnl_vector< double >
linear_fun
::operator()(double ts) const
{
//   vcl_cout << ts << vcl_endl;
  vnl_vector< double > pred(number_of_state_params);
//   vcl_cout << "predict: " << params_ << " ts: " << ts << vcl_endl;
  pred[loc_x] = params_[loc_x] + ts*params_[loc_delta_x];
  pred[loc_y] = params_[loc_y] + ts*params_[loc_delta_y];
  pred[loc_delta_x] = params_[loc_delta_x];
  pred[loc_delta_y] = params_[loc_delta_y];
//   vcl_cout << "result" << pred << vcl_endl;
  return pred;
}

void
linear_fun
::current_velocity( vnl_double_2 & v ) const
{
  v[0] = params_[loc_delta_x];
  v[1] = params_[loc_delta_y];
}

vnl_matrix<double>
linear_fun
::jacobian( double d ) const
{
  vnl_matrix<double> j(number_of_state_params,number_of_state_params);
  j(0,0)=1; j(0,1)=0; j(0,2)=d; j(0,3)=0;
  j(1,0)=0; j(1,1)=1; j(1,2)=0; j(1,3)=d;
  j(2,0)=0; j(2,1)=0; j(2,2)=1; j(2,3)=0;
  j(3,0)=0; j(3,1)=0; j(3,2)=0; j(3,3)=1;
  return j;
}

vnl_matrix<double>
linear_fun
::observation_model(vnl_vector<double> /*pred*/)
{
  vnl_matrix<double> H(2,number_of_state_params);
  H.fill( 0.0 );
  H(loc_x,loc_x) = 1;
  H(loc_y,loc_y) = 1;
  return H;
}

vnl_matrix<double> const &
linear_fun
::current_cov() const
{
  return covar_;
}

vnl_vector<double> const &
linear_fun
::current_state() const
{
  return params_;
}

vnl_vector<double>
linear_fun
::to_observation(vnl_vector<double> const & x_pred)const
{
  vnl_vector<double> r(2);
  r[0] = x_pred[loc_x];
  r[1] = x_pred[loc_y];
  return r;
}

void
linear_fun
::current_location(vnl_double_2 & loc) const
{
  loc[0] = params_[loc_x];
  loc[1] = params_[loc_y];
}

void
linear_fun
::current_location_covar( vnl_double_2x2 & lcov ) const
{
  lcov(loc_x,loc_x) = covar_(loc_x,loc_x); lcov(loc_x,loc_y) = covar_(loc_x,loc_y);
  lcov(loc_y,loc_x) = covar_(loc_y,loc_x); lcov(loc_y,loc_y) = covar_(loc_y,loc_y);
}

void
linear_fun
::state_to_loc( vnl_vector<double> const & state,
                vnl_matrix<double> const & covar,
                vnl_vector_fixed<double,2>& pred_pos,
                vnl_matrix_fixed<double,2,2>& pred_cov) const
{
  pred_pos[0] = state[loc_x];
  pred_pos[1] = state[loc_y];
  pred_cov(0,0) = covar(loc_x,loc_x); pred_cov(0,1) = covar(loc_x,loc_y);
  pred_cov(1,0) = covar(loc_y,loc_x); pred_cov(1,1) = covar(loc_y,loc_y);
}


void
linear_fun
::set_state(vnl_vector<double> const & new_state)
{
  params_ = new_state;
}

void
linear_fun
::set_cov( vnl_matrix<double> const & new_cov )
{
  covar_ = new_cov;
}

void
linear_fun
::initialize( track const & init_track, vnl_matrix<double> const & init_covar )
{
  track_state_sptr const& init_state = init_track.last_state();
  params_[loc_x] = init_state->loc_[0];
  params_[loc_y] = init_state->loc_[1];
  params_[loc_delta_x] = init_state->vel_[0];
  params_[loc_delta_y] = init_state->vel_[1];

  covar_ = init_covar;

}


speed_heading_fun
::speed_heading_fun()
{
  params_ = vnl_vector<double>(number_of_state_params,0);
  covar_  = vnl_matrix<double>(number_of_state_params,number_of_state_params,0);
  covar_.fill(0);

  params_[loc_x] = 0;
  params_[loc_y] = 0;
  params_[loc_heading] = 0.5;
  params_[loc_speed] = 1;
}

vnl_vector< double >
speed_heading_fun
::operator()(double d) const
{
//   vcl_cout << d << vcl_endl;
  vnl_vector< double > pred(number_of_state_params);
//   vcl_cout << "input: " << params_ << " ts: " << d << vcl_endl;
  double cos_t = vcl_cos(params_[loc_heading]);
  double sin_t = vcl_sin(params_[loc_heading]);
  double s = params_[loc_speed];
  pred[loc_x]       = params_[loc_x] + d*s*(cos_t);
  pred[loc_y]       = params_[loc_y] + d*s*(sin_t);
  pred[loc_speed]   = s;
  pred[loc_heading] = params_[loc_heading];
//   vcl_cout << "result: " << pred << " ts: " << d << vcl_endl<<vcl_endl;
  return pred;
}

vnl_matrix<double>
speed_heading_fun
::jacobian( double d ) const
{
  vnl_matrix<double> j(number_of_state_params,number_of_state_params);
  double cos_t = vcl_cos(params_[loc_heading]);
  double sin_t = vcl_sin(params_[loc_heading]);
  double dtheta_cos_t = -sin_t;
  double dtheta_sin_t =  cos_t;
  double s = params_[loc_speed];
  j(loc_x,loc_x)=1; j(loc_x,loc_y)=0;
  j(loc_x,loc_speed)=d*(cos_t);
  j(loc_x,loc_heading)=d*s*(dtheta_cos_t);

  j(loc_y,loc_x)=0; j(loc_y,loc_y)=1;
  j(loc_y,loc_speed)=d*(sin_t);
  j(loc_y,loc_heading)=d*s*(dtheta_sin_t);

  j(loc_speed,loc_x)=0; j(loc_speed,loc_y)=0;
  j(loc_speed,loc_speed)=1; j(loc_speed,loc_heading)=0;

  j(loc_heading,loc_x)=0;     j(loc_heading,loc_y)=0;
  j(loc_heading,loc_speed)=0; j(loc_heading,loc_heading)=1;

//   vcl_cout << "j" << vcl_endl << j << vcl_endl;

  return j;
}


void
speed_heading_fun
::current_velocity( vnl_double_2 & v ) const
{
  double cos_t = vcl_cos(params_[loc_heading]);
  double sin_t = vcl_sin(params_[loc_heading]);
  double s = params_[loc_speed];
  v[0] = s*(cos_t);
  v[1] = s*(sin_t);
}

vnl_matrix<double>
speed_heading_fun
::observation_model(vnl_vector<double> /*pred*/)
{
  vnl_matrix<double> H(2,number_of_state_params);
  H.fill( 0.0 );
  H(loc_x,loc_x) = 1;
  H(loc_y,loc_y) = 1;
  return H;
}

vnl_matrix<double> const &
speed_heading_fun
::current_cov() const
{
  return covar_;
}

vnl_vector<double> const &
speed_heading_fun
::current_state() const
{
  return params_;
}

vnl_vector<double>
speed_heading_fun
::to_observation(vnl_vector<double> const & x_pred)const
{
  vnl_vector<double> r(2);
  r[0] = x_pred[loc_x];
  r[1] = x_pred[loc_y];
  return r;
}

void
speed_heading_fun
::current_location_covar( vnl_double_2x2 & lcov ) const
{
  lcov(0,0) = covar_(loc_x,loc_x); lcov(0,1) = covar_(loc_x,loc_y);
  lcov(1,0) = covar_(loc_y,loc_x); lcov(1,1) = covar_(loc_y,loc_y);
//   lcov(1,1) = covar_(loc_x,loc_x); lcov(1,0) = covar_(loc_x,loc_y);
//   lcov(0,1) = covar_(loc_y,loc_x); lcov(0,0) = covar_(loc_y,loc_y);
//   vcl_cout << "lcov:\n" << lcov << vcl_endl;
}

void
speed_heading_fun
::current_location(vnl_double_2 & loc) const
{
  loc[0] = params_[loc_x];
  loc[1] = params_[loc_y];
}


void
speed_heading_fun
::set_state(vnl_vector<double> const & new_state)
{
  params_ = new_state;
}

void
speed_heading_fun
::set_cov( vnl_matrix<double> const & new_cov )
{
  covar_ = new_cov;
}

void
speed_heading_fun
::initialize( track const & init_track, vnl_matrix<double> const & init_covar )
{
  track_state_sptr const& init_state = init_track.last_state();
  params_[loc_x] = init_state->loc_[0];
  params_[loc_y] = init_state->loc_[1];
  vnl_double_2 vel(init_state->vel_[0],init_state->vel_[1]);
  params_[loc_speed] = ((vel[0]<0)?-1:1)*vcl_sqrt(vel[0]*vel[0]+vel[1]*vel[1]);
  if(params_[loc_speed] && vel[0])
  {
    params_[loc_heading] = vcl_atan(vel[1]/vel[0]);
  }
  else if(vel[1]) //vel == 0, so 90*
  {
    params_[loc_heading] = 1.5707963267949;
  }
  else //speed is zero, so velocity is inf, we need to do something else to get heading.
  {
    track_state_sptr const & first_state = init_track.first_state();
    double dx = params_[loc_x] - first_state->loc_[0];
    double dy = params_[loc_y] - first_state->loc_[1];
    if(dx == 0 && dy == 0)
    {
      params_[loc_heading] = 0.5;
    }
    else if( dx == 0 )
    {
      params_[loc_heading] = 1.5707963267949;
    }
    else
    {
      params_[loc_heading] = vcl_atan(dy/dx);
    }
  }

  //double cos_t = vcl_cos(vcl_atan(vel[1]/vel[0]));
  //double sin_t = vcl_sin(vcl_atan(vel[1]/vel[0]));
  //double s = params_[loc_speed];

  //vcl_cout << "\tinit: " << vel << "\t" << s*(cos_t) << " " << s*(sin_t) << vcl_endl;

  covar_ = init_covar;
}

void
speed_heading_fun
::state_to_loc( vnl_vector<double> const & state,
                vnl_matrix<double> const & covar,
                vnl_vector_fixed<double,2>& pred_pos,
                vnl_matrix_fixed<double,2,2>& pred_cov) const
{
  pred_pos[0] = state[loc_x];
  pred_pos[1] = state[loc_y];
  pred_cov(0,0) = covar(loc_x,loc_x); pred_cov(0,1) = covar(loc_x,loc_y);
  pred_cov(1,0) = covar(loc_y,loc_x); pred_cov(1,1) = covar(loc_y,loc_y);
//   vcl_cout << "pred_cov:\n" << pred_cov << vcl_endl;
}

}//namespace extened_kalman_functions

}//namespace vidtk

