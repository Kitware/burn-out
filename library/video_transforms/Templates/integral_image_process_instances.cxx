/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/integral_image_process.txx>

template class vidtk::integral_image_process<vxl_byte,int>;
template class vidtk::integral_image_process<vxl_byte,unsigned>;
template class vidtk::integral_image_process<vxl_byte,double>;

template class vidtk::integral_image_process<vxl_uint_16,int>;
template class vidtk::integral_image_process<vxl_uint_16,double>;
