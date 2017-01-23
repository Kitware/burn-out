/*ckwg +5
 * Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifdef USE_CAFFE

#include <object_detectors/cnn_detector.txx>

template class vidtk::cnn_detector< vxl_byte >;
template class vidtk::cnn_detector< vxl_uint_16 >;

#endif
