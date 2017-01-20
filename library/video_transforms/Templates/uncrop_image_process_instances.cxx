/*ckwg +5
 * Copyright 2011-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vxl_config.h>

#include <video_transforms/uncrop_image_process.txx>

template class vidtk::uncrop_image_process<vxl_byte>;
template class vidtk::uncrop_image_process<vxl_uint_16>;
template class vidtk::uncrop_image_process<bool>;
