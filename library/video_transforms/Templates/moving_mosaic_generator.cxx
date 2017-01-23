/*ckwg +5
 * Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/moving_mosaic_generator.txx>

template class vidtk::moving_mosaic_generator<vxl_byte, vxl_byte>;
template class vidtk::moving_mosaic_generator<vxl_uint_16, vxl_uint_16>;

template class vidtk::moving_mosaic_generator<vxl_byte, double>;
template class vidtk::moving_mosaic_generator<vxl_uint_16, double>;
