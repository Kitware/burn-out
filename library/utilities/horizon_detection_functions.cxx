/*ckwg +5
 * Copyright 2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "horizon_detection_functions.h"

namespace vidtk
{


// Does any corner of the rectangle exist above the horizon?
//
// The function calculates whether or not either of the top two points
// in the image plane rectangle 'region' are both below the horizon. More formally:
//
// For each point, p = [ u , v ] = [ col, row ] this function first calculates whether or
// not the difference between the row coordinates of point p (v) and the closest point on
// the vanishing line Au + Bv + C = 0 is positive, where the vanishing line is derived from the
// bottom-most row of the homography (terms A=A31, B=A32, C=A33).
//
// The row coordinate of point p is:
//
//  v1 = v
//
// The v coordinate of the closest point on the vanishing line from point p is:
//
//  v2 = - (u*A*B - v*A^2 + B*C) / ( B^2 + A^2 )
//
// The difference of the two points is then:
//
//  v1 - v2 = (v*B^2 + u*A*B + B*C) / ( B^2 + A^2 )
//
// We are only interested in the sign of this expresion (above/below), which
// reduces to:
//
//  sign = ( v*B^2 + u*A*B + B*C  ) > 0
//       = B * dot( [A B C] [u v 1] ) > 0
//
// The below operation calculates an optimized version of this final expression
//
// Note that this operation makes a dangerous assumption that the camera is
// oriented upwards. This assumption may not always hold for all image orientations.
bool is_region_above_horizon( const vgl_box_2d<unsigned>& region,
                              const homography::transform_t& homog )
{
  // Confirm that there's a horizon line in the given transformation
  if( !contains_horizon_line( homog ) )
  {
    return false;
  }

  // The vanishing line derived from the homography (Au + Bv + C = 0)
  const double& a31 = homog.get( 2,0 ); //A
  const double& a32 = homog.get( 2,1 ); //B
  const double& a33 = homog.get( 2,2 ); //C

  // The shared factor for the top two points
  const double dot_partial = a32 * ( a32 * static_cast<double>(region.min_y()) + a33 );
  const double AB = a31 * a32;

  // Check left pt
  if( dot_partial + AB * static_cast<double>(region.min_x()) > 0 )
  {
    return false;
  }

  // Check right pt
  if( dot_partial + AB * static_cast<double>(region.max_x()) > 0 )
  {
    return false;
  }

  return true;
}


// Is some point above the vanishing line? Uses above derivision.
bool is_point_above_horizon( const vgl_point_2d<unsigned>& point,
                             const homography::transform_t& homog )
{
  // Confirm that there's a horizon line in the given transformation
  if( !contains_horizon_line( homog ) )
  {
    return false;
  }

  // The vanishing line derived from the homography (Au + Bv + C = 0)
  const double& a31 = homog.get( 2,0 ); //A
  const double& a32 = homog.get( 2,1 ); //B
  const double& a33 = homog.get( 2,2 ); //C

  // The shared factor for the top two points
  const double dot_product = a31*point.x() + a32*point.y() + a33;

  if( a32 * dot_product > 0 )
  {
    return false;
  }

  return true;
}


// Does a potential horizon intersection with the image? Given the resolution
// of the image, check to see if the vanishing line intersects with the
// image plane segment.
bool does_horizon_intersect( const unsigned& ni,
                             const unsigned& nj,
                             const homography::transform_t& homog )
{
  // Confirm that there's a horizon line in the given transformation
  if( !contains_horizon_line( homog ) )
  {
    return false;
  }

  const double& a31 = homog.get( 2,0 );
  const double& a32 = homog.get( 2,1 );
  const double& a33 = homog.get( 2,2 );

  // Calculate the intersection of the vanishing line and the lines comprising
  // the edges of the image, and see if any reside in the image plane
  double j_intersection = -a33/a32;

  if( j_intersection >= 0 && j_intersection <= nj )
  {
    return true;
  }

  j_intersection += ( -a31/a32 * ni );

  if( j_intersection >= 0 && j_intersection <= nj )
  {
    return true;
  }

  if( a31 != 0.0 )
  {
    double i_intersection = -a33/a31;

    if( i_intersection >= 0 && i_intersection <= ni )
    {
      return true;
    }

    i_intersection += ( -a32/a31 * nj );

    if( i_intersection >= 0 && i_intersection <= ni )
    {
      return true;
    }
  }

  return false;
}

// Does the homography contain a horizon?
bool contains_horizon_line( const homography::transform_t& homog  )
{
  return ( homog.get( 2,1 ) == 0.0 ? false : true );
}



} //namespace vidtk
