/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tracker_cost_func_color_size_kin_amhi.h"

#include <tracking/tracking_keys.h>

#include <vnl/vnl_inverse.h>
#include <vnl/vnl_math.h>

#include <vcl_cassert.h>
#include <vcl_cmath.h>
#include <vcl_algorithm.h>

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{

tracker_cost_func_color_size_kin_amhi
::tracker_cost_func_color_size_kin_amhi( vnl_double_2x2 const & mod_cov,
                                         double sigma_gate_sqr,
                                         multiple_features const * mf_params,
                                         unsigned int area_window_size,
                                         double area_weight_decay_rate,
                                         double min_area_similarity_for_da,
                                         double min_color_similarity_for_da)
  : mod_cov_(mod_cov), sigma_gate_sqr_(sigma_gate_sqr), mf_params_(mf_params)
{
  assert(mf_params_);
  assert(mf_params_->test_enabled);
  area_window_size_ = area_window_size;
  area_weight_decay_rate_ = area_weight_decay_rate;
  min_area_similarity_for_da_ = min_area_similarity_for_da;
  min_color_similarity_for_da_ = min_color_similarity_for_da;
}

void
tracker_cost_func_color_size_kin_amhi
::set( da_so_tracker_sptr tracker, track_sptr track, timestamp const* cur_ts )
{
  tracker->predict( cur_ts->time_in_secs(), pred_pos_, pred_cov_ );
  curr_trk_o_ = NULL;

  vcl_vector<image_object_sptr> curr_trk_objs;
  vcl_vector<track_state_sptr> const& curr_hist = track->history();

  area_=0.0;
  if( !curr_hist.empty() && 
      curr_hist.back()->data_.get( tracking_keys::img_objs, curr_trk_objs ) )
  {
    curr_trk_o_ = curr_trk_objs[0];
    if(!curr_trk_o_->data_.get(tracking_keys::histogram, trk_histo_obj_))
    {
      trk_histo_obj_.set_mass(0.0);
    }

    double total_weight = 0.0;
    double start_idx = vcl_max(0, ((int)curr_hist.size() - (int)area_window_size_) );
    double curr_weight = 1.0;
    for(int i = curr_hist.size() - 1 ; i >= start_idx ; i--)
    {
      curr_weight *= ( 1.0 - area_weight_decay_rate_ );
      if(curr_hist.at(i)->data_.get( tracking_keys::img_objs, curr_trk_objs ))
      {
        if(curr_trk_objs[0] && curr_trk_objs[0]->area_ > 0)
        {
          area_ += curr_trk_objs[0]->area_ * curr_weight;
          total_weight += curr_weight;
        }
      }
    }
    if(total_weight > 0.0)
    {
      area_ /= total_weight;
    }
    else
    {
      area_ = 0.0;
    }
  }
}

inline void
tcfcska_calculate_probs( image_object const& obj,
                         vnl_double_2x2 & cov,
                         vnl_double_2 & delta,
                         double mf_pdf_delta,
                         image_histogram<vxl_byte, bool> & trk_histo_obj,
                         image_object_sptr curr_trk_o,
                         double area,
                         double & color_prob,
                         double & kinematics_prob,
                         double & area_prob,
                         double min_area_similarity_for_da,
                         double min_color_similarity_for_da )
{
  static double const two_pi = 2 * vnl_math::pi;
  static double const min_prob_for_log = 1.0e-20;

  //Compute the combined cost based on linear combination of MF.

  //P_color
  image_histogram<vxl_byte, bool> hist;
  try
  {
    obj.data_.get( vidtk::tracking_keys::histogram, hist );
    if( trk_histo_obj.mass() == 0.0 )
    {
      color_prob = 0.0;
    }
    else
    {
      color_prob = hist.compare( trk_histo_obj.get_h() );
      color_prob = (color_prob > min_color_similarity_for_da) ? color_prob : -0.1;
    }
  }
  catch( unchecked_return_value const& e)
  {
    log_error("tcfcska_calculate_probs(): couldn't get color histogram from object\n" );
    color_prob = 0.0;
  }

  //P_kinematics
  double mahan_dist_sqr = dot_product( delta, vnl_inverse( cov ) * delta );
  //normalized probability density (pd) by the maximum pd. It is in (0,1]
  kinematics_prob = vcl_exp( -0.5 * mahan_dist_sqr );


  //P_area
  double area_diff;
  if(curr_trk_o == NULL || vcl_min( area , obj.area_ ) <= 0)
  {
    area_prob = 0.0;
  }
  else
  {
    area_diff = vcl_max( area , obj.area_ ) / vcl_min( area , obj.area_ ) - 1.0;
    area_prob = vcl_exp( - area_diff);
    area_prob = (area_prob > min_area_similarity_for_da) ? area_prob : -0.1;
  }
}

double
tracker_cost_func_color_size_kin_amhi
::cost( image_object_sptr obj_sptr )
{
  image_object const& obj = *obj_sptr;

  // Add the uncertainty of the measurement.
  vnl_double_2x2 cov = pred_cov_;
  cov += mod_cov_; // now mod_cov in meter square, it is better to be
                   // in pixel square, which requires homography here
                   // or at least gsd. Note: gui only outputs pred_cov
                   // times gate_sigma

  // Assume z=0 in the measurements.
  vnl_double_2 delta( obj.world_loc_[0], obj.world_loc_[1] );
  delta -= pred_pos_;

  double mahan_dist_sqr;
  if(vnl_det(cov) != 0)
  {
    mahan_dist_sqr = dot_product( delta, vnl_inverse( cov ) * delta );
  }
  else
  {
    mahan_dist_sqr = INF_;
  }


  if( mahan_dist_sqr < sigma_gate_sqr_ )
  {
    double color_prob, kinematics_prob, area_prob;

    tcfcska_calculate_probs( obj, cov, delta,
                             mf_params_->pdf_delta,
                             trk_histo_obj_,
                             curr_trk_o_,
                             area_,
                             color_prob,
                             kinematics_prob,
                             area_prob,
                             min_area_similarity_for_da_,
                             min_color_similarity_for_da_);

    // if color or area similarity is below thresholds
    if(color_prob < 0 || area_prob < 0)
    {
      return INF_;
    }
    else
    {
      //Only put the combined cost in final_distance if MF is used.
      return calc_distance(color_prob, kinematics_prob, area_prob);
    }
  }
  else
  {
    // Inf, to ensure its not assigned.
    return INF_;
  }
}

double
tracker_cost_func_color_size_kin_amhi_training
::cost( image_object_sptr obj_sptr )
{
  image_object const& obj = *obj_sptr;

  // Add the uncertainty of the measurement.
  vnl_double_2x2 cov = pred_cov_;
  cov += mod_cov_;

  // Assume z=0 in the measurements.
  vnl_double_2 delta( obj.world_loc_[0], obj.world_loc_[1] );
  delta -= pred_pos_;

  double mahan_dist_sqr;
  if(vnl_det(cov) != 0)
  {
    mahan_dist_sqr = dot_product( delta, vnl_inverse( cov ) * delta );
  }
  else
  {
    mahan_dist_sqr = INF_;
  }

  if( mahan_dist_sqr < sigma_gate_sqr_ )
  {
    double final_distance;
    double color_prob, kinematics_log_prob, area_log_prob;

    tcfcska_calculate_probs( obj, cov,
                             delta, mf_params_->pdf_delta,
                             trk_histo_obj_, curr_trk_o_, area_,
                             color_prob, kinematics_log_prob, area_log_prob,
                             min_area_similarity_for_da_,
                             min_color_similarity_for_da_ );

    //Only put the combined cost in final_distance if MF is used for testing. 
    if( mf_params_->test_enabled )
    {
      final_distance = calc_distance(color_prob, kinematics_log_prob, area_log_prob);
    }
    //Use the old cost based only on the Mahanalobis distance (kinematics)
    else
    {

      // -ve log of Gaussian pdf
      //gaussian pdf: 1/((2pi)^(k/2)*det(Sigma)^0.5)*exp( -0.5*x*(1/Sigma)*x' )
      //            : 1/(2*pi*det(Sigma)^0.5) * exp( -0.5*Mahan_dist )
      //log( gaussian pdf ): log( 1/(2*pi*det(Sigma)^0.5) - 0.5*Mahan_dist
      //                   : -log( 2*pi*det(Sigma)^0.5 ) - 0.5*Mahan_dist
      //-log( gaussian pdf ): log( 2*pi*det(Sigma)^0.5 ) + 0.5*Mahan_dist
      //                    : log( 2*pi ) + log( det(Sigma)^0.5 ) + 0.5*Mahan_dist
      //                    : log( 2*pi ) + 0.5*log( det(Sigma) ) + 0.5*Mahan_dist
      double neg_ll = vcl_log( 2 * vnl_math::pi ) + 0.5 * vcl_log( vnl_det( cov ) ) 
                      + 0.5 * mahan_dist_sqr;

#ifdef DEBUG
      if( vnl_math_isnan( neg_ll ) )
      {
        log_debug( "NAN: At ("<<trk_idx<<","<<mod_idx<<"): mahan_dist="<< mahan_dist_sqr << ";  cov=\n"<<cov<<"\n" );
      }
      else if( vnl_math_isinf( neg_ll ) )
      {
        log_debug( "INF: At ("<<trk_idx<<","<<mod_idx<<"): mahan_dist="<< mahan_dist_sqr << ";  cov=\n"<<cov<<"\n" );
      }
#endif
      //Original 
      final_distance = neg_ll;
    }

    if( mf_params_ && mf_params_->train_enabled )
    {
      kinematics_p_(trk_idx_, mod_idx_) = kinematics_log_prob;
      color_p_(trk_idx_, mod_idx_) = color_prob;
      area_p_(trk_idx_, mod_idx_) = area_log_prob;
    }

    return final_distance;
  }
  else
  {
    // Inf, to ensure its not assigned.
    return INF_;
  }
}

} //namespace vidtk
