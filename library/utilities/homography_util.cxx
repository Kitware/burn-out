/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "homography_util.h"

#include <logger/logger.h>


namespace vidtk {

  VIDTK_LOGGER("homography_util");

// ----------------------------------------------------------------
/** Multiply homographies.
 *
 * Define template to multiply two homographies with a shared inner type.
 *
 * @tparam SrcT - source reference type
 * @tparam SharedType - shared inner type
 * @tparam DestType - destination reference type
 */
template< class SrcT, class SharedType, class DestType >
vidtk::homography_T< SrcT, DestType > operator*(vidtk::homography_T< SharedType, DestType > const & l,
                                                vidtk::homography_T< SrcT, SharedType > const & r )
{
  vidtk::homography_T< SrcT, DestType > re;
  re.set_transform(l.get_transform() * r.get_transform());
  re.set_source_reference(r.get_source_reference());
  re.set_dest_reference(l.get_dest_reference());
  re.set_valid( l.is_valid() && r.is_valid() );
  return re;
}

//
// Define macros to instantiate all allowable variants of multiplying two homographies.
//

#define SET_UP_SINGLE_MULT(X) \
  template vidtk::homography_T < X, X > operator*< X, X, X >( vidtk::homography_T < X, X > const & l, vidtk::homography_T < X, X > const & r )

#define SET_UP_PAIR_MULT( X, Y ) \
  template vidtk::homography_T < X, Y > operator*< X, X, Y >( vidtk::homography_T < X, Y > const & l, vidtk::homography_T < X, X > const & r ); \
  template vidtk::homography_T < X, Y > operator*< X, Y, Y >( vidtk::homography_T < Y, Y > const & l, vidtk::homography_T < X, Y > const & r )

#define SET_UP_MID_MULT(X,Y,Z) \
  template vidtk::homography_T < X, Z > operator*< X, Y, Z >( vidtk::homography_T < Y, Z > const & l, vidtk::homography_T < X, Y > const & r )

#define SET_UP_MULT(X,Y,Z)                      \
  SET_UP_SINGLE_MULT(X);                        \
  SET_UP_SINGLE_MULT(Y);                        \
  SET_UP_SINGLE_MULT(Z);                        \
  SET_UP_PAIR_MULT( X, Y );                     \
  SET_UP_PAIR_MULT( Y, X );                     \
  SET_UP_PAIR_MULT( X, Z );                     \
  SET_UP_PAIR_MULT( Z, X );                     \
  SET_UP_PAIR_MULT( Y, Z );                     \
  SET_UP_PAIR_MULT( Z, Y );                     \
  SET_UP_MID_MULT(X, Y, Z);                     \
  SET_UP_MID_MULT(X, Z, Y);                     \
  SET_UP_MID_MULT(Y, X, Z);                     \
  SET_UP_MID_MULT(Y, Z, X);                     \
  SET_UP_MID_MULT(Z, X, Y);                     \
  SET_UP_MID_MULT(Z, Y, X)

// Instantiate all allowable types
SET_UP_MULT(vidtk::timestamp, vidtk::plane_ref_t, vidtk::utm_zone_t);

#undef SET_UP_MULT
#undef SET_UP_MID_MULT
#undef SET_UP_PAIR_MULT
#undef SET_UP_SINGLE_MULT


// ----------------------------------------------------------------
vgl_homg_point_2d<double> operator*( vidtk::utm_to_image_homography const & l,
                                     geographic::geo_coords const & r )
{
  vgl_homg_point_2d< double > utm_pt( r.easting(), r.northing() );
  return l.get_transform()*utm_pt;
}


// ----------------------------------------------------------------
geographic::geo_coords operator*( vidtk::image_to_utm_homography const & l,
                                  vgl_homg_point_2d< double > const & r )
{
  vgl_homg_point_2d< double > utm_pt = l.get_transform() * r;
  return geographic::geo_coords( l.get_dest_reference().zone(),
                                 l.get_dest_reference().is_north(),
                                 utm_pt.x() / utm_pt.w(),
                                 utm_pt.y() / utm_pt.w());
}


// ----------------------------------------------------------------
vgl_homg_point_2d< double > operator*( vidtk::homography const & l,
                                       vgl_homg_point_2d< double > const & r )
{
  vgl_homg_point_2d< double > res = l.get_transform() * r;

  //normalize the result
  return vgl_homg_point_2d< double >( res.x() / res.w(), res.y() / res.w(), 1.0 );
}


// ----------------------------------------------------------------
// This function computes the 2x2 jacobian from a 3x3 homogrpy at the given location,
// return by reference the jacobian
bool
jacobian_from_homo_wrt_loc( vnl_matrix_fixed < double, 2, 2 > & jac,
              vnl_matrix_fixed < double, 3, 3 > const& H,
              vnl_vector_fixed < double, 2 >  const& from_loc )
{
  // The jacobian is a 2x2 matrix with entries
  // [d(f_0)/dx   d(f_0)/dy;
  //  d(f_1)/dx   d(f_1)/dy]
  //
  const double mapped_w = H(2,0) * from_loc[0] + H(2,1) * from_loc[1] + H(2,2);

  if(std::abs(mapped_w) < 1e-10)
  {
    LOG_ERROR("Degenerate case: cannot divided by zero "
               "in jacobian_from_homo_wrt_loc()");
    return false;
  }

  // w/ respect to x
  jac(0,0) = H(0,0)*( H(2,1)*from_loc[1]+H(2,2) ) - H(2,0)*( H(0,1)*from_loc[1] + H(0,2) );
  jac(1,0) = H(1,0)*( H(2,1)*from_loc[1]+H(2,2) ) - H(2,0)*( H(1,1)*from_loc[1] + H(1,2) );

  // w/ respect to y
  jac(0,1) = H(0,1)*( H(2,0)*from_loc[0]+H(2,2) ) - H(2,1)*( H(0,0)*from_loc[0] + H(0,2) );
  jac(1,1) = H(1,1)*( H(2,0)*from_loc[0]+H(2,2) ) - H(2,1)*( H(1,0)*from_loc[0] + H(1,2) );

  jac *= (1 / (mapped_w * mapped_w));

  return true;
}

} // end namespace
