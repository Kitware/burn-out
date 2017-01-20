/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "polygon_centroid.h"

namespace vidtk
{

vnl_vector_fixed< double, 2 >
polygon_centroid( vgl_polygon< double > const& poly )
{
  typedef double float_type;
  typedef vnl_vector_fixed< double, 2 > float2d_type;

  assert( poly.num_sheets() == 1 );
  vgl_polygon<float_type>::sheet_t const& sh = poly[0];

  std::size_t const n = sh.size();
  assert( n > 0 );

  float2d_type cent( 0, 0 );
  float_type A2 = 0.0;

  for( unsigned i = 0, j = n-1; i < n; j = i++ )
  {
    float_type d = sh[j].x() * sh[i].y() - sh[i].x() * sh[j].y();
    A2 += d;
    cent[0] += ( sh[j].x() + sh[i].x() ) * d;
    cent[1] += ( sh[j].y() + sh[i].y() ) * d;
  }
  cent /= (3 * A2);

  return cent;
}

}
