/*ckwg +5
 * Copyright 2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <kwklt/klt_util.txx>

template vil_pyramid_image_view<float>
vidtk::create_klt_pyramid<vxl_byte>(vil_image_view<vxl_byte> /*const*/& img, int levels, int subsampling, float sigma_factor, float init_sigma);

template vil_pyramid_image_view<float>
vidtk::create_klt_pyramid<vxl_uint_16>(vil_image_view<vxl_uint_16> /*const*/& img, int levels, int subsampling, float sigma_factor, float init_sigma);
