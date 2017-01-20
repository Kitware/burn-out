/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/convert_color_space.txx>

#define INITIALIZE_TYPE( TYPE ) \
\
template bool vidtk::is_conversion_supported< TYPE >( const color_space src, \
                                                      const color_space dst ); \
\
template bool vidtk::convert_color_space( const vil_image_view< TYPE >& src, \
                                          vil_image_view< TYPE >& dst, \
                                          const color_space src_space, \
                                          const color_space dst_space ); \
\
template void vidtk::swap_planes( const vil_image_view< TYPE >& src, \
                                  vil_image_view< TYPE >& dst, \
                                  const unsigned plane1, \
                                  const unsigned plane2 ); \


INITIALIZE_TYPE( vxl_byte );
INITIALIZE_TYPE( vxl_uint_16 );
INITIALIZE_TYPE( float );

#undef INITIALIZE_TYPE
