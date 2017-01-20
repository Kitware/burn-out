/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "compute_gsd.h"

#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_homg_point_2d.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_3x3.h>


namespace vidtk
{

// helper function forward declrations
double scale_factor( vnl_double_2 p1, 
                     vnl_double_2 p2, 
                     vnl_double_3x3 const& H_img2wld );


void transform_point( vnl_vector_fixed<double,2> const& src_loc,
                      vnl_vector_fixed<double,3> & dst_loc, 
                      vnl_double_3x3 const& H_src2dst );


double compute_gsd( vgl_box_2d<unsigned> const & img_bbox,
                    vnl_double_3x3 const& H_img2wld )
{
  double factors[4];



  // Top side
  factors[0] = scale_factor( vnl_double_2( img_bbox.min_x(), img_bbox.min_y() ), 
                             vnl_double_2( img_bbox.max_x(), img_bbox.min_y() ),
                             H_img2wld );

  // Right side
  factors[1] = scale_factor( vnl_double_2( img_bbox.max_x(), img_bbox.min_y() ),
                             vnl_double_2( img_bbox.max_x(), img_bbox.max_y() ),
                             H_img2wld );

  // Left side
  factors[2] = scale_factor( vnl_double_2( img_bbox.min_x(), img_bbox.min_y() ), 
                             vnl_double_2( img_bbox.min_x(), img_bbox.max_y() ), 
                             H_img2wld );

  // Bottom side
  factors[3] = scale_factor( vnl_double_2( img_bbox.min_x(), img_bbox.max_y() ), 
                             vnl_double_2( img_bbox.max_x(), img_bbox.max_y() ), 
                             H_img2wld );
  
  //Take the mean factor
  double sum = 0;
  for(unsigned i = 0; i < 4; ++i)
  {
    sum += factors[i];
  }
  
  double world_units_per_pixel = sum / 4;

  return world_units_per_pixel;
}

double scale_factor( vnl_double_2 p1, 
                     vnl_double_2 p2, 
                     vnl_double_3x3 const& H_img2wld )
{
  vnl_double_3 wp1, wp2;
  transform_point( p1, wp1, H_img2wld );
  transform_point( p2, wp2, H_img2wld );

  // world units (meters) / pixel
  return (wp1 - wp2).magnitude() / (p1 - p2).magnitude();
}

void transform_point( vnl_vector_fixed<double,2> const& src_loc,
                      vnl_vector_fixed<double,3> & dst_loc, 
                      vnl_double_3x3 const& H_src2dst )
{
  vnl_double_3 sp( src_loc[0], src_loc[1], 1.0 );
  vnl_double_3 dp = H_src2dst * sp;
  dst_loc[0] = dp[0] / dp[2];
  dst_loc[1] = dp[1] / dp[2];
  dst_loc[2] = 0;
}

} // end namespace vidtk
