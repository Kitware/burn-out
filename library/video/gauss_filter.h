/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _gauss_filter_h_
#define _gauss_filter_h_

#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <vil/vil_transpose.h>
#include <vnl/vnl_math.h>
#include <video/convolve_1d.h>
#include <vil/algo/vil_convolve_1d.h>
#include <vcl_limits.h>
#include <vcl_cmath.h>
#include <utilities/checked_bool.h>

namespace vidtk
{

inline vxl_int_64 gen_int_gauss_filter(double sd, vcl_vector<unsigned> &filter)
{

  /*
    Input: Gaussian sd, kernel radius r, and kernel
    dimension D
    Output: Integer Gaussian kernel
    1) Calculate real number R'0,1 = exp( 1/2sd^2)  using
    Equation 6;

    2) Since R'0,1 must be rational, find the “best rational
    approximation” R0,1 = p/q of integers that
    approximates R'0,1 using the “continued fraction”;

    3) Calculate the Euclidean distance from the kernel
    center to each kernel entry xj , where
    j = 1...(2r + 1)^D;

    4) Calculate the maximum distance xm;

    5) Calculate G(0) = (p)^(xm^2) using Equation 11;

    6) Find each kernel entry G(xj) = G(0)/(R0,1)^(xj^2)
    using Equation 10;

    7) Find the sum of the kernel entries from j to (2r+1)^D of G(xj);

    8) Apply the kernel to the image and divide the result by
    the kernel sum;
  */

  //1)
  double R_prime01 = exp( 1.0 / (2.0 * sd * sd) );

  //2)
  double d = R_prime01;
  // Continued fraction approximation: recursively determined
  vxl_int_64 den=0L, num=1L, prev_den=1L, prev_num=0L;

  while (d*num < 1e9 && d*den < 1e9) {
    vxl_int_64 a = static_cast<vxl_int_64>(d); // integral part of d
    d -= a; // certainly >= 0
    vxl_int_64 temp = num; num = a*num + prev_num; prev_num = temp;
    temp = den; den = a*den + prev_den; prev_den = temp;
    if (d < 1e-1) break;
    d = 1/d;
  }

  double p = num;
  double q = den;

  //3) &  //4)
  unsigned rad = filter.size()/2;
  unsigned rad_sq = rad*rad;

  //5)
  //the distance from the center of the matrix to the corner is always
  // xm = srt( ( 0-rad)^2 + ( 0-rad)^2 ).  We only need xm^2 so we can
  // just skip taking the sqrt of rad_sq + rad_sq here
  // even if it's a bit less clear what's going on.
  int xm_sq = rad_sq + rad_sq;
  vxl_int_64 G_0 = vcl_pow(p,xm_sq);

  //6)
  vcl_size_t center = filter.size()/2; // or just past center if even length
  vcl_vector< vxl_int_64 > tmp_filter( filter.size() );
  tmp_filter[center] = G_0;

  for (unsigned i=1 ; i<=center; ++i)
  {
    // use p^(xm^2 - xj^2)*q^(xm^2) for G_xj to avoid
    // the floating point errors from double division.
    int xj_s = i*i;
    vxl_int_64 p_xj = vcl_pow( p, (xm_sq - xj_s) );
    vxl_int_64 q_xj = vcl_pow( q, xj_s );
    vxl_int_64 G_xj = p_xj * q_xj;

    tmp_filter[center+i] = tmp_filter[center-i] = G_xj;
  }

  //do the process again.
  //This process will produce the 1st row of the nxn matrix will gives the same results
  //but allows smaller values, thus minimizing overflow risk.
  if ( tmp_filter[0] > vcl_numeric_limits<unsigned>::max() )
  {
    return -1;
  }
  filter[center] = tmp_filter[0];

  double p_q_ratio = p/q;

  for (unsigned i=1 ; i<=center; ++i)
  {
    //G(xj) = G(0)/(R0,1)^(xj^2)
    int i_sq = i*i;
    double xj_s = i_sq + rad_sq;
    vxl_int_64 G_xj = vnl_math::rnd_halfintup(G_0/( vcl_pow( p_q_ratio, xj_s) ));

    filter[center+i] = filter[center-i] = G_xj;
  }

  vxl_int_64 kernel_sum = 0;
  for (unsigned s = 0; s < filter.size(); s++)
  {
    kernel_sum += filter[ s ];
  }

  return kernel_sum;

}



//: Smooth a src_im to produce dest_im with gaussian of width sd
//  Generates gaussian filter of width sd, using (2*half_width+1)
//  values in the filter.  Typically half_width>3sd.
//  Convolves this with src_im to generate work_im, then applies filter
//  vertically to generate dest_im.
template <class srcT, class destT>
inline checked_bool gauss_filter_2d_int(const vil_image_view<srcT>& src_im,
                                vil_image_view<destT>& dest_im,
                                double sd, unsigned half_width)
{
  // Generate filter
  vcl_vector< unsigned > filter(2 * half_width + 1);
  unsigned kernel_sum = gen_int_gauss_filter(sd,filter);

  if (kernel_sum == -1 )
  {
    return false;
  }

  //test that the first iteration of the sum of the product
  // of max val of srcT and the kernel elements don't exceed the
  // size of unsigned since unsigned is the the size of our
  // temporary vil_image_view containers.
  // If it is, return false so the caller knows to use the floating-point version.
  vxl_int_64 original_sum = vcl_numeric_limits<srcT>::max() * static_cast<vxl_int_64>(kernel_sum);

  if ( original_sum >= vcl_numeric_limits<unsigned>::max() )
  {
    return false;
  }
  //since we're only normalizing at the end, make sure our biggest value
  //on the second convolve operation will fit.
  if ( original_sum*kernel_sum >= vcl_numeric_limits<vxl_int_64>::max() )
  {
    return false;
  }

  // Apply 1D convolution along i direction
  vil_image_view<unsigned> work_im;
  vil_convolve_1d(
    src_im, work_im, &filter[half_width], -int(half_width),
    half_width, unsigned(), vil_convolve_zero_extend,
    vil_convolve_zero_extend );

  // Apply 1D convolution along j direction by applying filter to transpose
  dest_im.set_size(src_im.ni(),src_im.nj(),src_im.nplanes());
  vil_image_view<unsigned> work_im_t = vil_transpose(work_im);
  vil_image_view<destT> dest_im_t = vil_transpose(dest_im);

  float norm_val = 1/( kernel_sum*static_cast<float>(kernel_sum) );
  convolve_1d(
    work_im_t, dest_im_t, &filter[half_width],
    -int(half_width),half_width, vxl_int_64(), norm_val);

  return true;
}//inline checked_bool gauss_filter_2d_int

}//namespace vdtk

#endif // _gauss_filter_h_
