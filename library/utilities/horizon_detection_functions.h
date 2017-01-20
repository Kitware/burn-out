/*ckwg +5
 * Copyright 2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_utilities_horizon_detection_functions_h_
#define vidtk_utilities_horizon_detection_functions_h_

#include <utilities/homography.h>
#include <utilities/compute_gsd.h>

#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_point_2d.h>



namespace vidtk
{

/// Does a given homography contain a non-vertical vanishing line?
bool contains_horizon_line( const homography::transform_t& homog  );



/// Given an image to world homography, and a rectangle region in the image
/// plane, calculates if any points in the region is above a horizon (vanishing
/// line) in the given homography. By "above" we assume that the camera is
/// vertically aligned, and a lower j value in the image corresponds to "up".
bool is_region_above_horizon( const vgl_box_2d<unsigned>& region,
                              const homography::transform_t& homog );



/// Is a single point (in the image plane) above the horizon?
bool is_point_above_horizon( const vgl_point_2d<unsigned>& point,
                             const homography::transform_t& homog );



/// Given the resolution of some image [0, 0, ni, nj] calculates if
/// the horizon line derived from the given homography intersects with
/// the image plane.
bool does_horizon_intersect( const unsigned& ni,
                             const unsigned& nj,
                             const homography::transform_t& homog );



} // namespace vidtk

#endif // vidtk_utilities_horizon_detection_functions_h_
