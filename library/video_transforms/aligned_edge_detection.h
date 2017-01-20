/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_aligned_edge_detection_h_
#define vidtk_aligned_edge_detection_h_

#include <vil/vil_image_view.h>

namespace vidtk
{

/// @file aligned_edge_detection.h
/// @brief Helper functions used for generating information pretaining to possible
/// vertical and horizontally aligned edges within an image.



/// Given the partial i and j derivatives for each pixel, compute canny edges
/// by performing non-maximum suppression in vertical and horizontal directions
/// only. The output is a 2-channel image containing aligned edges where the first
/// channel of the image contains potential NMS horizontal edges, and the second
/// contains the vertical equivalent.
template< typename InputType, typename OutputType >
void calculate_aligned_edges( const vil_image_view< InputType >& grad_i,
                              const vil_image_view< InputType >& grad_j,
                              const InputType edge_threshold,
                              vil_image_view< OutputType >& output );


/// Alternative overload for the calculate_aligned_edges function, only accepting
/// an input image before gradient detection, and two optional parameters specifying
/// the gradient buffers to use and output.
template< typename PixType, typename GradientType >
void calculate_aligned_edges( const vil_image_view< PixType >& input,
                              const GradientType edge_threshold,
                              vil_image_view< PixType >& output,
                              vil_image_view< GradientType >& grad_i = vil_image_view<GradientType>(),
                              vil_image_view< GradientType >& grad_j = vil_image_view<GradientType>() );



} // end namespace vidtk


#endif // vidtk_aligned_edge_detection_h_
