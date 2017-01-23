/*ckwg +5
 * Copyright 2011-2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_util.h"

#include <vil/vil_image_view_base.h>
#include <vil/vil_convert.h>

#include <klt/convolve.h>
#include <klt/klt_util.h>

namespace vidtk
{

_KLT_FloatImage klt_image_convert(vil_image_view<float> /*const*/& img);
_KLT_FloatImage klt_image_shared(vil_image_view<float> /*const*/& img);

vil_image_view<float> klt_img_to_vil(_KLT_FloatImage const& klt);

template<class PixType>
_KLT_FloatImage vil_to_klt_cast(vil_image_view<PixType> /*const*/& img)
{
  _KLT_FloatImage klt_img = _KLTCreateFloatImage(img.ni(), img.nj());

  for(size_t y = 0; y < img.nj(); ++y)
  {
    const size_t off = img.ni() * y;
    for(size_t x = 0; x < img.ni(); ++x)
    {
      klt_img->data[off + x] = static_cast<float>(img(x, y));
    }
  }

  return klt_img;
}

template <class PixType>
vil_pyramid_image_view<float>
create_klt_pyramid(vil_image_view<PixType> /*const*/& img, int levels, int subsampling, float sigma_factor, float init_sigma)
{
  vil_pyramid_image_view<float> pyramid;

  const float sigma = subsampling * sigma_factor;
  const int subhalf = subsampling / 2;
  int cols = img.ni();
  int rows = img.nj();
  int old_cols;
  double scale = 1;

  vil_image_view_base_sptr img_ptr;

  _KLT_FloatImage cur_img = vil_to_klt_cast(img);

  {
    _KLT_FloatImage smooth = _KLTCreateFloatImage(cur_img->ncols, cur_img->nrows);

    _KLTComputeSmoothedImage(cur_img, init_sigma, smooth);

    img_ptr = new vil_image_view<float>( klt_img_to_vil(smooth) );
    pyramid.add_view(img_ptr, scale);
    scale /= subsampling;

    _KLTFreeFloatImage(cur_img);
    cur_img = smooth;
  }
  for (int i = 1; i < levels; ++i)
  {
    _KLT_FloatImage smooth = _KLTCreateFloatImage(cur_img->ncols, cur_img->nrows);

    // FIXME: Inefficient (pixels tossed away below).
    _KLTComputeSmoothedImage(cur_img, sigma, smooth);

    old_cols = cols;
    cols /= subsampling;
    rows /= subsampling;
    cur_img->ncols = cols;
    cur_img->nrows = rows;

    for(int y = 0; y < rows; ++y)
    {
      const size_t off_sub = cols * y;
      const size_t off_sm = (subsampling * y + subhalf)*old_cols;
      for(int x = 0; x < cols; ++x)
      {
        cur_img->data[off_sub+x] = smooth->data[ off_sm + subsampling * x + subhalf ];
      }
    }

    img_ptr = new vil_image_view<float>( klt_img_to_vil(cur_img));
    pyramid.add_view(img_ptr, scale);

    scale /= subsampling;

    _KLTFreeFloatImage(smooth);
  }

  _KLTFreeFloatImage(cur_img);

  return pyramid;
}

} //namespace vidtk
