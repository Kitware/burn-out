/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/automatic_white_balancing.txx>

template void vidtk::compute_correction_matrix( std::vector< awb_reference_point< vxl_byte > >& ref,
                                                const unsigned& resolution_per_chan,
                                                std::vector< double >& output,
                                                const double& white_point );

template void vidtk::compute_correction_matrix( std::vector< awb_reference_point< vxl_uint_16 > >& ref,
                                                const unsigned& resolution_per_chan,
                                                std::vector< double >& output,
                                                const double& white_point );

template class vidtk::auto_white_balancer<vxl_byte>;

template class vidtk::auto_white_balancer<vxl_uint_16>;
