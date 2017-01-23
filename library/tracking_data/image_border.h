/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_border_
#define vidtk_image_border_

#include <vgl/vgl_box_2d.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

/// \brief A standard definition for a rectangular image border.
///
/// This object describes the terminating edge of the image border (in the
/// image plane) which seperates the border from image contents. The border
/// rectangle therefore encompases all scene pixels.
typedef vgl_box_2d<int> image_border;

/// \brief A standard definition for a non-rectangular image border.
///
/// This object is simply a binary mask image representing which pixels belong
/// to the image border, and which are part of the given scene.
typedef vil_image_view<bool> image_border_mask;


} // end namespace vidtk

#endif
