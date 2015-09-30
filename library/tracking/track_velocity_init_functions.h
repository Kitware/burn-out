/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#ifndef vidtk_track_velocity_init_functions_h_
#define vidtk_track_velocity_init_functions_h_

#include <tracking/track.h>

namespace vidtk
{

bool initialize_velocity_const_last_detection( track_sptr trk,
                                               unsigned int step_back = 0 );
bool initialize_velocity_robust_const_acceleration( track_sptr trk,
                                                    unsigned int num_detection_back,
                                                    double norm_resid = 1.5
                                                  );
bool initialize_velocity_raw( track_sptr trk, unsigned int num_detection_back );
bool initialize_velocity_robust_point(track_sptr trk,double norm_resid = 2);

}//namespace vidtk

#endif //vidtk_track_velocity_init_functions

