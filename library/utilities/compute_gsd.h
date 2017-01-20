/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gsd_h_
#define vidtk_gsd_h_

#include <vnl/vnl_double_3x3.h>
#include <vgl/vgl_box_2d.h>

namespace vidtk
{

// Computing GSD by taking a mean GSD for top, bottom, 
// left and right of the provided rectangle. Input parameters: 
// Image bounding box, and  image-to-world (in meters) homography
// transformation.
double compute_gsd( vgl_box_2d<unsigned> const & img_box,
                    vnl_double_3x3 const& H_img2wld );

} // end namespace vidtk

#endif // vidtk_gsd_h_
