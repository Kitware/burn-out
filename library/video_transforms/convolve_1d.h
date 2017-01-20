/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _convolve_1d_h_
#define _convolve_1d_h_

//  convolve_1d.h was adapted from the code in vil_convolve_1d.h
//  Original file header listed below for clarity.
//  This file differs from the vil version in that
//  1) it is a scaled down version which includes only those pieces
//     required in vidtk
//  2) allows the user to pass a normalization value for use with
//     integer based gaussian kernels.


//:
// \file
// \brief 1D Convolution with cunning boundary options
// \author Tim Cootes, Ian Scott (based on work by fsm)
//
// \note  The convolution operation is defined by
//    $(f*g)(x) = \int f(x-y) g(y) dy$
// i.e. the kernel g is reflected before the integration is performed.
// If you don't want this to happen, the behaviour you want is not
// called "convolution". So don't break the convolution routines in
// that particular way.

#include <cassert>
#include <vil/vil_image_view.h>

// the value of norm is the inverse of the sum of the values in the kernel squared.
// This values is multiplies to the convolution sum for normalization.
// Note* We are multiplying the inverse rather than dividing for performance reasons.
template <class srcT, class destT, class accumT>
inline void convolve_edge_1d(const srcT* src, unsigned /*n*/, std::ptrdiff_t s_step,
                                 destT* dest, std::ptrdiff_t d_step,
                                 const unsigned* kernel,
                                 std::ptrdiff_t k_lo, std::ptrdiff_t k_hi,
                                 std::ptrdiff_t kstep, accumT /*ac*/, float norm)
{
  for (std::ptrdiff_t i=0;i<k_hi;++i,dest+=d_step)
  {
    accumT sum = 0;
    const srcT* s = src;
    const unsigned* k = kernel+i*kstep;
    for (std::ptrdiff_t j=i;j>=k_lo;--j,s+=s_step,k-=kstep)
    {
      sum+= static_cast<accumT>((static_cast<accumT>((*s))*(*k)));
    }
    *dest = destT(sum*norm);
  }

  return;
}

// the value of norm is the inverse of the sum of the values in the kernel squared.
// This values is multiplies to the convolution sum for normalization.
// Note* We are multiplying the inverse rather than dividing for performance reasons.
template <class srcT, class destT, class accumT>
inline void convolve_1d(const srcT* src0, unsigned nx, std::ptrdiff_t s_step,
                            destT* dest0, std::ptrdiff_t d_step,
                            const unsigned* kernel,
                            std::ptrdiff_t k_lo, std::ptrdiff_t k_hi,
                            accumT ac, float norm)
{
  assert(k_hi - k_lo < static_cast<std::ptrdiff_t>(nx));

  // Deal with start (fill elements 0..1+k_hi of dest)
  convolve_edge_1d(src0,nx,s_step,dest0,d_step,kernel,k_lo,k_hi,1, ac, norm);

  const unsigned* k_rbegin = kernel+k_hi;
  const unsigned* k_rend   = kernel+k_lo-1;
  assert(k_rbegin >= k_rend);
  const srcT* src = src0;

  for (destT       * dest = dest0 + d_step*k_hi,
       * const   end_dest = dest0 + d_step*(static_cast<unsigned>(nx)+k_lo);
       dest!=end_dest;
       dest+=d_step,src+=s_step)
  {
    accumT sum = 0;
    const srcT* s= src;
    for (const unsigned *k = k_rbegin;k!=k_rend;--k,s+=s_step)
    {
      sum+= static_cast<accumT>(((*k))*static_cast<accumT>((*s)));
    }

    *dest = destT(sum*norm);
  }

  // Deal with end  (reflect data and kernel!)
  convolve_edge_1d(src0+(nx-1)*s_step,nx,-s_step,
                   dest0+(nx-1)*d_step,-d_step,
                   kernel,-k_hi,-k_lo,-1, ac, norm);
}


// the value of norm is the inverse of the sum of the values in the kernel squared.
// This values is multiplies to the convolution sum for normalization.
// Note* We are multiplying the inverse rather than dividing for performance reasons.
template <class srcT, class destT, class accumT>
inline void convolve_1d(const vil_image_view<srcT>& src_im,
                        vil_image_view<destT>& dest_im,
                        const unsigned* kernel,
                        std::ptrdiff_t k_lo, std::ptrdiff_t k_hi,
                        accumT ac, float norm )
{
  unsigned n_i = src_im.ni();
  unsigned n_j = src_im.nj();
  assert(k_hi - k_lo +1 <= static_cast<std::ptrdiff_t> (n_i));
  std::ptrdiff_t s_istep = src_im.istep(), s_jstep = src_im.jstep();

  dest_im.set_size(n_i,n_j,src_im.nplanes());
  std::ptrdiff_t d_istep = dest_im.istep(),d_jstep = dest_im.jstep();

  for (unsigned int p=0;p<src_im.nplanes();++p)
  {
    // Select first row of p-th plane
    const srcT*  src_row  = src_im.top_left_ptr() + p*src_im.planestep();
    destT* dest_row = dest_im.top_left_ptr() + p*dest_im.planestep();

    for (unsigned int j = 0; j < n_j; ++j, src_row += s_jstep, dest_row += d_jstep)
    {
      convolve_1d(src_row, n_i, s_istep, dest_row, d_istep, kernel, k_lo, k_hi, ac, norm);
    }
  }
}

#endif // convolve_1d_h_
