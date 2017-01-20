/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifdef USE_VLFEAT

#include <descriptors/mfeat_bow_generator.txx>

template class vidtk::mfeat_bow_generator<vxl_byte>;
template class vidtk::mfeat_bow_generator<vxl_uint_16>;

#endif
