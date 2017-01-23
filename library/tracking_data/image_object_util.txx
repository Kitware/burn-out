/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/image_object_util.h>
#include <vil/vil_crop.h>

namespace vidtk {

template< typename T >
vil_image_view<T> clip_image_for_detection( vil_image_view< T >const& input_image,
                                            image_object_sptr const detection,
                                            unsigned border )
{
  // Establish the bounds to clip
  vgl_box_2d< unsigned > const& bbox = detection->get_bbox();
  unsigned min_x  = bbox.min_x()  - border;
  unsigned min_y  = bbox.min_y()  - border;
  unsigned width  = bbox.width()  + 2*border;
  unsigned height = bbox.height() + 2*border;

  // validate these bounds
  if( min_x >= input_image.ni() ) { min_x = 0; }
  if( min_y >= input_image.nj() ) { min_y = 0; }
  if( min_x + width >= input_image.ni() ) { width = input_image.ni()-min_x; }
  if( min_y + height >= input_image.nj() ) { height = input_image.nj()-min_y; }

  vil_image_view< T >  clipped = vil_crop( input_image, min_x, width, min_y, height );

  return clipped;
}

} // end namespace
