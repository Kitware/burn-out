/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_linking.txx"
#include "kinematic_track_linking_functor.h"
#include "visual_track_linking_functor.h"
#include "multi_track_linking_functors_functor.h"

template class vidtk::track_linking<vidtk::kinematic_track_linking_functor>;
template class vidtk::track_linking<vidtk::visual_track_linking_functor>;
template class vidtk::track_linking<vidtk::multi_track_linking_functors_functor>;
