/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief Template specializations of populate_algo_map for expected types
 */

#include <vxl_config.h>

#include <tracking/generic_shot_stitching_algo.txx>

#include <tracking/klt_shot_stitching_algo.h>

#ifdef WITH_MAPTK_ENABLED
#include <tracking/maptk_shot_stitching_algo.h>
#endif


namespace vidtk
{


template<>
void
generic_shot_stitching_algo_impl<vxl_byte>
::populate_algo_map()
{
  this->algo_map_["klt"] = new klt_shot_stitching_algo<vxl_byte>();

#ifdef WITH_MAPTK_ENABLED
  this->algo_map_["maptk"] = new maptk_shot_stitching_algo<vxl_byte>();
#endif
}

template<>
void
generic_shot_stitching_algo_impl<vxl_uint_16>
::populate_algo_map()
{
  this->algo_map_["klt"] = new klt_shot_stitching_algo<vxl_uint_16>();

#ifdef WITH_MAPTK_ENABLED
  this->algo_map_["maptk"] = new maptk_shot_stitching_algo<vxl_uint_16>();
#endif
}


} // end vidtk namespace
