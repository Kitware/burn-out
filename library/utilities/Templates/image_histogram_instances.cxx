/*ckwg +5
 * Copyright 2010-2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vxl_config.h>
#include <utilities/image_histogram.txx>

template class vidtk::image_histogram_functions< vxl_byte, float>;
template class vidtk::image_histogram_functions< vxl_byte, bool>;
template class vidtk::image_histogram_functions< vxl_uint_16, float>;
template class vidtk::image_histogram_functions< vxl_uint_16, bool>;
