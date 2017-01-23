/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/world_morphology_process.txx>

#include <vil/algo/vil_greyscale_erode.hxx>
VIL_GREYSCALE_ERODE_INSTANTIATE(vxl_uint_16);

#include <vil/algo/vil_greyscale_dilate.hxx>
VIL_GREYSCALE_DILATE_INSTANTIATE(vxl_uint_16);

template class vidtk::world_morphology_process< vxl_byte >;
template class vidtk::world_morphology_process< bool >;
template class vidtk::world_morphology_process< vxl_uint_16 >;
