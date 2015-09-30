/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/synthetic_path_generator_process.txx>
#include <tracking/linear_path.h>
#include <tracking/circular_path.h>

template class vidtk::synthetic_path_generator_process< vidtk::linear_path >;
template class vidtk::synthetic_path_generator_process< vidtk::circular_path >;
