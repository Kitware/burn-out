/*ckwg +5
 * Copyright 2011-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_util.h"

#include <vil/vil_image_view_base.h>
#include <vil/vil_convert.h>

#include <logger/logger.h>

#include <klt/convolve.h>
#include <klt/klt_util.h>

#include <cstring> //memcpy

#include <cstdlib>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_klt_tracking_process_cxx__
VIDTK_LOGGER("klt_util_cxx");

namespace vidtk
{

_KLT_FloatImage klt_image_convert(vil_image_view<float> /*const*/& img);

_KLT_Pyramid
klt_pyramid_convert(vil_pyramid_image_view<float> /*const*/& pyramid)
{
  size_t const nlevels = pyramid.nlevels();
  int subsampling = 1;

  if (!nlevels)
  {
    return NULL;
  }

  if (nlevels >= 2)
  {
    double l0_scale;
    double l1_scale;

    pyramid.get_view(0, l0_scale);
    pyramid.get_view(1, l1_scale);

    // Round after the division.
    subsampling = static_cast<int>((l0_scale / l1_scale) + 0.5);
  }

#ifndef NDEBUG
  double top_scale;
  vil_image_view_base_sptr lv0_img = pyramid.get_view(0, top_scale);
  size_t const top_width = lv0_img->ni();
  size_t const top_height = lv0_img->nj();
  size_t const top_planes = lv0_img->nplanes();
  for (size_t i = 1; i < nlevels; ++i)
  {
    double l0_scale;
    double l1_scale;

    pyramid.get_view(i - 1, l0_scale);
    pyramid.get_view(i, l1_scale);

    // Round after the division.
    int const lv_subsampling = static_cast<int>((l0_scale / l1_scale) + 0.5);

    if (lv_subsampling != subsampling)
    {
      LOG_ERROR("Subsampling is not constant throughout the pyramid on level: " << i);
      return NULL;
    }

    vil_image_view<float> const& img = pyramid(i);
    size_t const width = img.ni();
    size_t const height = img.nj();
    size_t const planes = img.nplanes();

    double const local_scale = l1_scale / top_scale;

    // No rounding here (partial pixels are just dropped).
    size_t const exp_width = local_scale * top_width;
    size_t const exp_height = local_scale * top_height;

    if (width != exp_width)
    {
      LOG_ERROR("Pyramid for KLT level " << i << " does not have the correct width: "
                "Expected: " << exp_width << " "
                "Received: " << width);
      return NULL;
    }
    if (height != exp_height)
    {
      LOG_ERROR("Pyramid for KLT level " << i << " does not have the correct height: "
                "Expected: " << exp_height << " "
                "Received: " << height);
      return NULL;
    }
    if (planes != top_planes)
    {
      LOG_ERROR("Pyramid for KLT level " << i << " does not have the correct number of planes: "
                "Expected: " << top_planes << " "
                "Received: " << planes);
      return NULL;
    }
  }
#endif

  _KLT_Pyramid klt_pyramid = _KLTCreatePyramid(pyramid(0).ni(), pyramid(0).nj(), subsampling, nlevels);

  for (size_t i = 0; i < nlevels; ++i)
  {
    // Free the image made during pyramid creation
    _KLTFreeFloatImage(klt_pyramid->img[i]);
    klt_pyramid->img[i] = klt_image_convert(pyramid(i));
    klt_pyramid->ncols[i] = klt_pyramid->img[i]->ncols;
    klt_pyramid->nrows[i] = klt_pyramid->img[i]->nrows;
  }

  return klt_pyramid;
}

std::pair<vil_image_view<float>, vil_image_view<float> >
compute_gradients(vil_image_view<float> /*const*/& img, float sigma)
{
  vil_image_view<float> gradx(img.ni(), img.nj(), 1);
  vil_image_view<float> grady(img.ni(), img.nj(), 1);
  _KLT_FloatImage orig = klt_image_convert(img);
  _KLT_FloatImage klt_gradx = _KLTCreateFloatImage(img.ni(), img.nj());
  _KLT_FloatImage klt_grady = _KLTCreateFloatImage(img.ni(), img.nj());

  _KLTComputeGradients(orig, sigma, klt_gradx, klt_grady);

  for(size_t y = 0; y < img.nj(); ++y)
  {
    const size_t off = img.ni() * y;
    for(size_t x = 0; x < img.ni(); ++x)
    {
      gradx(x, y) = klt_gradx->data[off + x];
      grady(x, y) = klt_grady->data[off + x];
    }
  }

  _KLTFreeFloatImage(orig);
  _KLTFreeFloatImage(klt_gradx);
  _KLTFreeFloatImage(klt_grady);

  return std::make_pair(gradx, grady);
}

_KLT_FloatImage
klt_image_convert(vil_image_view<float> /*const*/& img)
{
  _KLT_FloatImage klt_img = _KLTCreateFloatImage(img.ni(), img.nj());

  for(size_t y = 0; y < img.nj(); ++y)
  {
    const size_t off = img.ni() * y;
    for(size_t x = 0; x < img.ni(); ++x)
    {
      klt_img->data[off + x] = img(x, y);
    }
  }

  return klt_img;
}

vil_image_view<float> klt_img_to_vil(_KLT_FloatImage const& klt_img)
{
  vil_image_view<float> result(klt_img->ncols, klt_img->nrows);
  memcpy( result.top_left_ptr(), klt_img->data, klt_img->ncols*klt_img->nrows*sizeof(float) );
  return result;
}

}
