/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_icos_hog_descriptor_h_
#define vidtk_icos_hog_descriptor_h_

#include <descriptors/online_descriptor_generator.h>

#include <vector>
#include <string>

namespace vidtk
{


// Single frame dsift settings
struct icosahedron_hog_parameters
{
  /// Have n cells per the width of the BB around each track
  unsigned int width_cells;

  /// Have n cells per the height of the BB around each track
  unsigned int height_cells;

  /// Divide the time period over which this descriptor is formulated by n
  unsigned int time_cells;

  /// Enable variable bin smoothing
  bool variable_smoothing;

  // Default settings
  icosahedron_hog_parameters()
   : width_cells(3),
     height_cells(3),
     time_cells(3),
     variable_smoothing(true)
  {}
};

// Calculate a vector of raw dsift features, images must be single channel
// and they must all be the same size
bool calculate_icosahedron_hog_features(
  const std::vector< vil_image_view<float> >& images,
  std::vector<double>& features,
  const icosahedron_hog_parameters& options = icosahedron_hog_parameters() );


} // end namespace vidtk

#endif
