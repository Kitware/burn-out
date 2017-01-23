/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/percentile_image.txx>

#define INSTANTIATE( TYPE ) \
template void vidtk::sample_and_sort_image( const vil_image_view< TYPE >& src, \
                                            std::vector< TYPE >& dst, \
                                            unsigned sampling_points ); \
template void vidtk::get_image_percentiles( const vil_image_view< TYPE >& src, \
                                            const std::vector< double >& percentiles, \
                                            std::vector< TYPE >& dst, \
                                            unsigned sampling_points ); \
template void vidtk::percentile_threshold_above( const vil_image_view< TYPE >& src, \
                                                 const std::vector< double >& percentiles, \
                                                 vil_image_view< bool >& dst, \
                                                 unsigned sampling_points ); \
template void vidtk::percentile_threshold_above( const vil_image_view< TYPE >& src, \
                                                 const double percentile, \
                                                 vil_image_view< bool >& dst, \
                                                 unsigned sampling_points ); \

INSTANTIATE( vxl_byte );
INSTANTIATE( vxl_uint_16 );
INSTANTIATE( float );
INSTANTIATE( double );

#undef INSTANTIATE
