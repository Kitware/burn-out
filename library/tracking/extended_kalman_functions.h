/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_extended_kalman_functions_h_
#define vidtk_extended_kalman_functions_h_

#include <vnl/vnl_vector.h>
#include <vnl/vnl_matrix.h>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_2x2.h>

#include <vcl_iostream.h>

#include <tracking/track.h>

///Here are several example functions for the extended kalman filter

namespace vidtk
{

namespace extended_kalman_functions
{

class circular_fun
{
  public:
    circular_fun();

    vnl_vector< double > operator()(double d) const;

    vnl_matrix<double> jacobian( double d ) const;

    vnl_matrix<double> observation_model(vnl_vector<double> pred);

    vnl_matrix<double> const & current_cov() const;

    vnl_vector<double> const & current_state() const;

    vnl_vector<double> to_observation(vnl_vector<double> const & x_pred)const;

    void set_state(vnl_vector<double> const & new_state);

    void set_cov( vnl_matrix<double> const & new_cov );

    void current_location(vnl_double_2 & loc) const;
    void current_velocity( vnl_double_2 & v ) const;
    void current_location_covar( vnl_double_2x2 & lcov ) const;

    void initialize( track const & init_track,
                     vnl_matrix<double> const & init_covar );

    void state_to_loc( vnl_vector<double> const & state,
                       vnl_matrix<double> const & covar,
                       vnl_vector_fixed<double,2>& pred_pos,
                       vnl_matrix_fixed<double,2,2>& pred_cov) const;

    unsigned int const static number_of_state_params = 5;

  protected:
    vnl_vector<double> params_;
    vnl_matrix<double> covar_;
    enum {loc_x = 0, loc_y = 1, loc_theta = 2, loc_eta = 3, loc_r = 4 };
};

class linear_fun
{
  public:
    linear_fun();

    vnl_vector< double > operator()(double ts) const;

    vnl_matrix<double> jacobian( double d ) const;

    vnl_matrix<double> observation_model(vnl_vector<double> /*pred*/);

    vnl_matrix<double> const & current_cov() const;

    vnl_vector<double> const & current_state() const;

    vnl_vector<double> to_observation(vnl_vector<double> const & x_pred)const;

    void set_state(vnl_vector<double> const & new_state);

    void set_cov( vnl_matrix<double> const & new_cov );

    void current_location(vnl_double_2 & loc) const;
    void current_velocity( vnl_double_2 & v ) const;
    void current_location_covar( vnl_double_2x2 & lcov ) const;

    void initialize( track const & init_track,
                     vnl_matrix<double> const & init_covar );

    void state_to_loc( vnl_vector<double> const & state,
                       vnl_matrix<double> const & covar,
                       vnl_vector_fixed<double,2>& pred_pos,
                       vnl_matrix_fixed<double,2,2>& pred_cov) const;

    unsigned int const static number_of_state_params = 4;
  protected:
    vnl_vector<double> params_;
    vnl_matrix<double> covar_;
    enum {loc_x = 0, loc_y = 1, loc_delta_x = 2, loc_delta_y = 3 };
};

class speed_heading_fun
{
  public:
    speed_heading_fun();

    vnl_vector< double > operator()(double ts) const;

    vnl_matrix<double> jacobian( double d ) const;

    vnl_matrix<double> observation_model(vnl_vector<double> /*pred*/);

    vnl_matrix<double> const & current_cov() const;

    vnl_vector<double> const & current_state() const;

    vnl_vector<double> to_observation(vnl_vector<double> const & x_pred)const;

    void set_state(vnl_vector<double> const & new_state);

    void set_cov( vnl_matrix<double> const & new_cov );

    void current_location(vnl_double_2 & loc) const;
    void current_velocity( vnl_double_2 & v ) const;
    void current_location_covar( vnl_double_2x2 & lcov ) const;

    void initialize( track const & init_track,
                     vnl_matrix<double> const & init_covar );

    void state_to_loc( vnl_vector<double> const & state,
                       vnl_matrix<double> const & covar,
                       vnl_vector_fixed<double,2>& pred_pos,
                       vnl_matrix_fixed<double,2,2>& pred_cov) const;

    unsigned int const static number_of_state_params = 4;
  protected:
    vnl_vector<double> params_;
    vnl_matrix<double> covar_;
    enum {loc_x = 0, loc_y = 1, loc_speed = 2, loc_heading = 3 };
};

} //namespace extended_kalman_functions

} //namespace vidtk

#endif
