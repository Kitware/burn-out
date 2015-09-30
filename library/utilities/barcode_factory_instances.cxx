/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/barcode_factory.txx>
#include <vcl_vector.h>
#include <vil/vil_image_view.h>

template bool vidtk::barcode_factory::encode<vxl_byte>(
  const vcl_vector<unsigned char>&, vil_image_view<vxl_byte>, int, int ) const;
template bool vidtk::barcode_factory::decode<vxl_byte>(
  vil_image_view<vxl_byte>, vcl_vector<unsigned char>&, int, int ) const;
template bool vidtk::barcode_factory::encode<vxl_uint_16>(
  const vcl_vector<unsigned char>&, vil_image_view<vxl_uint_16>, int, int ) const;
template bool vidtk::barcode_factory::decode<vxl_uint_16>(
  vil_image_view<vxl_uint_16>, vcl_vector<unsigned char>&, int, int ) const;


