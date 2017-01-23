/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_TRACKING_SHOT_STITCHING_ALGO_H
#define VIDTK_TRACKING_SHOT_STITCHING_ALGO_H

/**
 * \file
 * \brief Shot stitching algorithm abstract interface
 */

#include <vil/vil_image_view.h>

#include <utilities/config_block.h>
#include <utilities/homography.h>


namespace vidtk
{

template< typename PixType >
class shot_stitching_algo
{
public:
  shot_stitching_algo() {}
  virtual ~shot_stitching_algo() {}

  /// Get algorithm configuration parameters
  virtual config_block params() const = 0;
  /// Set algorithm configuration parameters
  virtual bool set_params( config_block const &cfg ) = 0;

  /// Given two images with optional masks
  virtual image_to_image_homography stitch( const vil_image_view < PixType >& src,
                                            const vil_image_view < bool >& src_mask,
                                            const vil_image_view < PixType >& dest,
                                            const vil_image_view < bool >& dest_mask) = 0;

};


} // namespace vidtk


#endif
