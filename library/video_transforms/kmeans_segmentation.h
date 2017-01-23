/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kmeans_segmentation_h_
#define vidtk_kmeans_segmentation_h_

#include <vil/vil_image_view.h>

#include <vector>

namespace vidtk
{

/// \brief Performs a kmeans-based segmentation on the input image.
///
/// \param src The input image.
/// \param labels The output labeled image.
/// \param clusters Number of clusteres to use.
/// \param sample_points Number of input pixels to sample.
template< typename PixType, typename LabelType >
bool segment_image_kmeans( const vil_image_view< PixType >& src,
                           vil_image_view< LabelType >& labels,
                           const unsigned clusters = 4,
                           const unsigned sample_points = 1000 );


/// \brief Assign a label to each pixel depending on which of the specified
///        center values said pixel is nearest to.
///
/// Computes the distance (L2) between each pixel, and the specified list of
/// center points. In the output image the ID value given is the index
/// of the closest center point to each pixel. Each entry in the list of
/// centers must have the same number of planes as the input image.
///
/// \param src The input image.
/// \param centers The vector of center points.
/// \param dst The labeled output image.
template < typename PixType, typename LabelType >
void label_image_from_centers( const vil_image_view< PixType >& src,
                               const std::vector< std::vector< PixType > >& centers,
                               vil_image_view< LabelType >& dst );


}

#endif // vidtk_kmeans_segmentation_h_
