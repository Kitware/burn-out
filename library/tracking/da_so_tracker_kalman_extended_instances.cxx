/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//da_so_tracker_kalman_extended_instances.cxx
#include <tracking/da_so_tracker_kalman_extended.txx>
#include <tracking/extended_kalman_functions.h>

template class vidtk::da_so_tracker_kalman_extended< vidtk::extended_kalman_functions::linear_fun >;
template class vidtk::da_so_tracker_kalman_extended< vidtk::extended_kalman_functions::speed_heading_fun >;
template class vidtk::da_so_tracker_kalman_extended< vidtk::extended_kalman_functions::circular_fun >;

#include <vbl/vbl_smart_ptr.hxx>

// Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::da_so_tracker_kalman_extended< vidtk::extended_kalman_functions::linear_fun >::const_parameters );
VBL_SMART_PTR_INSTANTIATE( vidtk::da_so_tracker_kalman_extended< vidtk::extended_kalman_functions::speed_heading_fun >::const_parameters );
VBL_SMART_PTR_INSTANTIATE( vidtk::da_so_tracker_kalman_extended< vidtk::extended_kalman_functions::circular_fun >::const_parameters );
