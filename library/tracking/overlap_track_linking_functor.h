/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_overlap_track_linking_functor_h_
#define vidtk_overlap_track_linking_functor_h_

#include <vcl_vector.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_math.h>
#include <limits>

#include <tracking/track.h>
#include <tracking/track_linking_functor.h>

///Track linking functor that utilizes a small about of overlap produced to link
namespace vidtk
{

class overlap_track_linking_functor : public track_linking_functor
{
public:
  typedef vidtk::track                    TrackType;
  typedef vidtk::track_sptr               TrackTypePointer;
  typedef track_state_sptr                TrackState;
  typedef vcl_vector< TrackState >        TrackStateList;
  typedef vnl_vector_fixed<double,3>      LocationVector;
  typedef LocationVector                  LocationDifferenceVector;
  typedef vnl_vector_fixed<double,3>      VelocityVector;

  overlap_track_linking_functor( double bbox_size_sigma = 2.0,
                                 double direction_sigma = 0.25,
                                 double overlap_sigma = 3.0,
                                 double overlap_mean = 2.5,
                                 double distance_sigma = 5.0,
                                 double bbox_not_overlap_sigma = 0.6,
                                 double max_min_distance = 7
                               );

  //Returns the negative log.
  overlap_track_linking_functor& operator=(const overlap_track_linking_functor& kf )
  {

    if ( this != & kf )
    {
      this->bbox_size_sigma_ = kf.bbox_size_sigma_;
      this->direction_sigma_ = kf.direction_sigma_;
      this->overlap_sigma_ = kf.overlap_sigma_;
      this->distance_sigma_ = kf.distance_sigma_;
    }
    return *this;
  }

  double operator()( TrackTypePointer track_i, TrackTypePointer track_j );

  virtual vcl_string name() { return "overlap_track_linking_functor"; }

protected:
  double bbox_size_sigma_;
  double direction_sigma_;
  double overlap_sigma_;
  double distance_sigma_;
  double overlap_mean_;
  double bbox_not_overlap_sigma_;
  double max_min_distance_;

};

}

#endif
