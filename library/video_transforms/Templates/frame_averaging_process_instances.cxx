/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/frame_averaging_process.txx>

template class vidtk::frame_averaging_process<vxl_byte,double>;
template class vidtk::frame_averaging_process<vxl_byte,float>;
template class vidtk::frame_averaging_process<vxl_byte,vxl_byte>;

template class vidtk::frame_averaging_process<vxl_uint_16,double>;
template class vidtk::frame_averaging_process<vxl_uint_16,float>;
template class vidtk::frame_averaging_process<vxl_uint_16,vxl_uint_16>;
