/*ckwg +5
 * Copyright 2012-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_high_pass_filter_h_
#define vidtk_high_pass_filter_h_


#include <vil/vil_image_view.h>


namespace vidtk
{


/// @file high_pass_filter.h
/// @brief A file for containing high-pass filtering and related functions for
///        images, based on techniques either in the spatial or frequency domain.




/// @brief Fast 1D (horizontal) box filter smoothing
/// @param width The width of the box filter (an odd value)
template <typename PixType>
void box_average_1d( const vil_image_view<PixType>& src,
                     vil_image_view<PixType>& dst,
                     unsigned width );



/// @brief Returns an image containing the per-pixel difference between
///        each pixel in the input image, and some local regional average.
///
/// The input is expected to be a single channel grayscale image. The output
/// contains 3 channels: the first being the difference between each pixel
/// and an average in the horizontal direction only, the second in the vertical
/// direction, and the third a 2D kernel (of size kernel_height by kernel_width).
/// The averaging kernel is centered on each pixel.
///
/// @param grey_img The input single channel image
/// @param output The output image, containing 3 channels
/// @param kernel_width Width of the averaging kernel
/// @param kernel_height Height of the averaging kernel
/// @param treat_as_interlaced Should we assume the input image is interlaced
///        at a pixel level?
template <typename PixType>
void box_high_pass_filter( const vil_image_view<PixType>& grey_img,
                           vil_image_view<PixType>& output,
                           const unsigned kernel_width = 7,
                           const unsigned kernel_height = 7,
                           const bool treat_as_interlaced = false );



/// @brief Returns an image containing the minimum per-pixel difference between
///        each pixel in the input image, and some local regional average computed
///        in different directions around the pixel.
///
/// The input is expected to be a single channel grayscale image. The output
/// contains 3 channels: the first being the minimum difference between each pixel
/// and an average computed on both sides of it in the horizontal direction only,
/// the second in the vertical direction only, and the third the maximum of the
/// first two. Differs from a box_high_pass filter in that instead of having a kernel
/// centered on each pixel, we take the minimum pixel distance between the averages
/// moving away from each pixel (on both sides of it).
///
/// @param grey_img The input single channel image
/// @param output The output image, containing 3 channels
/// @param kernel_width Width of the averaging kernel
/// @param kernel_height Height of the averaging kernel
/// @param treat_as_interlaced Should we assume the input image is interlaced
///        at a pixel level?
template <typename PixType>
void bidirection_box_filter( const vil_image_view<PixType>& grey_img,
                             vil_image_view<PixType>& output,
                             const unsigned kernel_width = 7,
                             const unsigned kernel_height = 7,
                             const bool treat_as_interlaced = false );


} // end namespace vidtk


#endif // vidtk_high_pass_filter_h_
