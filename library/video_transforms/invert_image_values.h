/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_invert_image_values_h_
#define vidtk_invert_image_values_h_


#include <vil/vil_image_view.h>
#include <vil/vil_transform.h>

#include <limits>

namespace vidtk
{

/// For an individual pixel value, invert it's value.
template <typename PixType>
PixType invert_pixel_value( PixType& val )
{
  return std::numeric_limits< PixType >::max() - val;
}

/// For an individual pixel value, negate it's value.
template <typename PixType>
PixType negate_pixel_value( PixType& val )
{
  return -val;
}

/// \brief Invert all values in an input image.
///
/// For unsigned types, this will subtract each pixel value by the
/// maximum pixel value for each respective type of image. For signed
/// types, this will simply negate all values in the image.
///
/// This can be useful in a few circumstances, such as normalizing IR
/// hot or cold operating modes to appear similarly. Additionally it is
/// useful for experimentation purposes.
template <typename PixType>
void invert_image( vil_image_view< PixType >& img )
{
  if( std::numeric_limits<PixType>::is_signed )
  {
    vil_transform( img, negate_pixel_value< PixType > );
  }
  else
  {
    vil_transform( img, invert_pixel_value< PixType > );
  }
}

} // end namespace vidtk

#endif // vidtk_invert_image_values_h_
