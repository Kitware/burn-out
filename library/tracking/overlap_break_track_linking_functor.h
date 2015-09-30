/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_overlap_break_track_linking_functor_h_
#define vidtk_overlap_break_track_linking_functor_h_

#include <vcl_vector.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_math.h>
#include <limits>

#include <tracking/track.h>
#include <tracking/track_linking_functor.h>

///Track linking functor that removes the overlap between two tracks and applies another linking functor to it.
namespace vidtk
{

  class overlap_break_track_linking_functor : public track_linking_functor
  {
  public:
    typedef vidtk::track                    TrackType;
    typedef vidtk::track_sptr               TrackTypePointer;
    typedef track_state_sptr                TrackState;
    typedef vcl_vector< TrackState >        TrackStateList;
    typedef vnl_vector_fixed<double,3>      LocationVector;
    typedef LocationVector                  LocationDifferenceVector;
    typedef vnl_vector_fixed<double,3>      VelocityVector;

    enum overlap_score_merge_type{MERGE_MIN, MERGE_MAX, MERGE_MEAN};

    overlap_break_track_linking_functor( track_linking_functor_sptr sub_functor );

    //Returns the negative log.
    overlap_break_track_linking_functor& operator=(const overlap_break_track_linking_functor& kf )
    {

      if ( this != & kf )
      {
        functor_ = kf.functor_;
      }
      return *this;
    }

    double operator()( TrackTypePointer track_i, TrackTypePointer track_j );

    virtual vcl_string name() { return "overlap_break_track_linking_functor"; }

    void merge_type(overlap_score_merge_type osmt);
    void overlap_time_sigma( double s );

  protected:
    track_linking_functor_sptr functor_;
    overlap_score_merge_type merge_stratigy_;
    bool overlap_time_sigma_set_;
    double overlap_time_sigma_;
  };

}

#endif
