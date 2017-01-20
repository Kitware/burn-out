/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/diff_buffer_process.txx>

#include <vector>
#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <algorithm>

template class vidtk::diff_buffer_process< vxl_byte >;
template class vidtk::diff_buffer_process<vxl_uint_16>;
