/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_histogram_io.h"

#include <vsl/vsl_binary_io.h>

//************************************************************************

void vsl_b_write( vsl_b_ostream &os, vidtk::Image_Histogram_Type t )
{
  vsl_b_write( os, (int)t );
}

void vsl_b_read( vsl_b_istream &is, vidtk::Image_Histogram_Type &t )
{
  int value;
  vsl_b_read( is, value );
  t = static_cast<vidtk::Image_Histogram_Type>(value);
}


