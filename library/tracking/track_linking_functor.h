/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_linking_functor_h_
#define vidtk_track_linking_functor_h_

#include <tracking/track.h>

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

#include <vcl_string.h>

namespace vidtk
{

class track_linking_functor : public vbl_ref_count
{
  public:
    typedef vidtk::track_sptr               TrackTypePointer;

    track_linking_functor( double imc = 1000.)
      : impossible_match_cost_(imc)
    {
    }

    void set_impossible_match_cost( double c )
    {
      impossible_match_cost_ = c;
      //vcl_cout << "Impossible match cost : " << impossible_match_cost_ << vcl_endl;
    }

    double get_impossible_match_cost() const
    {
      return impossible_match_cost_;
    }

    virtual double operator()( TrackTypePointer track_i, TrackTypePointer track_j ) = 0;

    virtual vcl_string name() = 0;

  protected:
    // Maximum possible cost
    double impossible_match_cost_;
};

typedef vbl_smart_ptr<track_linking_functor> track_linking_functor_sptr;

} //namespace vidtk

#endif //track_linking_functor_h_
