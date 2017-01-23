/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/high_pass_filter.txx>

#define INSTANTIATE_HP_FILTERS( TYPE ) \
template void vidtk::box_average_1d( const vil_image_view< TYPE >& src, \
                                     vil_image_view< TYPE >& dst, \
                                     const unsigned width ); \
template void vidtk::box_high_pass_filter( const vil_image_view< TYPE >& grey_img, \
                                           vil_image_view< TYPE >& output, \
                                           const unsigned kernel_width, \
                                           const unsigned kernel_height, \
                                           const bool treat_as_interlaced ); \
template void vidtk::bidirection_box_filter( const vil_image_view< TYPE >& grey_img, \
                                             vil_image_view< TYPE >& output, \
                                             const unsigned kernel_width, \
                                             const unsigned kernel_height, \
                                             const bool treat_as_interlaced );

INSTANTIATE_HP_FILTERS( vxl_byte )
INSTANTIATE_HP_FILTERS( vxl_uint_16 )

#undef INSTANTIATE_HP_FILTERS
