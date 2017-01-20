/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_nearest_neighbor_inpaint_process_h_
#define vidtk_nearest_neighbor_inpaint_process_h_

#include <vil/vil_image_view.h>

namespace vidtk
{


/// A (relatively) fast simple nearest neighbor inpainting scheme. Takes as
/// input an image (RGB, greyscale, or boolean), a mask indicating the area
/// to inpaint, and an optional unsigned buffer of the same size, which
/// is only used to prevent constant reallocation of this buffer in the event
/// that we are performing multiple sequential inpaints on images of the same
/// size. Currently only supports 3 channel integral types (bool, uint8, uint16).
template< typename PixType >
void nn_inpaint( vil_image_view<PixType>& image,
                 const vil_image_view<bool>& mask,
                 vil_image_view<unsigned> status = vil_image_view<unsigned>() );


} // end namespace vidtk

#endif // vidtk_nearest_neighbor_inpaint_process_h_
