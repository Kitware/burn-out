/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/floating_point_image_hash_process.txx>

template class vidtk::floating_point_image_hash_process< float, vxl_byte >;
template class vidtk::floating_point_image_hash_process< double, vxl_byte >;
template class vidtk::floating_point_image_hash_process< float, vxl_uint_16 >;
template class vidtk::floating_point_image_hash_process< double, vxl_uint_16 >;
template class vidtk::floating_point_image_hash_process< float, unsigned int >;
template class vidtk::floating_point_image_hash_process< double, unsigned int >;
