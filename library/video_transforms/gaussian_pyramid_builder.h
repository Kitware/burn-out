/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef gaussian_pyramid_h_
#define gaussian_pyramid_h_

#include <vil/vil_image_view.h>
#include <vil/algo/vil_gauss_filter.h>
#include <vector>

/// \file
/// Build a Gaussian pyramid

namespace vidtk
{


class gaussian_pyramid_builder
{
public:

  gaussian_pyramid_builder()
    : gauss_params(1.0), decimation(2), levels(1) {}

  /// Parameters for computing a Gaussian pyramid of \a levels levels.
  /// Each level is 1 / \a sampling the width and height of the previous level
  /// with a Gaussian smoothing of \a sigma
  /// Index 0 is the original image.
  gaussian_pyramid_builder(unsigned pyr_levels, unsigned sampling=2, double sigma=1.0)
    : gauss_params(sigma), decimation(sampling), levels(pyr_levels) {}

  unsigned get_sampling() const { return decimation; }
  double get_sigma() const { return gauss_params.sigma(); }
  unsigned get_num_levels() const { return levels; }

  /// Compute the Gaussian pyramid of \a image
  /// Index 0 is the original image.
  template<class PixType>
  void
  build_pyramid(const vil_image_view<PixType>& image,
                std::vector<vil_image_view<PixType> >& gauss_pyr) const;

  /// Compute a Gaussian pyramid along with the gradient of the Gaussian pyramid
  /// of \a image.  Index 0 is the original image.
  template<class PixType, class GradType>
  void
  build_pyramid(const vil_image_view<PixType>& image,
                std::vector<vil_image_view<PixType> >& gauss_pyr,
                std::vector<vil_image_view<GradType> >& grad_pyr) const;

private:

  vil_gauss_filter_5tap_params gauss_params;
  unsigned decimation;
  unsigned levels;
};

/// This function flattens a pyramid of images to a single image
/// If it is given a 2 plane image it will expand it to 3 and display in RGB
/// \param pyramid is the Gaussian/gradient pyramid
/// \param tiled is the flattened image
template<class PixType>
void
tile_pyramid(const std::vector<vil_image_view<PixType> > &pyramid,
             vil_image_view<PixType> &tiled);

}

#endif // gaussian_pyramid_h_
