/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_util.h"

#include <vil/vil_image_view_base.h>
#include <vil/vil_convert.h>

#include <klt/convolve.h>
#include <klt/klt_util.h>

#include <cstdlib>

namespace vidtk
{

static _KLT_FloatImage klt_image_convert(vil_image_view<float> /*const*/& img);
static _KLT_FloatImage klt_image_shared(vil_image_view<float> /*const*/& img);


_KLT_Pyramid
klt_pyramid_convert(vil_pyramid_image_view<float> /*const*/& pyramid, int subsampling)
{
  size_t const nlevels = pyramid.nlevels();

  _KLT_Pyramid klt_pyramid = _KLTCreatePyramid(pyramid(0).ni(), pyramid(0).nj(), subsampling, nlevels);

  for (size_t i = 0; i < nlevels; ++i)
  {
    // Free the image made during pyramid creation
    _KLTFreeFloatImage(klt_pyramid->img[i]);
    klt_pyramid->img[i] = klt_image_convert(pyramid(nlevels - i - 1));
    klt_pyramid->ncols[i] = klt_pyramid->img[i]->ncols;
    klt_pyramid->nrows[i] = klt_pyramid->img[i]->nrows;
  }

  return klt_pyramid;
}

vil_pyramid_image_view<float>
create_klt_pyramid(vil_image_view<vxl_byte> /*const*/& img, int levels, int subsampling, float sigma_factor, float init_sigma)
{
  vil_pyramid_image_view<float> pyramid;

  const float sigma = subsampling * sigma_factor;
  const int subhalf = subsampling / 2;
  int cols = img.ni();
  int rows = img.nj();
  double scale = 1;

  vil_image_view_base_sptr img_ptr;

  vil_image_view<float> prev_img;

  vil_convert_cast(img, prev_img);

  {
    vil_image_view<float> smoothed_img(cols, rows, 1);
    _KLT_FloatImage orig = klt_image_shared(prev_img);
    _KLT_FloatImage smooth = klt_image_shared(smoothed_img);

    _KLTComputeSmoothedImage(orig, init_sigma, smooth);

    img_ptr = new vil_image_view<float>(smoothed_img);
    pyramid.add_view(img_ptr, scale);

    prev_img = smoothed_img;

    free(orig);
    free(smooth);
  }

  for (int i = 1; i < levels; ++i)
  {
    vil_image_view<float> smoothed_img(cols, rows, 1);
    _KLT_FloatImage orig = klt_image_shared(prev_img);
    _KLT_FloatImage smooth = klt_image_shared(smoothed_img);

    _KLTComputeSmoothedImage(orig, sigma, smooth);

    cols /= subsampling;
    rows /= subsampling;
    vil_image_view<float> subsampled_img(cols, rows, 1);

    for(int y = 0; y < rows; ++y)
    {
      for(int x = 0; x < cols; ++x)
      {
        subsampled_img(x, y) = smoothed_img(subsampling * x + subhalf,
                                            subsampling * y + subhalf);
      }
    }

    img_ptr = new vil_image_view<float>(subsampled_img);
    pyramid.add_view(img_ptr, scale);

    scale /= subsampling;
    prev_img = subsampled_img;

    free(orig);
    free(smooth);
  }

  return pyramid;
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

_KLT_FloatImage
klt_image_shared(vil_image_view<float> /*const*/& img)
{
  _KLT_FloatImage klt_img = (_KLT_FloatImage)malloc(sizeof(_KLT_FloatImageRec));

  klt_img->ncols = img.ni();
  klt_img->nrows = img.nj();
  klt_img->data = img.top_left_ptr();

  return klt_img;
}

}
