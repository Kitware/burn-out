/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifdef USE_GDAL

#include <video_io/gdal_nitf_writer.txx>

template class vidtk::gdal_nitf_writer<vxl_byte>;
//template class vidtk::gdal_nitf_writer<vxl_uint_16>; Commented for now, the writer needs more work to properly support this mode.

#endif
