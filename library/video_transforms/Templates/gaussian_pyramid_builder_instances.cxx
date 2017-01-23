/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/gaussian_pyramid_builder.txx>

#define GAUSSIAN_PYRAMID_INSTANTIATE(PixType)                                \
template void                                                                \
vidtk::gaussian_pyramid_builder::                                            \
build_pyramid<PixType >(const vil_image_view<PixType >&,                     \
                       std::vector<vil_image_view<PixType > >&) const;        \
template void                                                                \
vidtk::tile_pyramid<PixType >(const std::vector<vil_image_view<PixType > > &, \
                             vil_image_view<PixType > &)

#define GAUSSIAN_PYRAMID_INSTANTIATE2(PixType, GradType)                     \
template void                                                                \
vidtk::gaussian_pyramid_builder::                                            \
build_pyramid<PixType, GradType >(const vil_image_view<PixType >&,           \
                                 std::vector<vil_image_view<PixType > >&,     \
                                 std::vector<vil_image_view<GradType > >&) const

GAUSSIAN_PYRAMID_INSTANTIATE(float);
GAUSSIAN_PYRAMID_INSTANTIATE(double);
GAUSSIAN_PYRAMID_INSTANTIATE(vxl_byte);
GAUSSIAN_PYRAMID_INSTANTIATE(vxl_uint_16);

GAUSSIAN_PYRAMID_INSTANTIATE2(float, float);
GAUSSIAN_PYRAMID_INSTANTIATE2(double, double);
GAUSSIAN_PYRAMID_INSTANTIATE2(vxl_byte, double);
GAUSSIAN_PYRAMID_INSTANTIATE2(vxl_uint_16, double);
