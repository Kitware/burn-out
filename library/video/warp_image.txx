/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "warp_image.h"

#include <vcl_cassert.h>
#include <vcl_vector.h>
#include <vil/vil_bilin_interp.h>
#include <vnl/vnl_inverse.h>
#include <vnl/vnl_double_3.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_intersection.h>

namespace
{

bool is_identity( vgl_h_matrix_2d<double> const& H )
{
  vnl_matrix_fixed<double,3,3> const& M = H.get_matrix();
  return ( M(0,1) == 0.0 && M(0,2) == 0.0 &&
           M(1,0) == 0.0 && M(1,2) == 0.0 &&
           M(2,0) == 0.0 && M(2,1) == 0.0 &&
           M(0,0) == M(1,1) && M(1,1) == M(2,2) );
}

}


namespace vidtk
{

// Helper routine.  Defined in .cxx
bool
warp_image_is_identity( vgl_h_matrix_2d<double> const& H );


template<class T>
bool
warp_image( vil_image_view<T> const& src,
            vil_image_view<T>& dest,
            vgl_h_matrix_2d<double> const& dest_to_src_homography,
            vil_image_view< bool > * const unmapped_mask )
{
  return warp_image( src, dest, dest_to_src_homography, 0, 0, unmapped_mask );
}


template<class T>
bool
warp_image( vil_image_view<T> const& src,
            vil_image_view<T>& dest,
            vgl_h_matrix_2d<double> const& dest_to_src_homography,
            int off_i, int off_j,
            vil_image_view< bool > * const unmapped_mask )
{
  return warp_image( src,
                     dest,
                     dest_to_src_homography,
                     warp_image_parameters().set_offset( off_i, off_j ),
                     unmapped_mask );
}


template<class T>
bool
warp_image( vil_image_view<T> const& src,
            vil_image_view<T>& dest,
            vgl_h_matrix_2d<double> const& dest_to_src_homography,
            warp_image_parameters const& param,
            vil_image_view< bool > * const unmapped_mask_ptr )
{
  // Retrieve destination and source image properties
  unsigned const dni = dest.ni();
  unsigned const dnj = dest.nj();
  unsigned const dnp = dest.nplanes();
  unsigned const sni = src.ni();
  unsigned const snj = src.nj();
  unsigned const snp = src.nplanes();

  // Source and destination must have the same number of channels
  assert( snp == dnp );

  // Cast unmapped parameter value
  T const unmapped_value = static_cast<T>( param.unmapped_value_ );

  // Special check for "simple" homographies.
  if( is_identity( dest_to_src_homography ) &&
      dni == sni &&
      dnj == snj )
  {
    if( param.shallow_copy_okay_ )
    {
      dest = src;
    }
    else
    {
      dest.deep_copy( src );
    }
    if( unmapped_mask_ptr )
    {
      unmapped_mask_ptr->fill( false );
    }
    return true;
  }

  typedef vgl_homg_point_2d<double> homog_type;
  typedef vgl_point_2d<double> point_type;
  typedef vgl_box_2d<double> box_type;

  // First, figure out the bounding box of src projected into dest.
  // There may be considerable computation saving for the cases when
  // the output image is much larger that the projected source image,
  // which will often occur during mosaicing.

  vgl_h_matrix_2d<double> const& src_to_dest_homography(
    vnl_inverse( dest_to_src_homography.get_matrix() ) );

  box_type src_on_dest_bounding_box;

  homog_type cnrs[4] = { homog_type( 0, 0 ),
                         homog_type( sni - 1, 0 ),
                         homog_type( sni - 1, snj - 1 ),
                         homog_type( 0, snj - 1 ) };

  for( unsigned i = 0; i < 4; ++i )
  {
    // Shift the point to destination image pixel index coordinates
    point_type p = src_to_dest_homography * cnrs[i];
    p.x() -= param.off_i_;
    p.y() -= param.off_j_;
    src_on_dest_bounding_box.add( p );
  }

  // Calculate intersection with destination pixels we are looking to fill
  box_type dest_boundaries( 0, dni - 1, 0, dnj - 1 );
  box_type intersection = vgl_intersection( src_on_dest_bounding_box, dest_boundaries );

  // Fill in unmapped mask and destination with default values. Maybe
  // implement a couple of loops to fill only the "boundary" regions with
  // default values later.
  if( param.fill_unmapped_ )
  {
    dest.fill( unmapped_value );
  }
  if( unmapped_mask_ptr )
  {
    unmapped_mask_ptr->fill( true );
  }

  // Determine if this is a special case (source and destination images
  // overlap at only a single exact point). We can handle this case better
  // later.
  bool point_intercept = false;
  if( intersection.min_x() == intersection.max_x() &&
      intersection.min_y() == intersection.max_y() )
  {
    point_intercept = true;
  }

  // Exit on invalid case, or else semi-optimized warp will fail. This
  // condition should only occur if we exceed the image bounds given the
  // above computation (ie one image maps to a region completely outside
  // the other, or there is less than a 1 pixel overlap)
  if( intersection.width() == 0 && intersection.height() == 0 && !point_intercept)
  {
    return false;
  }

  // Fill in the appropriate pixels in the destination image by mapping
  // every pixel in the destination image to its location in the source,
  // and bilinearly interpret its respective value

  // Note: This operation is optimized such that we don't compute the
  // full homography matrix multiplication for every pixel in the
  // destination image, and instead only have to add a row and column
  // component for each (i,j). It could be further optimized in a few ways,
  // however, for now the bilinear interpret is the largest bottleneck
  // to performance in many, but not all, instances.

  // Bounds minus 1, precomputed for efficiency (we perform multiple
  // checks against these values later in order to determine spec cases)
  const int src_ni_m1 = sni - 1;
  const int src_nj_m1 = snj - 1;

  // Extract start and end, row/col scanning ranges [start..end-1]
  const int start_j = std::floor(intersection.min_y());
  const int start_i = std::floor(intersection.min_x());
  const int end_j = std::floor(intersection.max_y()+1);
  const int end_i = std::floor(intersection.max_x()+1);

  // Create adjusted start and end scanning values for supplied offset
  const int start_j_adj = start_j + param.off_j_;
  const int start_i_adj = start_i + param.off_i_;
  const int end_j_adj = end_j + param.off_j_;
  const int end_i_adj = end_i + param.off_i_;

  // Get pointers to image data and retrieve required step values
  T* row_start = dest.top_left_ptr();
  const T* src_start = src.top_left_ptr();
  const vcl_ptrdiff_t dest_j_step = dest.jstep();
  const vcl_ptrdiff_t src_j_step = src.jstep();
  const vcl_ptrdiff_t dest_i_step = dest.istep();
  const vcl_ptrdiff_t src_i_step = src.istep();
  const vcl_ptrdiff_t dest_p_step = dest.planestep();
  const vcl_ptrdiff_t src_p_step = src.planestep();
  const T* src_p_end = src_start + dnp * dest_p_step;

  // Adjust destination itr position to start of bounding box
  row_start = row_start + start_j * dest_j_step + dest_i_step * start_i;

  // Precompute partial column homography values
  int factor_size = end_i_adj - start_i_adj;

  vnl_matrix_fixed<double,3,3> homog = dest_to_src_homography.get_matrix();
  vnl_double_3 homog_col_1 = homog.get_column( 0 );
  vnl_double_3 homog_col_2 = homog.get_column( 1 );
  vnl_double_3 homog_col_3 = homog.get_column( 2 );

  vcl_vector< vnl_double_3 > col_factors( factor_size );

  for( int i = 0; i < factor_size; i++ )
  {
    double col = double( i + start_i_adj );
    col_factors[ i ] = col * homog_col_1 + homog_col_3;
  }

  // Perform scan of boxed region
  for( int j = start_j_adj; j < end_j_adj; j++, row_start += dest_j_step )
  {

    // dest_col_ptr now points to the start of the BB region for this row
    T* dest_col_ptr = row_start;

    // Precompute row homography partials for this row
    const vnl_double_3 row_factor = homog_col_2 * (double)j;

    // Get pointer to start of precomputed column values
    vnl_double_3* col_factor_ptr = &col_factors[0];

    // Iterate through each column in the BB
    for( int i = start_i_adj; i < end_i_adj; i++, col_factor_ptr++, dest_col_ptr += dest_i_step )
    {

      // Compute homography mapping for this point (dest->src)
      vnl_double_3 pt = row_factor + (*col_factor_ptr);

      // Normalize by dividing out third term
      double& x = pt[0];
      double& y = pt[1];
      double& w = pt[2];

      x /= w;
      y /= w;

      // Check if we can perform interp at this point
      if( !(x < 0 || y < 0 || x > src_ni_m1 || y > src_nj_m1) )
      {

        // For each channel interpolate from src
        const T* src_plane = src_start;
        T* dest_pixel_ptr = dest_col_ptr;
        for( ; src_plane < src_p_end; src_plane += src_p_step, dest_pixel_ptr += dest_p_step)
        {
          double v = vil_bilin_interp_raw( x, y, src_plane, src_i_step, src_j_step );
          *dest_pixel_ptr = T(v);
        }

        // If using an optional mask, mark corresp. value
        if( unmapped_mask_ptr )
        {
          (*unmapped_mask_ptr)(i-param.off_i_, j-param.off_j_) = false;
        }
      }
    }
  }

  return true;
}


} // end namespace vidtk
