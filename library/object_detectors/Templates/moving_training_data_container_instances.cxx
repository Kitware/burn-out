/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/moving_training_data_container.txx>

#include <vil/vil_image_view.h>

#define INSTANTIATE( TYPE ) \
template class vidtk::moving_training_data_container< TYPE >; \
template std::ostream& vidtk::operator<<( std::ostream& out, \
  const vidtk::moving_training_data_container< TYPE >& data );

INSTANTIATE( vxl_byte )
INSTANTIATE( vxl_uint_16 )
INSTANTIATE( float )
INSTANTIATE( double )

#undef INSTANTIATE
