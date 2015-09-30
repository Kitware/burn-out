/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "warp_image.txx"

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

INSTANTIATE( vxl_byte );
INSTANTIATE( vxl_uint_16 );
INSTANTIATE( double );
