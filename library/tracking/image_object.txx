/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/image_object.h>
#include <vil/vil_image_view.h>
#include <vil/vil_crop.h>

namespace vidtk
{

template< typename T >
vil_image_view<T>
image_object
::templ( const vil_image_view<T>& image ) const
{
  return vil_crop( image, bbox_.min_x(), bbox_.width(), bbox_.min_y(), bbox_.height() );
}

template< typename T >
vil_image_view<T>
image_object
::templ( const vil_image_view<T>& image, unsigned buffer ) const
{
  unsigned min_x  = bbox_.min_x()  - buffer;
  unsigned min_y  = bbox_.min_y()  - buffer;
  unsigned width  = bbox_.width()  + 2*buffer;
  unsigned height = bbox_.height() + 2*buffer;
  if(min_x >= image.ni())
  {
    min_x = 0;
  }
  if(min_y >= image.nj())
  {
    min_y = 0;
  }
  if( min_x + width >= image.ni())
  {
    width = image.ni()-min_x;
  }
  if( min_y + height >= image.nj())
  {
    height = image.nj()-min_y;
  }

//   vil_image_resource vir;
//   vir.put_view(vil_crop( image, min_x, width, min_y, height ));

  return vil_crop( image, min_x, width, min_y, height );
}

} // namespace

