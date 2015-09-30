/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_histogram_io_txx_
#define vidtk_image_histogram_io_txx_

#include "image_histogram_io.h"

#include <vsl/vsl_binary_io.h>
#include <vil/io/vil_io_image_view.h>

//************************************************************************

template< class imT, class maskT>
void vsl_b_write( vsl_b_ostream& os, 
                  const vidtk::image_histogram<imT, maskT> &h )
{
  vsl_b_write( os, h.mass() );
  vsl_b_write( os, h.type() );
  vsl_b_write( os, h.get_h() );
}

template< class imT, class maskT>
void vsl_b_read( vsl_b_istream& is, 
                 vidtk::image_histogram<imT, maskT> &h )
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

#endif

