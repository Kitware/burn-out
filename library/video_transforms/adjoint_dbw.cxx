/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "adjoint_dbw.h"
#include "adjoint_flow_warp.h"
#include "adjoint_resample.h"
#include "adjoint_image_op.h"

#include <boost/bind.hpp>

#include <vil/algo/vil_gauss_filter.h>

namespace vidtk
{

/// Helper class to contain a set of 2 image operators that run in sequence.
/// Intermediate results are cached to avoid re-allocation of memory
template <typename T>
class image_op_2_func
{
public:
  typedef boost::function<void (const vil_image_view<T>& src,
                                vil_image_view<T>& dst)> func_t;

  image_op_2_func(func_t op1, func_t op2)
  : op1_(op1),
    op2_(op2) {}

  image_op_2_func(func_t op1, func_t op2,
                  unsigned ni, unsigned nj, unsigned np)
  : op1_(op1),
    op2_(op2),
    tmp_(ni, nj, np) {}

  /// Apply the sequence of 2 image operations
  void operator()(const vil_image_view<T>& src,
                  vil_image_view<T>& dst) const
  {
    op1_(src, tmp_);
    op2_(tmp_, dst);
  }

  /// Set the size of the intermediate result image between op1 and op2
  void set_intermediate_size(unsigned ni, unsigned nj, unsigned np)
  {
    tmp_.set_size(ni, nj, np);
  }

  /// Set the first function
  void set_op1(func_t op1)
  {
    op1_ = op1;
  }

  /// Set the second function
  void set_op2(func_t op2)
  {
    op2_ = op2;
  }

private:
  func_t op1_;
  func_t op2_;
  mutable vil_image_view<T> tmp_;
};

namespace
{

static void
vil_gauss_filter_2d_overload(const vil_image_view<double>& src_im,
                             vil_image_view<double>& dest_im,
                             double sd, unsigned half_width,
                             vil_convolve_boundary_option boundary)
{
  vil_gauss_filter_2d<double, double>(src_im, dest_im, sd, half_width, boundary);
}

}


//Makes an adjoint operator for DBW from optical flow fields
adjoint_image_ops_func<double>
create_dbw_from_flow(const vil_image_view<double> &flow,
                     const unsigned ni, const unsigned nj, const unsigned np,
                     int scale_factor,
                     double smoothing_sigma,
                     bool down_sample_averaging,
                     bool bicubic_warping)
{
  const unsigned int sni = flow.ni();
  const unsigned int snj = flow.nj();
  const unsigned int wni = ni * scale_factor;
  const unsigned int wnj = nj * scale_factor;

  typedef boost::function<void (const vil_image_view<double>& src,
                                vil_image_view<double>& dst)> func_t;
  func_t warp_fwd = boost::bind(warp_forward_with_flow_bilin<double, double, double>,
                                _1, flow, _2);
  func_t warp_back = boost::bind(warp_back_with_flow_bilin<double, double, double>,
                                 _1, flow, _2);
  if(bicubic_warping)
  {
    warp_fwd = boost::bind(warp_forward_with_flow_bicub<double, double, double>,
                           _1, flow, _2);
    warp_back = boost::bind(warp_back_with_flow_bicub<double, double, double>,
                            _1, flow, _2);
  }

  func_t blur_sf = boost::bind(vil_gauss_filter_2d_overload, _1, _2, smoothing_sigma,
                               static_cast<unsigned int>(3.0 * smoothing_sigma), vil_convolve_zero_extend);

  func_t down_s = boost::bind(down_sample<double>, _1, _2, scale_factor, 0, 0);
  func_t up_s = boost::bind(up_sample<double>, _1, _2, scale_factor, 0, 0);

  if(down_sample_averaging)
  {
    down_s = boost::bind(down_scale<double>, _1, _2, scale_factor);
    up_s = boost::bind(up_scale<double>, _1, _2, scale_factor);
  }

  image_op_2_func<double> forward_bw(warp_fwd, blur_sf, wni, wnj, np);
  image_op_2_func<double> forward(forward_bw, down_s, wni, wnj, np);

  image_op_2_func<double> backward_bw(blur_sf, warp_back, wni, wnj, np);
  image_op_2_func<double> backward(up_s, backward_bw, wni, wnj, np);

  return adjoint_image_ops_func<double>(forward, backward,
                                        sni, snj, np,
                                        ni, nj, np);
}


} // end namespace vidtk
