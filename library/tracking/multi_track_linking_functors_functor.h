/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_multi_track_linking_functors_functor_h_
#define vidtk_multi_track_linking_functors_functor_h_

#include <tracking/track.h>
#include <tracking/track_linking_functor.h>

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

#include <vcl_vector.h>

namespace vidtk
{

class multi_track_linking_functors_functor
{
  public:
    typedef vidtk::track_sptr               TrackTypePointer;

    multi_track_linking_functors_functor( double imc = 1000.)
      : impossible_match_cost_(imc)
    {
    }

    multi_track_linking_functors_functor( vcl_vector< track_linking_functor_sptr > const & fun,
                                          vcl_vector< double > const & w, double imc = 1000.);

    void set_impossible_match_cost( double c )
    {
      impossible_match_cost_ = c;
      vcl_cout << "Impossible match cost : " << impossible_match_cost_ << vcl_endl;
    }

    double get_impossible_match_cost() const
    {
      return impossible_match_cost_;
    }

    double operator()( TrackTypePointer track_i, TrackTypePointer track_j );

    void set_functors( vcl_vector< track_linking_functor_sptr > const & fun,
                       vcl_vector< double > const & w )
    {
      functors_ = fun;
      weights_ = w;
    }

  protected:
    // Maximum possible cost
    double impossible_match_cost_;
    vcl_vector< track_linking_functor_sptr > functors_;
    vcl_vector< double > weights_;
};

} //namespace vidtk

#endif //vidtk_multi_track_linking_functors_functor_h_
