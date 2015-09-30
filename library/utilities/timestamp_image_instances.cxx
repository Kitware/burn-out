/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/timestamp_image.h>

#include <utilities/timestamp_image.txx>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>

template vidtk::timestamp vidtk::get_timestamp(
  const vil_image_view< vxl_byte >, vil_image_view< vxl_byte >&, int, bool );
template bool vidtk::add_timestamp(
  const vil_image_view< vxl_byte >, vil_image_view< vxl_byte >&, bool, int );
template bool vidtk::add_timestamp(
  const vil_image_view< vxl_byte >, vil_image_view< vxl_byte >&, const vidtk::timestamp&, bool, int );

template vidtk::timestamp vidtk::get_timestamp(
  const vil_image_view< vxl_uint_16 >, vil_image_view< vxl_uint_16 >&, int, bool );
template bool vidtk::add_timestamp(
  const vil_image_view< vxl_uint_16 >, vil_image_view< vxl_uint_16 >&, bool, int );
template bool vidtk::add_timestamp(
  const vil_image_view< vxl_uint_16 >, vil_image_view< vxl_uint_16 >&, const vidtk::timestamp&, bool, int );


