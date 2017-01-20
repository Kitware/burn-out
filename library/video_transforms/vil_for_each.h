/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_VIDEO_VIL_FOR_EACH_H
#define VIDTK_VIDEO_VIL_FOR_EACH_H

#include <vil/vil_image_view.h>

namespace vidtk
{

template <typename PixType, typename F>
F
vil_for_each(vil_image_view<PixType> const& img,
             F func)
{
  size_t const ni = img.ni();
  size_t const nj = img.nj();
  size_t const np = img.nplanes();

  ptrdiff_t const si = img.istep();
  ptrdiff_t const sj = img.jstep();
  ptrdiff_t const sp = img.planestep();

  PixType const* ip = img.top_left_ptr();

  for (size_t p = 0; p < np; ++p, ip += sp)
  {
    PixType const* ir = ip;

    for (size_t j = 0; j < nj; ++j, ir += sj)
    {
      PixType const* ic = ir;

      for (size_t i = 0; i < ni; ++i, ic += si)
      {
        func(*ic);
      }
    }
  }

  return func;
}

template <typename PixType1, typename PixType2, typename F>
F
vil_for_each_2(vil_image_view<PixType1> const& img1,
               vil_image_view<PixType2> const& img2,
               F func)
{
  size_t const ni1 = img1.ni();
  size_t const nj1 = img1.nj();
  size_t const np1 = img1.nplanes();

  assert(ni1 == img2.ni());
  assert(nj1 == img2.nj());
  assert(np1 == img2.nplanes());

  ptrdiff_t const si1 = img1.istep();
  ptrdiff_t const sj1 = img1.jstep();
  ptrdiff_t const sp1 = img1.planestep();

  ptrdiff_t const si2 = img2.istep();
  ptrdiff_t const sj2 = img2.jstep();
  ptrdiff_t const sp2 = img2.planestep();

  PixType1 const* ip1 = img1.top_left_ptr();
  PixType2 const* ip2 = img2.top_left_ptr();

  for (size_t p = 0; p < np1; ++p, ip1 += sp1, ip2 += sp2)
  {
    PixType1 const* ir1 = ip1;
    PixType2 const* ir2 = ip2;

    for (size_t j = 0; j < nj1; ++j, ir1 += sj1, ir2 += sj2)
    {
      PixType1 const* ic1 = ir1;
      PixType2 const* ic2 = ir2;

      for (size_t i = 0; i < ni1; ++i, ic1 += si1, ic2 += si2)
      {
        func(*ic1, *ic2);
      }
    }
  }

  return func;
}

}

#endif // VIDTK_VIDEO_VIL_FOR_EACH_H
