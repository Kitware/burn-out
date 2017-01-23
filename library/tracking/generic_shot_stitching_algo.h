/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief Shot stitching algorithm that allows the choice of the algorithm
 *        implementation to use at configuration time.
 */

#ifndef vidtk_tracking_generic_shot_stitching_algo_h
#define vidtk_tracking_generic_shot_stitching_algo_h

#include "shot_stitching_algo.h"


namespace vidtk
{

template< typename PixType >
class generic_shot_stitching_algo_impl;

template< typename PixType >
class generic_shot_stitching_algo
  : public shot_stitching_algo<PixType>
{
public:

  /// Constructor
  generic_shot_stitching_algo();

  /// Destructor
  virtual ~generic_shot_stitching_algo();

  /// Get algorithm configuration parameters
  config_block params() const;

  /// Set algorithm configuration parameters
  /**
   * Sets algorithm type iff parameters successfully set.
   */
  bool set_params( config_block const &blk );

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
  image_to_image_homography stitch( vil_image_view<PixType> const &src,
                                    vil_image_view<bool> const &src_mask,
                                    vil_image_view<PixType> const &dest,
                                    vil_image_view<bool> const &dest_mask );

private:
  generic_shot_stitching_algo_impl<PixType> *impl_;

};


} // end vidtk namespace

#endif // vidtk_tracking_generic_shot_stitching_algo_h
