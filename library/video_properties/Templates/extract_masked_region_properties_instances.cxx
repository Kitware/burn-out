/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_properties/extract_masked_region_properties.txx>

#define INSTANTIATE( PIXTYPE )                                  \
template PIXTYPE                                                \
vidtk::compute_average( const vil_image_view< PIXTYPE >& input, \
                        const vil_image_view< bool >& mask );   \
template PIXTYPE                                                \
vidtk::compute_minimum( const vil_image_view< PIXTYPE >& input, \
                        const vil_image_view< bool >& mask );   \

INSTANTIATE( vxl_byte );
INSTANTIATE( vxl_uint_16 );
INSTANTIATE( float );
INSTANTIATE( double );

#undef INSTANTIATE
