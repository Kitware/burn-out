/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pixel_feature_array_
#define vidtk_pixel_feature_array_

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <vector>

#include <tracking_data/image_border.h>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

/// \brief A class for storing an array of pixel-level features, which are
/// computed for each individual pixel in an image region, and are all of
/// the same given type.
template <typename PixType>
class pixel_feature_array
{
public:

  typedef pixel_feature_array< PixType > self_t;
  typedef vil_image_view< PixType > image_t;
  typedef std::vector< image_t > array_t;
  typedef boost::shared_ptr< self_t > sptr_t;
  typedef vgl_box_2d< int > region_t;

  pixel_feature_array() {}
  ~pixel_feature_array() {}

  /// Array of features for each pixel in an image.
  array_t features;

  /// An optional border or subregion compared to some original image,
  /// in which all of the given features were computed in.
  region_t region;
};

// Commonly used definitions
typedef pixel_feature_array< vxl_byte > byte_pixel_feature_array;
typedef boost::shared_ptr< byte_pixel_feature_array > byte_pixel_feature_array_sptr;

typedef pixel_feature_array< float > fp_pixel_feature_array;
typedef boost::shared_ptr< fp_pixel_feature_array > fp_pixel_feature_array_sptr;

} // end namespace vidtk

#endif
