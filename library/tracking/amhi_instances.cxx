/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vxl_config.h>
#include <vbl/vbl_smart_ptr.hxx>
#include "amhi.txx"

template class vidtk::amhi< vxl_byte >;

//Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::amhi< vxl_byte > );
