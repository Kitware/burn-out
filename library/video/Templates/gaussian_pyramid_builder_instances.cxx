/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video/gaussian_pyramid_builder.txx>

template void
vidtk::gaussian_pyramid_builder::
build_pyramid<double>(const vil_image_view<double>&,
                      vcl_vector<vil_image_view<double> >&) const;
template void
vidtk::gaussian_pyramid_builder::
build_pyramid<vxl_byte>(const vil_image_view<vxl_byte>&,
                        vcl_vector<vil_image_view<vxl_byte> >&) const;
template void
vidtk::gaussian_pyramid_builder::
build_pyramid<double, double>(const vil_image_view<double>&,
                              vcl_vector<vil_image_view<double> >&,
                              vcl_vector<vil_image_view<double> >&) const;
template void
vidtk::gaussian_pyramid_builder::
build_pyramid<vxl_byte, double>(const vil_image_view<vxl_byte>&,
                                vcl_vector<vil_image_view<vxl_byte> >&,
                                vcl_vector<vil_image_view<double> >&) const;
template void
vidtk::tile_pyramid<double>(const vcl_vector<vil_image_view<double> > &,
                            vil_image_view<double> &);
template void
vidtk::tile_pyramid<vxl_byte>(const vcl_vector<vil_image_view<vxl_byte> > &,
                              vil_image_view<vxl_byte> &);
