/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/obj_specific_salient_region_classifier.txx>

template class vidtk::obj_specific_salient_region_classifier<vxl_byte,double>;
template class vidtk::obj_specific_salient_region_classifier<vxl_uint_16,double>;

template class vidtk::obj_specific_salient_region_classifier<vxl_byte,float>;
template class vidtk::obj_specific_salient_region_classifier<vxl_uint_16,float>;
