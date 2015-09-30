/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_following_track_linking_functor_h_
#define vidtk_following_track_linking_functor_h_

#include <vcl_vector.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_math.h>
#include <limits>

#include <tracking/track.h>
#include <tracking/track_linking_functor.h>

///Track linking functor based on the linking done in the following event dectector
namespace vidtk
{

class following_track_linking_functor : public track_linking_functor
{
public:
  typedef vidtk::track                    TrackType;
  typedef vidtk::track_sptr               TrackTypePointer;
  typedef track_state_sptr                TrackState;
  typedef vcl_vector< TrackState >        TrackStateList;
  typedef vnl_vector_fixed<double,3>      LocationVector;
  typedef LocationVector                  LocationDifferenceVector;
  typedef vnl_vector_fixed<double,3>      VelocityVector;

  following_track_linking_functor( double bbox_size_sigma = 2.0,
                                   double speed_sigma = 2.5,
                                   double time_sigma = 5.0,
                                   double direction_sigma = 0.25,
                                   double pred_dist_sigma = 10,
                                   double angle_velocity_sigma = 2.44,
                                   double angle_velocity_threshold = 2.44
                                 );

  following_track_linking_functor& operator=(const following_track_linking_functor& kf )
  {

    if ( this != & kf )
    {
      this->bbox_size_sigma_ = kf.bbox_size_sigma_;
      this->speed_sigma_ = kf.speed_sigma_;
      this->time_sigma_ = kf.time_sigma_;
      this->direction_sigma_ = kf.direction_sigma_;
      this->pred_dist_sigma_ = kf.pred_dist_sigma_;
    }
    return *this;
  }

  double operator()( TrackTypePointer track_i, TrackTypePointer track_j );

  virtual vcl_string name() { return "following_track_linking_functor"; }

protected:
  double bbox_size_sigma_;
  double speed_sigma_;
  double time_sigma_;
  double direction_sigma_;
  double pred_dist_sigma_;
  double angle_velocity_sigma_;
  double angle_velocity_threshold_;

};

}

#endif
