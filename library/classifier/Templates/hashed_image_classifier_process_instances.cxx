/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <classifier/hashed_image_classifier_process.txx>

template class vidtk::hashed_image_classifier_process<vxl_byte,double>;
template class vidtk::hashed_image_classifier_process<vxl_byte,float>;

template class vidtk::hashed_image_classifier_process<vxl_uint_16,double>;
template class vidtk::hashed_image_classifier_process<vxl_uint_16,float>;
