/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/image_object.h>
#include <tracking/image_object.txx>
#include <vil/vil_image_view.h>

template vil_image_view< vxl_byte > vidtk::image_object::templ( const vil_image_view<vxl_byte>& image) const;
template vil_image_view< vxl_uint_16 > vidtk::image_object::templ( const vil_image_view<vxl_uint_16>& image) const;
template vil_image_view< vxl_byte > vidtk::image_object::templ( const vil_image_view<vxl_byte>& image, unsigned int) const;
template vil_image_view< vxl_uint_16 > vidtk::image_object::templ( const vil_image_view<vxl_uint_16>& image, unsigned int) const;
