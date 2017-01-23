/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "high_pass_filter.h"

#include <utilities/point_view_to_region.h>

#include <vil/vil_transpose.h>
#include <vil/vil_math.h>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "high_pass_filter_txx" );


// Fast 1D (horizontal) box filter smoothing
template <typename PixType>
void box_average_1d( const vil_image_view<PixType>& src,
                     vil_image_view<PixType>& dst,
                     unsigned width )
{
  LOG_ASSERT(width % 2 == 1, "Kernel width must be odd");
  LOG_ASSERT(src.ni() > 0, "Image width must be non-zero");

  if( width >= src.ni() )
  {
    width = ( src.ni() == 1 ? 1 : (src.ni()-2) | 0x01 );
  }

  unsigned ni = src.ni(), nj = src.nj(), np = src.nplanes();
  dst.set_size(ni,nj,np);
  unsigned half_width = width / 2;

  std::ptrdiff_t istepS=src.istep(),  jstepS=src.jstep(),  pstepS=src.planestep();
  std::ptrdiff_t istepD=dst.istep(),  jstepD=dst.jstep(),  pstepD=dst.planestep();

  const PixType*   planeS = src.top_left_ptr();
  PixType*         planeD = dst.top_left_ptr();

  for (unsigned p=0; p<np; ++p, planeS+=pstepS, planeD+=pstepD)
  {
    const PixType* rowS = planeS;
    PixType*       rowD = planeD;
    for (unsigned j=0; j<nj; ++j, rowS += jstepS, rowD += jstepD)
    {
      const PixType* pixelS1 = rowS;
      const PixType* pixelS2 = rowS;
      PixType*       pixelD = rowD;

      // fast box filter smoothing by adding one pixel to the sum and
      // subtracting another pixel at each step
      unsigned sum = 0;
      unsigned i=0;

      // initialize the sum for half the kernel width
      for (; i<=half_width; ++i, pixelS2+=istepS)
      {
        sum += *pixelS2;
      }

      // starting boundary case: the kernel width is expanding
      for (; i<width; ++i, pixelS2+=istepS, pixelD+=istepD)
      {
        *pixelD = static_cast<PixType>(sum / i);
        sum += *pixelS2;
      }

      // general case: add the leading edge and remove the trailing edge.
      for (; i<ni; ++i, pixelS1+=istepS, pixelS2+=istepS, pixelD+=istepD)
      {
        *pixelD = static_cast<PixType>(sum / width);
        sum -= *pixelS1;
        sum += *pixelS2;
      }

      // ending boundary case: the kernel is shrinking
      for (i=width; i>half_width; --i, pixelS1+=istepS, pixelD+=istepD)
      {
        *pixelD = static_cast<PixType>(sum / i);
        sum -= *pixelS1;
      }
    }
  }
}

// Given an input grayscale image, and a smoothed version of this greyscale
// image, calculate the bidirectional filter response in either the vertical
// or horizontal directions.
template <typename PixType>
void box_bidirectional_pass( const vil_image_view<PixType>& grey,
                             const vil_image_view<PixType>& avg,
                             vil_image_view<PixType>& output,
                             unsigned kernel_width,
                             bool is_horizontal )
{
  const unsigned ni = grey.ni();
  const unsigned nj = grey.nj();

  output.fill( 0 );

  unsigned offset = ( kernel_width / 2 ) + 1;

  unsigned diff1 = 0, diff2 = 0;

  if( is_horizontal )
  {
    if( ni < 2 * offset + 1 )
    {
      return;
    }

    for( unsigned j = 0; j < nj; j++ )
    {
      for( unsigned i = offset; i < ni - offset; i++ )
      {
        const PixType& val = grey(i,j);
        const PixType& avg1 = avg(i-offset,j);
        const PixType& avg2 = avg(i+offset,j);

        diff1 = ( val > avg1 ? val - avg1 : avg1 - val );
        diff2 = ( val > avg2 ? val - avg2 : avg2 - val );

        output(i,j) = std::min( diff1, diff2 );
      }
    }
  }
  else
  {
    if( nj < 2 * offset + 1 )
    {
      return;
    }

    for( unsigned j = offset; j < nj - offset; j++ )
    {
      for( unsigned i = 0; i < ni; i++ )
      {
        const PixType& val = grey(i,j);
        const PixType& avg1 = avg(i,j+offset);
        const PixType& avg2 = avg(i,j-offset);

        diff1 = ( val > avg1 ? val - avg1 : avg1 - val );
        diff2 = ( val > avg2 ? val - avg2 : avg2 - val );

        output(i,j) = std::min( diff1, diff2 );
      }
    }
  }
}


// Perform box filtering
template <typename PixType>
void box_high_pass_filter( const vil_image_view<PixType>& grey_img,
                           vil_image_view<PixType>& output,
                           const unsigned kernel_width,
                           const unsigned kernel_height,
                           const bool treat_as_interlaced )
{
  // recreate the output image and create views of each plane
  output.set_size( grey_img.ni(), grey_img.nj(), 3 );
  vil_image_view<PixType> filter_x = vil_plane(output, 0);
  vil_image_view<PixType> filter_y = vil_plane(output, 1);
  vil_image_view<PixType> filter_xy = vil_plane(output, 2);

  // comptue the vertically smoothed image
  filter_x.set_size(grey_img.ni(), grey_img.nj(), 1);
  if( treat_as_interlaced )
  {
    // if interlaced, split the image into odd and even views
    // transpose all input and ouput images so that the horizontal
    // smoothing function produces vertical smoothing
    vil_image_view<PixType> im_even_t = vil_transpose(even_rows(grey_img));
    vil_image_view<PixType> im_odd_t = vil_transpose(odd_rows(grey_img));

    vil_image_view<PixType> smooth_even_t = vil_transpose(even_rows(filter_x));
    vil_image_view<PixType> smooth_odd_t = vil_transpose(odd_rows(filter_x));

    // use a half size odd kernel since images are half height
    int half_kernel_height = kernel_height / 2;
    if (half_kernel_height % 2 == 0)
    {
      ++half_kernel_height;
    }
    box_average_1d(im_even_t, smooth_even_t, half_kernel_height);
    box_average_1d(im_odd_t, smooth_odd_t, half_kernel_height);
  }
  else
  {
    // if not interlaced, transpose inputs and outputs and apply horizontal smoothing.
    vil_image_view<PixType> grey_img_t = vil_transpose(grey_img);
    vil_image_view<PixType> smooth_t = vil_transpose(filter_x);
    box_average_1d(grey_img_t, smooth_t, kernel_height);
  }

  // apply horizontal smoothing to the vertical smoothed image to get a 2D box filter
  box_average_1d(filter_x, filter_xy, kernel_width);

  // smooth the image horizontally and detect the vertical response
  box_average_1d(grey_img, filter_y, kernel_width);

  // Report the difference between the pixel value, and all of the smoothed responses
  vil_math_image_abs_difference( grey_img, filter_x, filter_x );
  vil_math_image_abs_difference( grey_img, filter_y, filter_y );
  vil_math_image_abs_difference( grey_img, filter_xy, filter_xy );
}


// Perform bidirectional filtering
template <typename PixType>
void bidirection_box_filter( const vil_image_view<PixType>& grey_img,
                             vil_image_view<PixType>& output,
                             const unsigned kernel_width,
                             const unsigned kernel_height,
                             const bool treat_as_interlaced )
{
  // recreate the output image and create views of each plane
  output.set_size( grey_img.ni(), grey_img.nj(), 3 );
  vil_image_view<PixType> filter_x = vil_plane(output, 1);
  vil_image_view<PixType> filter_y = vil_plane(output, 2);
  vil_image_view<PixType> filter_xy = vil_plane(output, 0);

  // comptue the vertically smoothed image
  filter_x.set_size(grey_img.ni(), grey_img.nj(), 1);
  if( treat_as_interlaced )
  {
    // if interlaced, split the image into odd and even views
    // transpose all input and ouput images so that the horizontal
    // smoothing function produces vertical smoothing
    vil_image_view<PixType> im_even_t = vil_transpose(even_rows(grey_img));
    vil_image_view<PixType> im_odd_t = vil_transpose(odd_rows(grey_img));

    vil_image_view<PixType> smooth_even_t = vil_transpose(even_rows(filter_x));
    vil_image_view<PixType> smooth_odd_t = vil_transpose(odd_rows(filter_x));

    // use a half size odd kernel since images are half height
    int half_kernel_height = kernel_height / 2;
    if (half_kernel_height % 2 == 0)
    {
      ++half_kernel_height;
    }
    box_average_1d(im_even_t, smooth_even_t, half_kernel_height);
    box_average_1d(im_odd_t, smooth_odd_t, half_kernel_height);
  }
  else
  {
    // if not interlaced, transpose inputs and outputs and apply horizontal smoothing.
    vil_image_view<PixType> grey_img_t = vil_transpose(grey_img);
    vil_image_view<PixType> smooth_t = vil_transpose(filter_x);
    box_average_1d(grey_img_t, smooth_t, kernel_height);
  }

  // smooth the image horizontally and detect the vertical response
  box_average_1d(grey_img, filter_y, kernel_width);

  // Report the difference between the pixel value, and all of the smoothed responses
  box_bidirectional_pass( grey_img, filter_x, filter_xy, kernel_width, true );
  box_bidirectional_pass( grey_img, filter_y, filter_x, kernel_height, false );
  vil_math_image_max( filter_xy, filter_x, filter_y );
}


} // end namespace vidtk
