/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kwklt_util_h_
#define vidtk_kwklt_util_h_

#include <vil/vil_image_view.h>
#include <vil/vil_pyramid_image_view.h>

#include <klt/pyramid.h>

namespace vidtk
{

_KLT_Pyramid
klt_pyramid_convert(vil_pyramid_image_view<float> /*const*/& pyramid, int subsampling);

vil_pyramid_image_view<float>
create_klt_pyramid(vil_image_view<vxl_byte> /*const*/& img, int levels, int subsampling, float sigma_factor, float init_sigma);

std::pair<vil_image_view<float>, vil_image_view<float> >
compute_gradients(vil_image_view<float> /*const*/& img, float sigma);

}

#endif
