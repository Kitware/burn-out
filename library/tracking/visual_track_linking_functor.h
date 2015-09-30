/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_visual_track_linking_functor_h_
#define vidtk_visual_track_linking_functor_h_

#include <vcl_vector.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_math.h>
#include <limits>

#include <tracking/track.h>
#include <tracking/track_linking_functor.h>

#include <vnl/vnl_double_2.h>

namespace vidtk
{

class visual_track_linking_functor : public track_linking_functor
{
public:
  typedef vidtk::track                    TrackType;
  typedef vidtk::track_sptr               TrackTypePointer;
  typedef track_state_sptr                TrackState;
  typedef vcl_vector< TrackState >        TrackStateList;
  typedef vnl_vector_fixed<double,3>      LocationVector;
  typedef LocationVector                  LocationDifferenceVector;
  typedef vnl_vector_fixed<double,3>      VelocityVector;

  visual_track_linking_functor( double area_penalty = 50,
                                double mean_dist_penalty = 10,
                                double chi_penalty = 10 );

  visual_track_linking_functor& operator=(const visual_track_linking_functor& kf ) {

    if ( this != & kf )
    {
      impossible_match_cost_ = kf.impossible_match_cost_;
    }
    return *this;
  }

  double operator()( TrackTypePointer track_i, TrackTypePointer track_j );

  virtual vcl_string name() { return "visual_track_linking_functor"; }

protected:

  void mean_var( vil_image_view<vxl_byte> const & data, vil_image_view<bool> const & mask, 
                 unsigned int xbegin, unsigned int ybegin,
                 double & mean, double & var ) const;

  double correlation( vil_image_view<vxl_byte> const & data_x,
                      vil_image_view<vxl_byte> const & data_y,
                      vil_image_view<bool> const & mask,
                      unsigned int x_xbegin, unsigned int x_ybegin,
                      unsigned int y_xoffset, unsigned int y_yoffset ) const;
  double find_best( vil_image_view<vxl_byte> x,
                    vil_image_view<vxl_byte> y,
                    vil_image_view<bool> bmask,
                    unsigned int x_xbegin, unsigned int x_ybegin ) const;
  vnl_double_2
  normalize_cross_correlation( vcl_vector<vidtk::image_object_sptr> const & objs_end,
                               vcl_vector<vidtk::image_object_sptr> const & objs_start ) const;
  void  mean_var( vcl_vector<vidtk::image_object_sptr> const & objs, double & mean, double & var ) const;

  double area_penalty_;
  double mean_dist_penalty_;
  double chi_penalty_;

};

}

#endif //vidtk_visual_track_linking_functor_h_
