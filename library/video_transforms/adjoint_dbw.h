/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef adjoint_dbw_h_
#define adjoint_dbw_h_

#include <vil/vil_image_view.h>

#include "adjoint_image_op.h"


namespace vidtk
{

/// Create DBW adjoint operator from a flow field and various options.
/// \param flow The optical flow field used in the warping step, also defined input resolution.
/// \param ni The width of the output image after applying DBW
/// \param nj The height of the output image after applying DBW
/// \param np The number of planes (e.g. color channels) in input and output
/// \param scale_factor The scale factor for the down sampling step
/// \param smoothing_sigma The amount of bluring for the blur step.
/// \param down_sample_averaging If true, average pixels in a box to down scale
///        rather than just picking one pixel to sample.
/// \param bicubic_warping toggle between bilinear (false) and bicubic (true)
adjoint_image_ops_func<double>
create_dbw_from_flow(const vil_image_view<double> &flow,
                     const unsigned ni, const unsigned nj, const unsigned np,
                     int scale_factor,
                     double smoothing_sigma,
                     bool down_sample_averaging = false,
                     bool bicubic_warping = false);

} // end namespace vidtk

#endif //adjoint_dbw_h_
