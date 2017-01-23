/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_video_variable_image_dilate_h_
#define vidtk_video_variable_image_dilate_h_


#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_greyscale_dilate.h>

#include <algorithm>
#include <vector>


namespace vidtk
{


typedef std::vector< vil_structuring_element > structuring_element_vector;
typedef std::vector< std::vector< std::ptrdiff_t > > offset_vector;


///
/// Perform a variable dilation of the image, where we specify some source image,
/// a list of structuring elements we want to use, and a per-pixel image of ids
/// of the structuring element we should use at each pixel (corresponding to
/// ids in the input structuring element vector). The se_element list should
/// be ordered based upon size. So far this function only supports basic
/// primitives (circle or regular symmetrical polygon elements) as inputs
/// due to optimizations.
///
/// Inputs:
///   src - single channel input image
///   se_vector - the vector of structuring elements we want to use
///   elem_ids - an image containing the per-pixel IDs of the se in the
///       element vector that we want to use at every pixel
///
/// Outputs:
///   dst - the destination image
///
template< typename PixType, typename IDType >
void variable_image_dilate( const vil_image_view<PixType>& src,
                            const vil_image_view<IDType>& elem_ids,
                            const structuring_element_vector& se_vector,
                            vil_image_view<PixType>& dst );



///
/// Core [unsafe] implementation for performing a variable dilation, in either
/// a grayscale or binary image. So far this function only supports basic
/// primitives (circle or regular polygon elements) as structuring elements
/// due to optimizations. All structuring elements must be symmetrical.
///
/// Inputs:
///   src - single channel input image
///   se_vector - the vector of structuring elements we want to use
///   offset_vector - the corresponding vector of pointer offsets for each
///       se for the input image, this and se_vector should be the same size.
///   elem_ids - an image containing the per-pixel IDs of the se in the
///       element vector that we want to use at every pixel
///   max_id - the se_vector id of the largest structuring element, used
///       to gracefully handle border conditions.
///
/// Outputs:
///   dst - the destination image
///
template< typename PixType, typename IDType >
void variable_image_dilate( const vil_image_view<PixType>& src,
                            const vil_image_view<IDType>& elem_ids,
                            const structuring_element_vector& se_vector,
                            const offset_vector& offset_vector,
                            const IDType& max_id,
                            vil_image_view<PixType>& dst );


/// Specialized (overload) of the above for boolean type
template< typename IDType >
void variable_image_dilate( const vil_image_view<bool>& src,
                            const vil_image_view<IDType>& elem_ids,
                            const structuring_element_vector& se_vector,
                            const offset_vector& offset_vector,
                            const IDType& max_id,
                            vil_image_view<bool>& dst );


} // end namespace vidtk

#endif
