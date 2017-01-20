/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_warp_and_average_h_
#define vidtk_warp_and_average_h_

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vector>

#include "warp_image.h"

//template < typename PixType >
//class vil_image_view;

namespace vidtk
{

/// \brief Warps images to a reference frame and averages them with a crop region
/// returns false if the crop region is not fully contained in ref_frame
///
/// \param src_frames The input images.
/// \param dest the output averaged image
/// \param homogs the homographies for src_frames, these are assumed to warp to the same plane
/// \param ref_frame the index into src_frames of the image to warp to
/// \param i0 left crop pixel index
/// \param j0 top crop pixel index
/// \param ni width of crop region
/// \param nj height of crop region
/// \param wip parameters to warp image, set the interp method and the unwarped value
/// \param scale_factor the output scale of dest
template < typename PixType >
bool warp_and_average(const std::vector<vil_image_view<PixType> > &src_frames,
                      vil_image_view<PixType> &dest,
                      const std::vector<vgl_h_matrix_2d<double> > &homogs,
                      unsigned int ref_frame, unsigned int i0, unsigned int j0,
                      unsigned int ni, unsigned int nj, const warp_image_parameters &wip,
                      double scale_factor = 1.0);

/// \brief Warps images to a reference frame and averages them
///
/// \param src_frames The input images.
/// \param dest the output averaged image
/// \param homogs the homographies for src_frames, these are assumed to warp to the same plane
/// \param ref_frame the index into src_frames of the image to warp to
/// \param wip parameters to warp image, set the interp method and the unwarped value
/// \param scale_factor the output scale of dest
template < typename PixType >
void warp_and_average(const std::vector<vil_image_view<PixType> > &src_frames,
                      vil_image_view<PixType> &dest,
                      const std::vector<vgl_h_matrix_2d<double> > &homogs,
                      unsigned int ref_frame, const warp_image_parameters &wip,
                      double scale_factor = 1.0);
}

#endif // vidtk_warp_and_average
