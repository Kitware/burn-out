/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/aligned_edge_detection.txx>

#define INSTANTIATE( TYPE1, TYPE2 ) \
template void vidtk::calculate_aligned_edges( const vil_image_view< TYPE1 >& grad_i, \
                                              const vil_image_view< TYPE1 >& grad_j, \
                                              const TYPE1 edge_threshold, \
                                              vil_image_view< TYPE2 >& output ); \
template void vidtk::calculate_aligned_edges( const vil_image_view< TYPE2 >& input, \
                                              const TYPE1 edge_threshold, \
                                              vil_image_view< TYPE2 >& output, \
                                              vil_image_view< TYPE1 >& grad_i, \
                                              vil_image_view< TYPE1 >& grad_j );

INSTANTIATE( float, vxl_byte )
INSTANTIATE( float, vxl_uint_16 )

#undef INSTANTIATE
