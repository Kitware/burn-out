/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_properties/windowed_mean_image_process.txx>

#include <vxl_config.h>

template class vidtk::windowed_mean_image_process< vxl_byte, double >;
template class vidtk::windowed_mean_image_process< vxl_uint_16, double >;
