/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief Declaring template instantiations of generic_shot_stitching_algo
 *        class.
 */

#include <tracking/generic_shot_stitching_algo.txx>

template class vidtk::generic_shot_stitching_algo<vxl_byte>;
template class vidtk::generic_shot_stitching_algo<vxl_uint_16>;
