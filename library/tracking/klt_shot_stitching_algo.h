/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_TRACKING_KLT_SHOT_STITCHING_ALGO_H
#define VIDTK_TRACKING_KLT_SHOT_STITCHING_ALGO_H

// The implementation of the across-a-glitch shot stitching algorithm.
//
// Given two images, attempt to establish the image-to-image homography
// between them; return the homography on success or an invalid homography
// on failure.

#include <vil/vil_image_view.h>

#include <utilities/config_block.h>
#include <utilities/homography.h>

#include <tracking/shot_stitching_algo.h>

namespace vidtk
{

template< typename PixType >
class klt_shot_stitching_algo_impl;

template< typename PixType >
class klt_shot_stitching_algo
  : public shot_stitching_algo<PixType>
{
public:

  klt_shot_stitching_algo();
  virtual ~klt_shot_stitching_algo();

  config_block params() const;
  bool set_params( config_block const &cfg );

  /**
   * \brief Create an image-to-image homography defining the transformation
   *        from \p src to \p dest
   *
   * Each image may be given an associated boolean mask, where true values
   * denote regions to not consider for stitching.
   *
   * \param src Source image
   * \param src_mask Boolean mask for \p src
   * \param dest Destination image
   * \param dest_mask Boolean mask for \p dest
   * \returns Image-to-image homography describing the transformation from the
   *          \p src image to the \p dest image.
   */
  image_to_image_homography stitch( const vil_image_view < PixType >& src,
                                    const vil_image_view < bool >& src_mask,
                                    const vil_image_view < PixType >& dest,
                                    const vil_image_view < bool >& dest_mask);

private:
  klt_shot_stitching_algo_impl<PixType>* impl_;
};


} // namespace vidtk


#endif
