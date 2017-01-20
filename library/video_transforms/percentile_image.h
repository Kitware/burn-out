/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_percentile_image_h_
#define vidtk_percentile_image_h_

#include <vil/vil_image_view.h>

#include <vector>

namespace vidtk
{


/// Samples a grayscale image and returns a sorted array of sampled points
/// which can be used to estimate different percentiles in the image.
template <typename PixType>
void sample_and_sort_image( const vil_image_view< PixType >& src,
                            std::vector< PixType >& dst,
                            unsigned sampling_points = 1000 );


/// Returns the specified image by sampling a certain number of points in it.
template <typename PixType>
void get_image_percentiles( const vil_image_view< PixType >& src,
                            const std::vector< double >& percentiles,
                            std::vector< PixType >& dst,
                            unsigned sampling_points = 1000 );


/// Calculate the specified percentiles from the input image and generate
/// binary output masks for each percentile.
template<typename PixType>
void percentile_threshold_above( const vil_image_view< PixType >& src,
                                 const std::vector< double >& percentiles,
                                 vil_image_view< bool >& dst,
                                 unsigned sampling_points = 1000 );


/// Calculate the specified percentiles from the input image and generate
/// binary output masks for each percentile.
template<typename PixType>
void percentile_threshold_above( const vil_image_view< PixType >& src,
                                 const double percentile,
                                 vil_image_view< bool >& dst,
                                 unsigned sampling_points = 1000 );

}

#endif
