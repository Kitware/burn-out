/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_properties/border_detection.txx>

#define INSTANTIATE( Type ) \
template void vidtk::detect_solid_borders( const vil_image_view< Type >& input_image, \
                                           const vidtk::border_detection_settings< Type >& settings, \
                                           image_border& output ); \
template void vidtk::detect_solid_borders( const vil_image_view< Type >& color_image, \
                                           const vil_image_view< Type >& gray_image, \
                                           const vidtk::border_detection_settings< Type >& settings, \
                                           image_border& output ); \
template void vidtk::detect_bw_nonrect_borders( const vil_image_view< Type >& img, \
                                                image_border_mask& output, \
                                                const Type tolerance ); \
template void vidtk::fill_border_pixels( vil_image_view< Type >& img, \
                                         const image_border& border, \
                                         const Type value );

INSTANTIATE( vxl_byte )
INSTANTIATE( vxl_uint_16 )
INSTANTIATE( double )
INSTANTIATE( float )

#undef INSTANTIATE
