/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef tracker_cost_func_kin_h
#define tracker_cost_func_kin_h

#include "tracker_cost_function.h"

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_2x2.h>

namespace vidtk
{

/// \brief Tracker cost function based on spatial distance from kinematics.
///
/// One of the several detection-to-track correspondence cost functions.
/// This particular flavour uses a spatial upper bound through a prespecified
/// Mahanalobis distance (gate_sigma_sqr). After the first test has passed
/// it computes: -log( 1/((2pi)^(k/2)*det(Sigma)^0.5)*exp( -0.5*x*(1/Sigma)*x' ) )
///    -log( gaussian pdf ): log(2 pi det(Sigma)) - 0.5 log( x Sigma^-1 x' ), 
/// where x is a 2x1 Euclidean distance vector between the position of the 
/// object and the predicted position of the track. 
///

class tracker_cost_func_kin : public tracker_cost_function
{
public:
  tracker_cost_func_kin( vnl_double_2x2 const & mod_cov,
                         double gate_sigma_sqr );


  virtual void set( da_so_tracker_sptr tracker,
                    track_sptr track,
                    timestamp const* cur_ts_ );

  virtual double cost( image_object_sptr obj );

protected:
  vnl_double_2 pred_pos_;
  vnl_double_2x2 pred_cov_;
  vnl_double_2x2 mod_cov_;
  const double gate_sigma_sqr_;
  const double log_2_pi_;
};

};


#endif
