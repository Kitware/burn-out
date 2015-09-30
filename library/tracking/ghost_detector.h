/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ghost_detector_h_
#define vidtk_ghost_detector_h_

#include <tracking/image_object.h>
#include <vil/vil_image_view.h>

/// \file Algorithm to detect ghost/shadow moving object detection (MOD).
///
/// Keeping this class in tracking as opposed to video due to dependency on
/// tracking/image_object.h.

namespace vidtk
{

template < class PixType >
class ghost_detector
{
public:
  ghost_detector( float min_grad_mag_var = 15.0,
                  unsigned pixel_padding = 2);

  /// \brief Anlyze distribution of gradient magnitudes to identify a ghost.
  ///
  /// Currently using the simple feature of variance of of gradient magnitude.
  /// The variance proved to be more discriminative of the ghost MODs as
  /// compared to two other features tried, i.e.
  /// a) Similarity of histogram-of-gradient-magnitudes of all pixels
  ///    encapsulated by the box vs. the mask inside the box. For ghost
  ///    MODs this ratio would be close to 1.
  /// b) Derivative of gradient magnitude between 95th and 80th percentile
  ///    (or similar range) over the whole box covering the MOD.
  bool has_ghost_gradients( image_object_sptr , vil_image_view<PixType> const& ) const;

private:
  /// \brief Threshold below which MODs are labelled as ghosts.
  ///
  /// This is the minimum value of image gradient magnitude, for a
  /// MOD to be considered as a valid (non-ghost) mover. Legitimate movers
  /// would typically have high variance values.
  float min_grad_mag_var_;

  /// \brief Padding applied to the boxes to include more background.
  unsigned pixel_padding_;

}; // class ghost_detector

} // namespace vidtk

#endif // vidtk_ghost_detector_h_
