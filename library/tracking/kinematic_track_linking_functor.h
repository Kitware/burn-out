/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kinematic_track_linking_functor_h_
#define vidtk_kinematic_track_linking_functor_h_

#include <vcl_vector.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_math.h>
#include <limits>

#include <tracking/track.h>
#include <tracking/track_linking_functor.h>

namespace vidtk
{

class kinematic_track_linking_functor : public track_linking_functor
{
public:
  typedef vidtk::track                    TrackType;
  typedef vidtk::track_sptr               TrackTypePointer;
  typedef track_state_sptr                TrackState;
  typedef vcl_vector< TrackState >        TrackStateList;
  typedef vnl_vector_fixed<double,3>      LocationVector;
  typedef LocationVector                  LocationDifferenceVector;
  typedef vnl_vector_fixed<double,3>      VelocityVector;

  kinematic_track_linking_functor( double sigma = 1.0,
                                   double direction_penalty = 40.,
                                   double exp_transition_point_distance = 30. );

  kinematic_track_linking_functor& operator=(const kinematic_track_linking_functor& kf )
  {

    if ( this != & kf )
    {
      norm_const_ = kf.norm_const_;
      sigma_squared_ = kf.sigma_squared_;
      impossible_match_cost_ = kf.impossible_match_cost_;
    }
    return *this;
  }

  double operator()( TrackTypePointer track_i, TrackTypePointer track_j );

  virtual vcl_string name() { return "kinematic_track_linking_functor"; }

protected:
  // Normalization constant for a single-dimensional Gaussian with
  // variance sigma_squared_.
  double norm_const_;

  // Variance for a single-dimensional Gaussian distribution.
  double sigma_squared_;

  // How far back in detections to for calculating velocity
  unsigned int back_step_;

  //Penalty multiplers
  double direction_penalty_;
  //distance for the expontial distance function
  double exp_transition_point_distance_;

};

}

#endif

