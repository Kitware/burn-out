/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_color_commonality_filter_functions_h_
#define vidtk_color_commonality_filter_functions_h_

#include <vil/vil_image_view.h>

#include <vector>

namespace vidtk
{

/// Settings for the below functions
struct color_commonality_filter_settings
{
  // Resolution per channel when building a histogram detailing commonality
  unsigned int resolution_per_channel;

  // Scale the output image (which will by default by in the range [0,1])
  // by this amount. Set as 0 to scale to the input type-specific maximum.
  unsigned int output_scale_factor;

  // Instead of computing the per-pixel color commonality out of all the pixels
  // in the entire image, should we instead compute it in grids windowed across it?
  bool grid_image;
  unsigned grid_resolution_height;
  unsigned grid_resolution_width;

  // [Advanced] A pointer to a temporary buffer for the histogram, which
  // can (a) prevent having to reallocate it over and over again, and
  // (b) allow the user to use it as a by-product of the main operation.
  // If set to NULL, will use an internal histogram buffer.
  std::vector<unsigned>* histogram;

  // Default constructor
  color_commonality_filter_settings()
    : resolution_per_channel(8),
      output_scale_factor(0),
      grid_image(false),
      grid_resolution_height(5),
      grid_resolution_width(6),
      histogram(NULL)
  {}
};


/// Create an output image indicating the relative commonality of each
/// input pixel's color occuring in the entire input image. A lower
/// value in the output image corresponds to that pixels value being
/// less common in the entire input image.
///
/// Functions by first building a histogram of the input image, then,
/// for each pixel, looking up the value in the histogram and scaling
/// this value by a given factor.
template< class InputType, class OutputType >
void color_commonality_filter( const vil_image_view<InputType>& input,
                               vil_image_view<OutputType>& output,
                               const color_commonality_filter_settings& options );

}

#endif // vidtk_color_commonality_filter_functions_h_
