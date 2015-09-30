/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vnl/vnl_double_3.h>
#include <vcl_vector.h>
#include <utilities/log.h>
#include <rrel/rrel_linear_regression.h>
#include <rrel/rrel_irls.h>
#include <rrel/rrel_tukey_obj.h>

#include <vnl/algo/vnl_svd.h>

#include "track_velocity_init_functions.h"

namespace vidtk
{

bool initialize_velocity_const_last_detection( track_sptr trk, unsigned int sp )
{
  if(!trk)
  {
    return false;
  }

  vcl_vector< track_state_sptr > const& hist = trk->history();
  if(!sp||sp > hist.size() )
  {
    sp = hist.size();
  }
  track_state_sptr start_state = hist[(sp>hist.size())?(0):(hist.size() - sp)];

  vnl_vector_fixed<double,3> vel
          = (trk->last_state()->loc_ - start_state->loc_ ) / trk->last_state()->time_.diff_in_secs( start_state->time_ );
  for( vcl_vector<track_state_sptr>::const_iterator iter = hist.begin();
       iter != trk->history().end(); ++iter)
  {
    (*iter)->vel_ = vel;
  }
  return true;
}

bool initialize_velocity_robust_const_acceleration( track_sptr trk,
                                                    unsigned int num_detection_back,
                                                    double norm_resid
                                                  )
{
  vcl_vector< track_state_sptr > const& hist = trk->history();
  if(num_detection_back >= hist.size()-2)
  {
    return false;
  }
  vnl_vector_fixed<double,3> vel = (trk->last_state()->loc_ - trk->first_state()->loc_) / trk->get_length_time_secs();
  //vcl_cout << vel << "\t" <<  trk->last_state()->loc_ << "\t" << trk->first_state()->loc_
  //         << "\tt" << trk->get_length_time_secs() << "\t "
  //         << (trk->last_state()->loc_ - trk->first_state()->loc_).magnitude() << vcl_endl;
  double end_speed = vel.magnitude();
  vnl_vector_fixed<double,3> dir = vel/end_speed;
  vnl_vector< double > init(2,0);
  double start_time = trk->first_state()->time_.time_in_secs();
  vcl_vector< vnl_vector< double > > ind;
  vcl_vector< double > dep;
  dep.reserve(hist.size());
  ind.reserve(hist.size());
  unsigned int j = 0;
  double sum = 0;
  double count = 0;
  for( unsigned int i = num_detection_back; i < hist.size(); ++i, ++j )
  {
    track_state_sptr previous_state = hist[j];
    track_state_sptr current_state = hist[i];
    vel = (current_state->loc_ - previous_state->loc_) / current_state->time_.diff_in_secs( previous_state->time_ );
    vnl_vector< double > s(2);
    s[0] = current_state->time_.time_in_secs() - start_time;
    s[1] = 1;
    dep.push_back(vel.magnitude());
    sum += s[1]/s[0];
    count++;
    vcl_cout << i << " (" << hist.size() << ")," << s[0] << "," << s[1]  << "," << current_state->time_.diff_in_secs( previous_state->time_ ) << vcl_endl;
    ind.push_back(s);
  }
  init[0] = sum/count;
  rrel_linear_regression rlr( ind,dep );
  rrel_tukey_obj tukey(norm_resid);
  rrel_irls irls;
  irls.initialize_params(init);
  irls.estimate(&rlr, &tukey);
  vnl_vector< double > r = irls.params();
  log_debug("Estimated Params: " << r << vcl_endl );

  for( unsigned int i = 0; i < hist.size(); ++i )
  {
    track_state_sptr current_state = hist[i];
    double t = current_state->time_.time_in_secs() - start_time;
    double s = r[0]*t+r[1];
    vcl_cout << i << " s: " << s << vcl_endl;
    if(s < 0) s = 0;
    vel = dir*s;
    current_state->vel_ = vel;
  }
  return true;
}

bool initialize_velocity_raw( track_sptr trk, unsigned int num_detection_back )
{
  vcl_vector< track_state_sptr > const& hist = trk->history();
  if(num_detection_back >= hist.size()-2)
  {
    return false;
  }
  vnl_vector_fixed<double,3> vel;
  unsigned int j = 0;
  for( unsigned int i = num_detection_back; i < hist.size(); ++i, ++j )
  {
    track_state_sptr previous_state = hist[j];
    track_state_sptr current_state = hist[i];
    vel = (current_state->loc_ - previous_state->loc_) / current_state->time_.diff_in_secs( previous_state->time_ );
    hist[i]->vel_ = vel;
  }
  for( unsigned int i = 0; i < num_detection_back; ++i )
  {
    hist[i]->vel_ =  hist[num_detection_back]->vel_;
  }
  return true;
}

class init_vel_point : public rrel_estimation_problem { // based on rrel_linear_regression
public:

  //: Constructor with data pre-separated into arrays of independent and dependent variables.
  init_vel_point( const vcl_vector< vnl_vector< double > >&  ind_vars,
                  const vcl_vector< double >&  dep_vars )
    : rand_vars_(dep_vars), ind_vars_(ind_vars)
  {
    vcl_cout << ind_vars.size() << " " << ind_vars.size() << vcl_endl;
    set_param_dof( ind_vars_[0].size() );
    set_num_samples_for_fit( param_dof() );
  }

  //: Destructor.
  virtual ~init_vel_point()
  {}

  //: Total number of data points.
  unsigned int num_samples( ) const
  {
    return rand_vars_.size();
  }

  //: Generate a parameter estimate from a minimal sample set.
  bool fit_from_minimal_set( const vcl_vector<int>& point_indices,
                             vnl_vector<double>& params ) const
  {
    if ( point_indices.size() != param_dof() ) {
      vcl_cerr << "rrel_linear_regression::fit_from_minimal_sample  The number of point "
               << "indices must agree with the fit degrees of freedom.\n";
      return false;
    }
    vnl_matrix<double> A(param_dof(), param_dof(),0);
    vnl_vector<double> b(param_dof());
    for ( unsigned int i=0; i<param_dof()/2; ++i ) {
      int index = point_indices[i]*2;
      A(0,i*2) = ind_vars_[index][0];
      A(1,i*2+1) = ind_vars_[index+1][0];
      A(2,i*2) = ind_vars_[index][1];
      A(3,i*2+1) = ind_vars_[index+1][1];
      b[i] = rand_vars_[index];
      b[i+1] = rand_vars_[index+1];
    }
    vcl_cout << A << vcl_endl;

    vnl_svd<double> svd( A, 1.0e-8 );
    if ( (unsigned int)svd.rank() < param_dof() ) {
      return false;    // singular fit --- no error message needed
    }
    else {
      params = vnl_vector<double>( b * svd.inverse() );
      return true;
    }
  }

  //: Compute signed fit residuals relative to the parameter estimate.
  void compute_residuals( const vnl_vector<double>& params,
                          vcl_vector<double>& residuals ) const
  {
    assert( residuals.size() == rand_vars_.size() );

    for ( unsigned int i=0; i<rand_vars_.size(); i+=2 )
    {
      double dx = rand_vars_[i]   - dot_product( params, ind_vars_[i] );
      double dy = rand_vars_[i+1] - dot_product( params, ind_vars_[i+1] );
      double r = vcl_sqrt( dx*dx + dy*dy );
      residuals[i] = r;
      residuals[i+1] = r;
      vcl_cout << i << "\t" << params << " " << ind_vars_[i] << "\t" << r << "     " << rand_vars_.size() << vcl_endl;
      vcl_cout << i+1 << "\t" << params << " " << ind_vars_[i+1] << "\t" << r << vcl_endl;
    }
  }

  //: \brief Weighted least squares parameter estimate.
  bool weighted_least_squares_fit( vnl_vector<double>& params,
                                   vnl_matrix<double>& norm_covar,
                                   const vcl_vector<double>* weights=0 ) const
  {
    // If params and cofact are NULL pointers and the fit is successful,
    // this function will allocate a new vector and a new
    // matrix. Otherwise, it assumes that *params is a 1 x param_dof() vector
    // and that cofact is param_dof() x param_dof() matrix.

    assert( !weights || weights->size() == rand_vars_.size() );

    vnl_matrix<double> sumProds(param_dof(), param_dof(), 0.0);
    vnl_matrix<double> sumDists(param_dof(), 1, 0.0);

    //  Aside:  this probably would be faster if I used iterators...

    if (weights) {
      for ( unsigned int i=0; i<rand_vars_.size(); ++i ) {
        for ( unsigned int j=0; j<param_dof(); ++j ) {
          for ( unsigned int k=j; k<param_dof(); k++ )
            sumProds(j,k) += ind_vars_[i][j] * ind_vars_[i][k] * (*weights)[i];
          sumDists(j,0) += ind_vars_[i][j] * rand_vars_[i] * (*weights)[i];
        }
      }
    } else {
      for ( unsigned int i=0; i<rand_vars_.size(); ++i ) {
        for ( unsigned int j=0; j<param_dof(); ++j ) {
          for ( unsigned int k=j; k<param_dof(); k++ )
            sumProds(j,k) += ind_vars_[i][j] * ind_vars_[i][k];
          sumDists(j,0) += ind_vars_[i][j] * rand_vars_[i];
        }
      }
    }

    for ( unsigned int j=1; j<param_dof(); j++ )
      for ( unsigned int k=0; k<j; k++ )
        sumProds(j,k) = sumProds(k,j);

    vnl_svd<double> svd( sumProds, 1.0e-8 );
    if ( (unsigned int)svd.rank() < param_dof() ) {
      vcl_cerr << "rrel_linear_regression::WeightedLeastSquaresFit --- singularity!\n";
      return false;
    }
    else {
      vnl_matrix<double> sumP_inv( svd.inverse() );
      vnl_matrix<double> int_result = sumP_inv * sumDists;
      params = int_result.get_column(0);
      norm_covar = sumP_inv;
      return true;
    }
  }

protected:
  vcl_vector< double> rand_vars_;               // e.g. the z or depth values
  vcl_vector<vnl_vector<double> > ind_vars_;   // e.g. the image coordinates (plus 1.0
                                               // for intercept parameters)
};

bool initialize_velocity_robust_point(track_sptr trk,double norm_resid)
{
  vcl_vector< track_state_sptr > const& hist = trk->history();
  if( hist.size() < 2 )
  {
    return false;
  }
  track_state_sptr start_state = hist[0];
  double start_time = start_state->time_.time_in_secs();
  vnl_double_3 start_loc = start_state->loc_;
  vnl_vector< double > init(4,0);
  {
    vnl_vector_fixed<double,3> vel = (trk->last_state()->loc_ - start_loc) / trk->last_state()->time_.diff_in_secs( start_state->time_ );
    init[0] = vel[0];
    init[1] = vel[1];
    vcl_cout << "init: " << init << "      " << vel.magnitude() << "     " << trk->last_state()->time_.diff_in_secs( start_state->time_ ) << vcl_endl;
  }
  //vcl_cout << vel << "\t" <<  trk->last_state()->loc_ << "\t" << trk->first_state()->loc_
  //         << "\tt" << trk->get_length_time_secs() << "\t "
  //         << (trk->last_state()->loc_ - trk->first_state()->loc_).magnitude() << vcl_endl;
  vcl_vector< vnl_vector< double > > samples_ind;
  vcl_vector< double > dependent;
  samples_ind.reserve(hist.size()*2);
  dependent.reserve(hist.size()*2);
  for( unsigned int i = 0; i < hist.size(); ++i )
  {
    track_state_sptr current_state = hist[i];
    vnl_double_3 cloc = current_state->loc_;
    vnl_vector< double > s(4);
    double t = current_state->time_.time_in_secs() - start_time;
    s[0] = t;
    s[1] = 0;
    s[2] = t*t;
    s[3] = 0;
    samples_ind.push_back(s);
    dependent.push_back(cloc[0] - start_loc[0]);
    s[0] = 0;
    s[1] = t;
    s[2] = 0;
    s[3] = t*t;
    samples_ind.push_back(s);
    dependent.push_back(cloc[1] - start_loc[1]);
  }
  init_vel_point rlr( samples_ind, dependent );
  rrel_tukey_obj tukey(norm_resid);
  vnl_vector< double > r;
  {
    rrel_irls irls;
    irls.initialize_params(init);
    irls.estimate(&rlr, &tukey);
    r = irls.params();
    vcl_cout << r << vcl_endl;
  }
  for( unsigned int i = 0; i < hist.size(); ++i )
  {
    track_state_sptr current_state = hist[i];
    double t = current_state->time_.time_in_secs() - start_time;
    current_state->vel_ = vnl_double_3( r[0]+r[2]*t, r[1]+r[3]*t,0);
    vcl_cout << current_state->loc_[0] << " " <<  current_state->loc_[1] << " " << start_loc[0]+r[0]*t+r[2]*t*t << " " << start_loc[1]+r[1]*t+r[3]*t*t << vcl_endl;
  }
  return true;
}
#if 0
bool initialize_velocity_robust_point(track_sptr trk,double norm_resid)
{
  vcl_vector< track_state_sptr > const& hist = trk->history();
  track_state_sptr start_state = hist[hist.size() - 5];
  double start_time = start_state->time_.time_in_secs();
  vnl_double_3 start_loc = start_state->loc_;
  vnl_vector< double > init(4,0);
  {
    vnl_vector_fixed<double,3> vel(0,0,0);
    track_state_sptr s_state[4];
    s_state[0] = hist[hist.size() - 9];
    s_state[1] = hist[hist.size() - 7];
    s_state[2] = hist[hist.size() - 3];
    s_state[3] = hist[hist.size() - 1];
    for( unsigned int i = 0; i < 4; ++i )
    {
      vel += (s_state[i]->loc_ - start_loc) / s_state[i]->time_.diff_in_secs( start_state->time_ );
    }
    init[0] = vel[0]*0.25;
    init[1] = vel[1]*0.25;
    double ax = 0, ay = 0;
    for( unsigned int i = 0; i < 4; ++i )
    {
      double t = s_state[i]->time_.diff_in_secs( start_state->time_ );
      double odts = 1./(t*t);
      ax += odts*(s_state[i]->loc_[0] - start_loc[0] - vel[0]*t);
      ay += odts*(s_state[i]->loc_[1] - start_loc[1] - vel[1]*t);
    }
    init[2] = ax*0.25;
    init[3] = ay*0.25;
    vcl_cout << "init: " << init << "      " << vel.magnitude() << "     " << trk->last_state()->time_.diff_in_secs( start_state->time_ ) << vcl_endl;
  }
  //vcl_cout << vel << "\t" <<  trk->last_state()->loc_ << "\t" << trk->first_state()->loc_
  //         << "\tt" << trk->get_length_time_secs() << "\t "
  //         << (trk->last_state()->loc_ - trk->first_state()->loc_).magnitude() << vcl_endl;
  vcl_vector< vnl_vector< double > > samples_ind;
  vcl_vector< double > dependent;
  samples_ind.reserve(hist.size()*2);
  dependent.reserve(hist.size()*2);
  for( unsigned int i = 0; i < hist.size(); ++i )
  {
    track_state_sptr current_state = hist[i];
    vnl_double_3 cloc = current_state->loc_;
    vnl_vector< double > s(4,0);
    double t = current_state->time_.time_in_secs() - start_time;
    s[0] = t;
    s[1] = 0;
    s[2] = t*t;
    s[3] = 0;
//     s[4] = 1;
//     s[5] = 0;
    samples_ind.push_back(s);
    dependent.push_back(cloc[0] - start_loc[0]);
    s[0] = 0;
    s[1] = t;
    s[2] = 0;
    s[3] = t*t;
//     s[4] = 0;
//     s[5] = 1;
    samples_ind.push_back(s);
    dependent.push_back(cloc[1] - start_loc[1]);
  }
  init_vel_point rlr( samples_ind, dependent );
  rrel_tukey_obj tukey(norm_resid);
  vnl_vector< double > r;
  {
    rrel_irls irls;
    irls.initialize_params(init);
    irls.estimate(&rlr, &tukey);
    r = irls.params();
    vcl_cout << r << vcl_endl;
  }
  for( unsigned int i = 0; i < hist.size(); ++i )
  {
    track_state_sptr current_state = hist[i];
    double t = current_state->time_.time_in_secs() - start_time;
    current_state->vel_ = vnl_double_3( r[0]+r[2]*t, r[1]+r[3]*t,0);
    vcl_cout << current_state->loc_[0] << " " <<  current_state->loc_[1] << " " << start_loc[0]+r[0]*t+r[2]*t*t+r[4] << " " << start_loc[1]+r[1]*t+r[3]*t*t+r[5] << vcl_endl;
  }
}
#endif

} //namespace vidtk
