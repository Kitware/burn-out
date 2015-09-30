/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_SHOT_STITCHING_ALGO_H
#define INCL_SHOT_STITCHING_ALGO_H

// The implementation of the across-a-glitch shot stitching algorithm.
//
// Given two images, attempt to establish the image-to-image homography
// between them; return the homography on success or an invalid homography
// on failure.

#include <vil/vil_image_view.h>

#include <utilities/config_block.h>
#include <utilities/homography.h>

namespace vidtk
{

template< typename PixType > class shot_stitching_algo_impl;

template< typename PixType >
class shot_stitching_algo
{
public:

  shot_stitching_algo();
  ~shot_stitching_algo();

  config_block params() const;
  bool set_params( const config_block& cfg );

  image_to_image_homography stitch( const vil_image_view<PixType>& src,
                                    const vil_image_view<PixType>& dest );

private:
  shot_stitching_algo_impl<PixType>* impl_;
};


} // namespace vidtk


#endif
