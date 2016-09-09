/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/paired_buffer_process.txx>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <vxl_config.h>

namespace vidtk
{

template class paired_buffer_process< timestamp,
                                      vil_image_view< vxl_byte > >;
template class paired_buffer_process< timestamp,
                                      double >;

}
