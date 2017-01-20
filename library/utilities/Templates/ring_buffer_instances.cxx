/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/ring_buffer.txx>

#include <vgl/vgl_box_2d.h>
#include <vil/vil_image_view.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>

template class vidtk::ring_buffer< int >;
template class vidtk::ring_buffer< double >;
template class vidtk::ring_buffer< vgl_box_2d<int> >;
template class vidtk::ring_buffer< vidtk::timestamp >;
template class vidtk::ring_buffer< vil_image_view< vxl_byte > >;
template class vidtk::ring_buffer< vil_image_view< vxl_uint_16 > >;
template class vidtk::ring_buffer< vil_image_view< double > >;
template class vidtk::ring_buffer< vil_image_view< float > >;
template class vidtk::ring_buffer< vil_image_view< bool > >;
template class vidtk::ring_buffer< vidtk::image_to_image_homography >;
template class vidtk::ring_buffer< vidtk::image_to_utm_homography >;
