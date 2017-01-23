/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/blob_pixel_feature_extraction.txx>

template void vidtk::extract_blob_pixel_features( const vil_image_view<bool>& image,
                                                  const blob_pixel_features_settings& settings,
                                                  vil_image_view<vxl_byte>& output );
template void vidtk::extract_blob_pixel_features( const vil_image_view<bool>& image,
                                                  const blob_pixel_features_settings& settings,
                                                  vil_image_view<vxl_uint_16>& output );
