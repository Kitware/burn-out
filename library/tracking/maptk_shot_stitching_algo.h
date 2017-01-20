/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief MAP-Tk shot stitching algorithm definition
 */

#ifndef VIDTK_TRACKING_MAPTK_SHOT_STITCHING_ALGO_H
#define VIDTK_TRACKING_MAPTK_SHOT_STITCHING_ALGO_H

#include <vil/vil_image_view.h>

#include <utilities/config_block.h>
#include <utilities/homography.h>

#include <tracking/shot_stitching_algo.h>

namespace vidtk
{

class maptk_shot_stitching_algo_impl;

template< typename PixType >
class maptk_shot_stitching_algo
  : public shot_stitching_algo<PixType>
{
public:

  maptk_shot_stitching_algo();
  virtual ~maptk_shot_stitching_algo();

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
  image_to_image_homography stitch( vil_image_view < PixType > const &src,
                                    vil_image_view < bool >    const &src_mask,
                                    vil_image_view < PixType > const &dest,
                                    vil_image_view < bool >    const &dest_mask );

private:
  maptk_shot_stitching_algo_impl *impl_;

};


} // namespace vidtk


#endif // VIDTK_TRACKING_MAPTK_SHOT_STITCHING_ALGO_H
