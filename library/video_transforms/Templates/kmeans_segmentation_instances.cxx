/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/kmeans_segmentation.txx>

#define INSTANTIATE( PIXTYPE, LABELTYPE ) \
\
template bool vidtk::segment_image_kmeans( const vil_image_view< PIXTYPE >& src, \
                                           vil_image_view< LABELTYPE >& labels, \
                                           const unsigned clusters, \
                                           const unsigned sample_points ); \
\
template void vidtk::label_image_from_centers( const vil_image_view< PIXTYPE >& src, \
                                               const std::vector< std::vector< PIXTYPE > >& centers, \
                                               vil_image_view< LABELTYPE >& dst ); \

INSTANTIATE( vxl_byte, vxl_byte );
INSTANTIATE( vxl_uint_16, vxl_uint_16 );

#undef INSTANTIATE
