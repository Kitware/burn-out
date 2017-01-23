/*ckwg +5
 * Copyright 2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/extract_matrix_process.txx>
#include <utilities/homography.h>

template class vidtk::extract_matrix_process<vidtk::image_to_image_homography>;
template class vidtk::extract_matrix_process<vidtk::image_to_plane_homography>;
template class vidtk::extract_matrix_process<vidtk::plane_to_image_homography>;
template class vidtk::extract_matrix_process<vidtk::image_to_utm_homography>;
