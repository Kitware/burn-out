/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/warp_and_average.txx>

#define INSTANTIATE( PIXTYPE ) \
\
template bool vidtk::warp_and_average(const std::vector<vil_image_view<PIXTYPE> > &src_frames, \
                                      vil_image_view<PIXTYPE> &dest, \
                                      const std::vector<vgl_h_matrix_2d<double> > &homogs, \
                                      unsigned int ref_frame, unsigned int i0, unsigned int j0, \
                                      unsigned int ni, unsigned int nj, \
                                      const warp_image_parameters &wip, double scale_factor); \
\
template void vidtk::warp_and_average(const std::vector<vil_image_view<PIXTYPE> > &src_frames, \
                                      vil_image_view<PIXTYPE> &dest, \
                                      const std::vector<vgl_h_matrix_2d<double> > &homogs, \
                                      unsigned int ref_frame, const warp_image_parameters &wip, \
                                      double scale_factor); \

//Does not work with non float
INSTANTIATE( float );
INSTANTIATE( double );

#undef INSTANTIATE
