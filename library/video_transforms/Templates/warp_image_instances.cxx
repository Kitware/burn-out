/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/warp_image.txx>
#include <vil/vil_bicub_interp.txx>

#define INSTANTIATE( PixType )                                              \
template bool vidtk::warp_image( vil_image_view<PixType> const& src,        \
                                 vil_image_view<PixType>& dest,             \
                                 vgl_h_matrix_2d<double> const& dest_to_src_homography, \
                                 vil_image_view< bool > * const unmapped_mask );  \
                                                                            \
template bool vidtk::warp_image( vil_image_view<PixType> const& src,        \
                                 vil_image_view<PixType>& dest,             \
                                 vgl_h_matrix_2d<double> const& dest_to_src_homography, \
                                 int, int,                                  \
                                 vil_image_view< bool > * const unmapped_mask );  \
                                                                            \
template bool vidtk::warp_image( vil_image_view<PixType> const& src,        \
                                 vil_image_view<PixType>& dest,             \
                                 vgl_h_matrix_2d<double> const& dest_to_src_homography, \
                                 warp_image_parameters const& param,        \
                                 vil_image_view< bool > * const unmapped_mask );  \

INSTANTIATE( bool );
INSTANTIATE( vxl_byte );
INSTANTIATE( vxl_uint_16 );
INSTANTIATE( double );
INSTANTIATE( float )

VIL_BICUB_INTERP_INSTANTIATE( bool );
