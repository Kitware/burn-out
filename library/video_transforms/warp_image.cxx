/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{


bool
warp_image_is_identity( vgl_h_matrix_2d<double> const& H )
{
  vnl_matrix_fixed<double,3,3> const& M = H.get_matrix();
  return ( M(0,1) == 0.0 && M(0,2) == 0.0 &&
           M(1,0) == 0.0 && M(1,2) == 0.0 &&
           M(2,0) == 0.0 && M(2,1) == 0.0 &&
           M(0,0) == M(1,1) && M(1,1) == M(2,2) );
}


} // end namespace vidtk
