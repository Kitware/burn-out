/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/color_commonality_filter.txx>

template void vidtk::color_commonality_filter( const vil_image_view<vxl_byte>& input,
                                               vil_image_view<vxl_byte>& output,
                                               const color_commonality_filter_settings& options );

template void vidtk::color_commonality_filter( const vil_image_view<vxl_uint_16>& input,
                                               vil_image_view<vxl_byte>& output,
                                               const color_commonality_filter_settings& options );

template void vidtk::color_commonality_filter( const vil_image_view<vxl_uint_16>& input,
                                               vil_image_view<vxl_uint_16>& output,
                                               const color_commonality_filter_settings& options );

template void vidtk::color_commonality_filter( const vil_image_view<vxl_byte>& input,
                                               vil_image_view<float>& output,
                                               const color_commonality_filter_settings& options );

template void vidtk::color_commonality_filter( const vil_image_view<vxl_uint_16>& input,
                                               vil_image_view<float>& output,
                                               const color_commonality_filter_settings& options );
