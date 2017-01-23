/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ghost_detector.h"

#include <tracking_data/image_object_util.h>

#include <vil/algo/vil_orientations.h>
#include <vil/vil_convert.h>
#include <vil/vil_math.h>

namespace vidtk
{

template < class PixType >
ghost_detector< PixType >
::ghost_detector( float min_grad_mag_var,
                  unsigned padding)
  : min_grad_mag_var_( min_grad_mag_var ),
    pixel_padding_( padding )
{
}

template < class PixType >
bool
ghost_detector< PixType >
::has_ghost_gradients( image_object_sptr obj, vil_image_view< PixType > const& img ) const
{
  vil_image_view< PixType > img_chip = clip_image_for_detection( img, obj, pixel_padding_ );
  vil_image_view< PixType > img_chip_grey;
  if( img_chip.nplanes() > 1 )
  {
    vil_convert_planes_to_grey( img_chip, img_chip_grey );
  }
  else
  {
    img_chip_grey = img_chip;
  }

  vil_image_view< float > orient_im;
  vil_image_view< float > grad_im;
  vil_orientations_from_sobel( img_chip_grey, orient_im, grad_im );

  float mean, var;
  vil_math_mean_and_variance(mean, var, grad_im, 0);

  return (var < min_grad_mag_var_);
}

} // namespace vidtk
