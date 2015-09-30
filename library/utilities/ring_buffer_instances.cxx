/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/ring_buffer.txx>

#include <vil/vil_image_view.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>

template class vidtk::ring_buffer< double >;
template class vidtk::ring_buffer< vidtk::timestamp >;
template class vidtk::ring_buffer< vil_image_view< vxl_byte > >;
template class vidtk::ring_buffer< vidtk::image_to_image_homography >;
template class vidtk::ring_buffer< vidtk::image_to_utm_homography >;
