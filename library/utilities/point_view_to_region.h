/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_utilities_point_view_to_region_
#define vidtk_utilities_point_view_to_region_


#include <vil/vil_image_view.h>
#include <vil/vil_crop.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_intersection.h>

namespace vidtk
{


// Misc functions to assist with pointing an image view to some sub-ROI
// given by a vgl rectangle describing the ROI location


// Helper typedefs
typedef vgl_box_2d< unsigned > image_region;


// Intersect a region with some image boundaries
template< typename PixType >
void check_region_boundaries( vgl_box_2d< int >& bbox, const vil_image_view< PixType >& img )
{
  vgl_box_2d<int> boundaries( 0, img.ni(), 0, img.nj() );
  bbox = vgl_intersection( boundaries, bbox );
}

template< typename PixType >
void check_region_boundaries( vgl_box_2d< unsigned >& bbox, const vil_image_view< PixType >& img )
{
  if( bbox.max_x() > img.ni() )
  {
    bbox.set_max_x( img.ni() );
  }
  if( bbox.min_x() > img.ni() )
  {
    bbox.set_min_x( img.ni() );
  }
  if( bbox.max_y() > img.nj() )
  {
    bbox.set_max_y( img.nj() );
  }
  if( bbox.min_y() > img.nj() )
  {
    bbox.set_min_y( img.nj() );
  }
}

// Point an image view to a rectangular region in the image
template< typename PixType, typename BoxType >
void point_view_to_region( const vil_image_view< PixType >& src,
                           const vgl_box_2d< BoxType >& region,
                           vil_image_view< PixType >& dst )
{
  // Early exit case, no crop required
  if( region.min_x() == 0 && region.min_y() == 0 &&
      region.max_x() == static_cast<BoxType>( src.ni() ) &&
      region.max_y() == static_cast<BoxType>( src.nj() ) )
  {
    dst = src;
    return;
  }

  // Validate boundaries
  vgl_box_2d< BoxType > to_crop = region;
  check_region_boundaries( to_crop, src );

  // Make sure width and height are non-zero
  if( to_crop.width() == 0 || to_crop.height() == 0 )
  {
    return;
  }

  // Perform crop
  dst = vil_crop( src,
                  to_crop.min_x(),
                  to_crop.width(),
                  to_crop.min_y(),
                  to_crop.height() );
}

// Alternative call for the above
template< typename PixType, typename BoxType >
vil_image_view< PixType > point_view_to_region( const vil_image_view< PixType >& src,
                                                const vgl_box_2d< BoxType >& region )
{
  vil_image_view< PixType > output;
  point_view_to_region( src, region, output );
  return output;
}

// Function which returns an image view of the even rows of an image
template <typename PixType>
inline vil_image_view<PixType> even_rows(const vil_image_view<PixType>& im)
{
  return vil_image_view<PixType>( im.memory_chunk(), im.top_left_ptr(),
                                  im.ni(), (im.nj()+1)/2, im.nplanes(),
                                  im.istep(), im.jstep()*2, im.planestep() );
}


// Function which returns an image view of the odd rows of an image
template <typename PixType>
inline vil_image_view<PixType> odd_rows(const vil_image_view<PixType>& im)
{
  return vil_image_view<PixType>( im.memory_chunk(), im.top_left_ptr()+im.jstep(),
                                  im.ni(), (im.nj())/2, im.nplanes(),
                                  im.istep(), im.jstep()*2, im.planestep() );
}

} // namespace vidtk

#endif // vidtk_utilities_point_view_to_region_
