/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/ring_buffer.txx>

#include <vcl_vector.h>
#include <tracking/image_object.h>
#include <tracking/track.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

template class vidtk::ring_buffer< vcl_vector<vidtk::track_sptr> >;
template class vidtk::ring_buffer< vcl_vector<vidtk::image_object_sptr> >;
template class vidtk::ring_buffer< vgl_h_matrix_2d<double> >;
