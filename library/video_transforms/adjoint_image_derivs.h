/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef adjoint_image_derivs_h_
#define adjoint_image_derivs_h_

#include <vil/vil_image_view.h>
#include <cstdlib>

namespace vidtk
{


/// Compute the forward gradient of an image
/// produces an image with 2x the number of planes
template <typename T>
void forward_gradient(const vil_image_view<T>& src, vil_image_view<T>& dst)
{
  const unsigned ni = src.ni();
  const unsigned nj = src.nj();
  const unsigned np = src.nplanes();
  dst.set_size(ni, nj, 2*np);
  dst.fill(T(0));
  for (unsigned p = 0; p < np; ++p)
  {
    const unsigned px = 2*p;
    const unsigned py = 2*p+1;
    for (unsigned j=0; j < nj; ++j)
    {
      for (unsigned i=0; i < ni; ++i)
      {
        const double& s = src(i,j,p);
        if( i < ni-1 )
        {
          dst(i,j,px) = src(i+1,j,p) - s;
        }
        if( j < nj-1 )
        {
          dst(i,j,py) = src(i,j+1,p) - s;
        }
      }
    }
  }
}


/// Compute the backward divergence of an image
/// produces an image with 0.5x the number of planes
template <typename T>
void backward_divergence(const vil_image_view<T>& src, vil_image_view<T>& dst)
{
  const unsigned ni = src.ni();
  const unsigned nj = src.nj();
  const unsigned np = src.nplanes() / 2;
  assert((src.nplanes() > 0) && (src.nplanes()%2 == 0));
  dst.set_size(ni, nj, np);
  for (unsigned p = 0; p < np; ++p)
  {
    const unsigned px = 2*p;
    const unsigned py = 2*p+1;
    for (unsigned j=0; j < nj; ++j)
    {
      for (unsigned i=0; i < ni; ++i)
      {
        T& d = dst(i,j,p);
        d = T(0);
        if (i == 0)
          d += src(i,j,px);
        else if (i < ni - 1)
          d += src(i,j,px) - src(i-1,j,px);
        else
          d += -src(i-1,j,px);
        if (j == 0)
          d += src(i,j,py);
        else if (j < nj - 1)
          d += src(i,j,py) - src(i,j-1,py);
        else
          d += -src(i,j-1,py);
      }
    }
  }
}

} // end namespace vidtk

#endif //adjoint_image_derivs_h_
