/*ckwg +5
 * Copyright 2011-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vxl_config.h>
#include <object_detectors/transform_image_object_functors.txx>
#include <vbl/vbl_smart_ptr.txx>

template class vidtk::add_image_clip_to_image_object<vxl_byte>;

VBL_SMART_PTR_INSTANTIATE( vidtk::transform_image_object_functor<vxl_byte> );

template class vidtk::add_image_clip_to_image_object<vxl_uint_16>;;

VBL_SMART_PTR_INSTANTIATE( vidtk::transform_image_object_functor<vxl_uint_16> );
