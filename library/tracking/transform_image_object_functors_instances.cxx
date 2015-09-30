/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vxl_config.h>
#include "transform_image_object_functors.txx"
#include <vbl/vbl_smart_ptr.txx>

template class vidtk::add_image_clip_to_image_object<vxl_byte>;

VBL_SMART_PTR_INSTANTIATE( vidtk::transform_image_object_functor<vxl_byte> );

