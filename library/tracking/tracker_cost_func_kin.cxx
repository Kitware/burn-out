/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tracker_cost_func_kin.h"

#include <vnl/vnl_math.h>
#include <vnl/vnl_inverse.h>

namespace vidtk
{

tracker_cost_func_kin
::tracker_cost_func_kin( vnl_double_2x2 const & mod_cov,
                         double gate_sigma_sqr )
  : mod_cov_(mod_cov), 
    gate_sigma_sqr_(gate_sigma_sqr),
    log_2_pi_( vcl_log( 2 * vnl_math::pi ) )
{
}

void
tracker_cost_func_kin
::set( da_so_tracker_sptr tracker, track_sptr /*track*/, timestamp const* cur_ts )
{
  tracker->predict( cur_ts->time_in_secs(), pred_pos_, pred_cov_ );
}

double
tracker_cost_func_kin
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
    return INF_;
  }

  if( mahan_dist_sqr < gate_sigma_sqr_ )
  {
    //normalized probability density (pd) by the peak pd, in (0,1]
    double final_distance = 0.5 * mahan_dist_sqr;
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
    return final_distance;
  }
  else
  {
    // Inf, to ensure its not assigned.
    return INF_;
  }
}

} //namespace vidtk
