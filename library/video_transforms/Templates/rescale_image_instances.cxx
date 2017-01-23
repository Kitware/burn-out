/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/rescale_image_process.txx>

#include <vxl_config.h>

template class vidtk::rescale_image_process< bool >;
template class vidtk::rescale_image_process< vxl_byte >;
template class vidtk::rescale_image_process< vxl_uint_16 >;
