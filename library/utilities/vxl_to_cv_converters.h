/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_descriptor_vxl_to_cv_wrappers
#define vidtk_descriptor_vxl_to_cv_wrappers

#include <vector>
#include <string>
#include <cassert>

#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_point_2d.h>
#include <vil/vil_image_view.h>

#include <boost/shared_ptr.hpp>

#include <opencv/cxcore.h>

namespace vidtk
{


// Returns false if we are unable to make a shallow copy into the output matrix
template< typename PixType >
bool shallow_cv_conversion( const vil_image_view< PixType >& input, cv::Mat& output )
{
  // Validate the input image and number of planes
  if( !input || input.nplanes() > 4 )
  {
    return false;
  }

  // Convert input type and channel count to cv::Mat depth
  int cvdepth = CV_MAKETYPE( cv::DataDepth< PixType >::value, input.nplanes() );

  // Validate depth id
  if( cvdepth < 0 )
  {
    return false;
  }

  // Make sure input data is interlaced
  if( input.istep() != input.nplanes() )
  {
    return false;
  }

  // Set opencv Mat header
  output = cv::Mat( input.nj(),
                    input.ni(),
                    cvdepth,
                    const_cast<PixType*>(input.top_left_ptr()),
                    input.jstep() * sizeof( PixType ) );

  return true;
}


// Create a new ipl_image and copy the input contents into it
template< typename PixType >
bool deep_cv_conversion( const vil_image_view< PixType >& input, cv::Mat& output, bool swap_rb = true )
{
  // Validate the input image and number of planes
  if( !input || input.nplanes() > 4 )
  {
    return false;
  }

  // Convert input type and channel count to cv::Mat depth
  int cvdepth = CV_MAKETYPE( cv::DataDepth< PixType >::value, input.nplanes() );

  // Validate depth id
  if( cvdepth < 0 )
  {
    return false;
  }

  // Allocate the new matrix
  output.create( input.nj(), input.ni(), cvdepth );

  // Confirm that out assumption that you cannot have sub sizeof( PixType )
  // widthStep increment is still valid and that it hasn't changed in a future
  // OpenCV version. Note: IplImages can have widthSteps with spacings less than
  // sizeof( PixType ), however cv::Mats using default allocation should not.
  // This is a requirement of using the vxl deep_copy.
  assert( output.step % sizeof( PixType ) == 0 );

  // Copy input contents into the new buffer
  PixType* dest_ptr = output.ptr<PixType>();
  std::ptrdiff_t dest_istep = input.nplanes();
  std::ptrdiff_t dest_jstep = output.step1();
  std::ptrdiff_t dest_pstep = 1;

  // Handle special case (swapping red and blue channels)
  if( swap_rb && input.nplanes() == 3 )
  {
    dest_pstep = -1;
    dest_ptr += 2;
  }

  // Perform deep copy
  vil_image_view<PixType> dest( dest_ptr,
                                input.ni(),
                                input.nj(),
                                input.nplanes(),
                                dest_istep,
                                dest_jstep,
                                dest_pstep );

  dest.deep_copy( input );

  return true;
}

// Attempt a shallow copy, if that fails, perform a deep conversion between
// opencv and vxl image types
template< typename PixType >
bool forced_cv_conversion( const vil_image_view< PixType >& input, cv::Mat& output )
{
  if( !shallow_cv_conversion( input, output ) &&
      !deep_cv_conversion( input, output ) )
  {
    return false;
  }
  return true;
}

// Convert a vxl box to a cv rect
template< typename InputType, typename OutputType >
void convert_to_rect( const vgl_box_2d< InputType >& input, cv::Rect_< OutputType >& output )
{
  output.x = static_cast<OutputType>(input.min_x());
  output.y = static_cast<OutputType>(input.min_y());
  output.height = static_cast<OutputType>(input.height());
  output.width = static_cast<OutputType>(input.width());
}

// Convert a vxl point to a cv rect
template< typename InputType, typename OutputType >
void convert_to_point( const vgl_point_2d< InputType >& input, cv::Point_< OutputType >& output )
{
  output.x = static_cast<OutputType>(input.x());
  output.y = static_cast<OutputType>(input.y());
}

}

#endif
