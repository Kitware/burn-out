/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/pixel_feature_writer.txx>

template class vidtk::pixel_feature_writer< vxl_byte >;
template class vidtk::pixel_feature_writer< vxl_uint_16 >;

template bool vidtk::write_feature_images(
  const std::vector< vil_image_view< vxl_byte > >& features,
  const std::string prefix,
  const std::string postfix,
  const bool scale_features );

template bool vidtk::write_feature_images(
  const std::vector< vil_image_view< vxl_uint_16 > >& features,
  const std::string prefix,
  const std::string postfix,
  const bool scale_features );
