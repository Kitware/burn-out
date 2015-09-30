/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef tracker_cost_func_color_size_kin_amhi_h
#define tracker_cost_func_color_size_kin_amhi_h

#include "tracker_cost_function.h"

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_2x2.h>

#include <utilities/image_histogram.h>

#include <tracking/multiple_features_process.h>

namespace vidtk
{

class tracker_cost_func_color_size_kin_amhi : public tracker_cost_function
{
  public:
    tracker_cost_func_color_size_kin_amhi( vnl_double_2x2 const & mod_cov,
                                           double sigma_gate_sqr,
                                           multiple_features const * mf_params,
                                           unsigned int area_window_size,
                                           double area_weight_decay_rate,
                                           double min_area_similarity_for_da,
                                           double min_color_similarity_for_da );
    
    virtual void set( da_so_tracker_sptr tracker,
                      track_sptr track,
                      timestamp const* cur_ts_ );

    virtual double cost( image_object_sptr obj );

  protected:

    inline double calc_distance( double color_prob,
                                 double kinematics_prob,
                                 double area_prob )
    {
      double color_similarity = mf_params_->get_similarity_score( color_prob, VIDTK_COLOR );
      double kinematics_similarity = kinematics_prob; // just take the probability (density)
      double area_similarity = area_prob; // just take the similarity

      double cumm_similarity =   kinematics_similarity * mf_params_->w_kinematics
                               + color_similarity      * mf_params_->w_color
                               + area_similarity       * mf_params_->w_area;

      return -vcl_log( cumm_similarity );
    }

    vnl_double_2 pred_pos_;
    vnl_double_2x2 pred_cov_;
    image_object_sptr curr_trk_o_;
    vnl_double_2x2 mod_cov_;
    double sigma_gate_sqr_;
    multiple_features const * mf_params_;
    image_histogram<vxl_byte, bool> trk_histo_obj_;
    image_histogram<vxl_byte, bool> trk_histo_obj_last_;
    double area_;
    unsigned int area_window_size_;
    double area_weight_decay_rate_;
    double min_area_similarity_for_da_;
    double min_color_similarity_for_da_;
};

class tracker_cost_func_color_size_kin_amhi_training : public tracker_cost_func_color_size_kin_amhi
{
  public:
    tracker_cost_func_color_size_kin_amhi_training(  vnl_double_2x2 const & mod_cov,
                                                     double sigma_gate_sqr,
                                                     multiple_features const * mf_params )
      : tracker_cost_func_color_size_kin_amhi(mod_cov, sigma_gate_sqr, mf_params , 1, 0, 0, 0 )
    {}
    virtual double cost( image_object_sptr obj );

    virtual void set_location(unsigned int trk_idx, unsigned int mod_idx)
    {
      trk_idx_ = trk_idx;
      mod_idx_ = mod_idx;
    }

    virtual void set_size(unsigned int M, unsigned int N)
    {
      color_p_ = vnl_matrix<double>( M , N );
      color_p_.fill( -1 );
      kinematics_p_ = vnl_matrix<double>( M , N );
      kinematics_p_.fill( -1 );
      area_p_ = vnl_matrix<double>( M , N );
      area_p_.fill( -1 );
    }

    virtual void output_training(unsigned int i, unsigned int j, vcl_ofstream & train_file_self)
    {
      train_file_self << color_p_(i,j) 
                      <<"\t" << kinematics_p_(i,j)
                      << "\t" << area_p_(i,j)
                      << vcl_endl;
    }

  protected:
    vnl_matrix<double> color_p_;
    vnl_matrix<double> kinematics_p_;
    vnl_matrix<double> area_p_;
    unsigned int trk_idx_, mod_idx_;
};

}

#endif
