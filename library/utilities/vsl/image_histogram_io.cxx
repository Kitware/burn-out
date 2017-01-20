/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_histogram_io.h"

#include <vil/io/vil_io_image_view.h>

#include <vsl/vsl_binary_io.h>

//************************************************************************

void vsl_b_write( vsl_b_ostream &os, vidtk::Image_Histogram_Type t )
{
  vsl_b_write( os, static_cast<int>(t) );
}

void vsl_b_read( vsl_b_istream &is, vidtk::Image_Histogram_Type &t )
{
  int value;
  vsl_b_read( is, value );
  t = static_cast<vidtk::Image_Histogram_Type>(value);
}

void vsl_b_write( vsl_b_ostream& os,
                  const vidtk::image_histogram &h )
{
  vsl_b_write( os, h.mass() );
  vsl_b_write( os, h.type() );
  vsl_b_write( os, h.get_h() );
}

void vsl_b_read( vsl_b_istream& is,
                 vidtk::image_histogram &h )
{
  double mass;
  vsl_b_read( is, mass );
  h.set_mass(mass);

  vidtk::Image_Histogram_Type type;
  vsl_b_read( is, type );
  h.set_type(type);

  vil_image_view<double> m_h;
  vsl_b_read( is, m_h);
  h.set_h(m_h);
}
