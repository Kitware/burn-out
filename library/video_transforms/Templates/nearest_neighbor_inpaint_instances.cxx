/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/nearest_neighbor_inpaint.txx>

#include <vil/vil_image_view.h>

template void vidtk::nn_inpaint< bool >( vil_image_view<bool>&,
                                         const vil_image_view<bool>&,
                                         vil_image_view<unsigned> );

template void vidtk::nn_inpaint< vxl_byte >( vil_image_view<vxl_byte>&,
                                             const vil_image_view<bool>&,
                                             vil_image_view<unsigned> );

template void vidtk::nn_inpaint< vxl_uint_16 >( vil_image_view<vxl_uint_16>&,
                                                const vil_image_view<bool>&,
                                                vil_image_view<unsigned> );

template void vidtk::nn_inpaint< double >( vil_image_view<double>&,
                                           const vil_image_view<bool>&,
                                           vil_image_view<unsigned> );
