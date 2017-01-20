/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/frame_averaging.txx>

#define INSTATIATE( TYPE ) \
template class vidtk::online_frame_averager< TYPE >; \
template class vidtk::cumulative_frame_averager< TYPE >; \
template class vidtk::windowed_frame_averager< TYPE >; \
template class vidtk::exponential_frame_averager< TYPE >; \

INSTATIATE( vxl_byte )
INSTATIATE( vxl_uint_16 )
INSTATIATE( float )
INSTATIATE( double )

#undef INSTATIATE
