/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "da_so_tracker_kalman_extended.h"

#include <vnl/vnl_inverse.h>

#include <vcl_sstream.h>

namespace vidtk
{

template< class function >
da_so_tracker_kalman_extended< function >
::da_so_tracker_kalman_extended( function fun,
                                 const_parameters_sptr kp )
: fun_(fun),
  params_(kp)
{
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::set_state( vnl_vector<double> state,
             vnl_matrix<double> covar,
             double time )
{
  fun_.set_state(state);
  fun_.set_cov(covar);
  last_time_ = time;
}

template< class function >
vnl_vector<double> const&
da_so_tracker_kalman_extended< function >
::state() const
{
  return fun_.current_state();
}

template< class function >
vnl_matrix<double> const&
da_so_tracker_kalman_extended< function >
::state_covar() const
{
  return fun_.current_cov();
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::get_current_location( vnl_double_2 & r ) const
{
  fun_.current_location(r);
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::get_current_velocity( vnl_double_2 & v ) const
{
  fun_.current_velocity(v);
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::get_current_location( vnl_vector_fixed< double, 3 > & l) const
{
  vnl_double_2 tl;
  fun_.current_location(tl);
  l[0] = tl[0];
  l[1] = tl[1];
  l[2] = 0;
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::get_current_velocity( vnl_vector_fixed< double, 3 > & v) const
{
  vnl_double_2 tv;
  fun_.current_velocity(tv);
  v[0] = tv[0];
  v[1] = tv[1];
  v[2] = 0;
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::get_current_location_covar( vnl_double_2x2 & lcov ) const
{
  fun_.current_location_covar(lcov);
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::initialize( track const & init_track )
{
  fun_.initialize(init_track, params_->init_cov_ );
  track_ = init_track.clone();
  last_time_ = init_track.last_state()->time_.time_in_secs();
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::update_impl( double const& ts,
               vnl_vector_fixed<double,2> const& loc,
               vnl_matrix_fixed<double,2,2> const& cov )
{
  vnl_vector< double > x_pred;
  vnl_matrix< double > P_pred;
  this->predict( ts, x_pred, P_pred );
  vnl_matrix<double> H_k = fun_.observation_model(x_pred);
  vnl_matrix<double> S_k = H_k*P_pred*H_k.transpose() + cov;
  vnl_matrix<double> K_k = P_pred*H_k.transpose()*vnl_inverse( S_k );

  fun_.set_state(x_pred+K_k*(loc-fun_.to_observation(x_pred)));
  vnl_matrix< double > eye(x_pred.size(), x_pred.size());
  eye.set_identity();
  fun_.set_cov( (eye - K_k*H_k)*P_pred );

  last_time_ = ts;
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::predict( double const& pred_ts_,
           vnl_vector_fixed<double,2>& pred_pos,
           vnl_matrix_fixed<double,2,2>& pred_cov ) const
{
  vnl_vector<double> state;
  vnl_matrix<double> covar;
  this->predict(pred_ts_, state, covar);
  fun_.state_to_loc( state,covar,pred_pos, pred_cov );
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::predict( double const& /*pred_ts_*/,
           vnl_vector_fixed<double,4>& /*state*/,
           vnl_matrix_fixed<double,4,4>& /*covar*/ ) const
{
  assert(!"DO NOT CALL, NOT IMPLEMNTED!!!");
}

template< class function >
void
da_so_tracker_kalman_extended< function >
::predict( double const& pred_ts_,
           vnl_vector<double>& state,
           vnl_matrix<double>& covar ) const
{
  double delta = pred_ts_ - last_time_;
//   vcl_cout << "\t\t" << pred_ts_ << " " << last_time_ << "    " << delta << vcl_endl;
  state = fun_(delta);
  vnl_matrix<double> F_kmd = fun_.jacobian(delta); // the predicted jacobian
//   vcl_cout << "FCiF^T\n" << F_kmd * fun_.current_cov() * F_kmd.transpose() << vcl_endl;
  covar = F_kmd * fun_.current_cov() * F_kmd.transpose() + params_->process_cov_;
}

template< class function >
vnl_double_4x4
da_so_tracker_kalman_extended< function >
::get_location_velocity_covar() const
{
  assert(!"DO NOT CALL, NOT IMPLEMNTED!!!");
  vnl_double_4x4 r;
  return r;
}

template< class function >
da_so_tracker_sptr
da_so_tracker_kalman_extended< function >
::clone() const
{
  return new da_so_tracker_kalman_extended< function >( *this );
}

template< class function >
void
da_so_tracker_kalman_extended< function >::const_parameters
::add_parameters_defaults(config_block & blk)
{
  unsigned int init_size = function::number_of_state_params;
  vcl_string proc_covar = "", init_covar = "";
  for( unsigned int i = 0; i < init_size; ++i )
  {
    for( unsigned int j = 0; j < init_size; ++j )
    {
      if(i == j)
      {
        proc_covar += "0.00001 ";
        init_covar += "0 ";
      }
      else
      {
        proc_covar += "0 ";
        init_covar += "0 ";
      }
    }
  }
  blk.add_parameter( "process_noise_covariance", proc_covar,
                     "The process noise" );
  blk.add_parameter( "initial_state_covariance", init_covar,
                     "The initial corvariance for kalman tracking" );
}

template< class function >
void
da_so_tracker_kalman_extended< function >::const_parameters
::set_parameters(config_block const & blk)
{
  //vnl_double_4x4 init_cov_;
  unsigned int init_size = function::number_of_state_params;
  init_cov_    = vnl_matrix<double>(init_size,init_size);
  process_cov_ = vnl_matrix<double>(init_size,init_size);
  blk.get( "initial_state_covariance", init_cov_ );
  //vnl_double_4x4 process_cov_;
  blk.get( "process_noise_covariance", process_cov_ );
  //bool use_fixed_process_noise_
  //vnl_double_2 process_noise_speed_scale_
}

template< class function >
da_so_tracker_kalman_extended< function >
::~da_so_tracker_kalman_extended()
{
}

};
