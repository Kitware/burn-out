/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "metadata_burnin_filter_process.h"

#include <vxl_config.h>
#include <vil/vil_transpose.h>
#include <vil/vil_math.h>
#include <vil/vil_convert.h>

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>


namespace vidtk
{

/// \brief Fast 1D (horizontal) box filter smoothing
/// \param width The width of the box filter
void
box_filter_1d(const vil_image_view<vxl_byte>& src,
              vil_image_view<vxl_byte>& dst,
              unsigned width)
{
  unsigned ni = src.ni(), nj = src.nj(), np = src.nplanes();
  dst.set_size(ni,nj,np);
  assert(width < ni);
  assert(width % 2 == 1); // width should be odd
  unsigned half_width = width / 2;

  vcl_ptrdiff_t istepS=src.istep(),  jstepS=src.jstep(),  pstepS=src.planestep();
  vcl_ptrdiff_t istepD=dst.istep(),  jstepD=dst.jstep(),  pstepD=dst.planestep();

  const vxl_byte*   planeS = src.top_left_ptr();
  vxl_byte*         planeD = dst.top_left_ptr();

  for (unsigned p=0; p<np; ++p, planeS+=pstepS, planeD+=pstepD)
  {
    const vxl_byte* rowS = planeS;
    vxl_byte*       rowD = planeD;
    for (unsigned j=0; j<nj; ++j, rowS += jstepS, rowD += jstepD)
    {
      const vxl_byte* pixelS1 = rowS;
      const vxl_byte* pixelS2 = rowS;
      vxl_byte*       pixelD = rowD;

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
        *pixelD = static_cast<vxl_byte>(sum / i);
        sum += *pixelS2;
      }
      // general case: add the leading edge and remove the trailing edge.
      for (; i<ni; ++i, pixelS1+=istepS, pixelS2+=istepS, pixelD+=istepD)
      {
        *pixelD = static_cast<vxl_byte>(sum / width);
        sum -= *pixelS1;
        sum += *pixelS2;
      }
      // ending boundary case: the kernel is shrinking
      for (i=width; i>half_width; --i, pixelS1+=istepS, pixelD+=istepD)
      {
        *pixelD = static_cast<vxl_byte>(sum / i);
        sum -= *pixelS1;
      }
    }
  }

}



/** Constructor.
 *
 *
 */
metadata_burnin_filter_process
::metadata_burnin_filter_process( vcl_string const& name )
  : process( name, "metadata_burnin_filter_process" )
{
  config_.add_parameter(
    "kernel_width",
    "7",
    "Width of the box filter." );

  config_.add_parameter(
    "kernel_height",
    "7",
    "Height of the box filter." );

  config_.add_parameter(
    "interlaced",
    "true",
    "If true, handle frames as interlaced" );

  config_.add_parameter(
    "adaptive_thresh_max",
    "25",
    "The adaptive threshold above which detections have maximum weight." );

  config_.add_parameter(
    "absolute_thresh_min",
    "128",
    "The absolute threshold below which detections have zero weight.\n"
    "This value is relative to detecting white burnin");

  config_.add_parameter(
    "absolute_thresh_max",
    "192",
    "The absolute threshold above which detections have maximum weight.\n"
    "This value is relative to detecting white burnin");
}


metadata_burnin_filter_process
::~metadata_burnin_filter_process()
{
}


config_block
metadata_burnin_filter_process
::params() const
{
  return config_;
}


bool
metadata_burnin_filter_process
::set_params( config_block const& blk )
{

  try
  {
    blk.get( "kernel_width", kernel_width_ );
    blk.get( "kernel_height", kernel_height_ );
    blk.get( "interlaced", interlaced_ );
    blk.get( "adaptive_thresh_max", adapt_thresh_high_ );
    blk.get( "absolute_thresh_min", abs_thresh_low_ );
    blk.get( "absolute_thresh_max", abs_thresh_high_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
metadata_burnin_filter_process
::initialize()
{
  return true;
}


/// helper function to return an image view of the even rows of an image
template <typename T>
inline vil_image_view<T> even_rows(const vil_image_view<T>& im)
{
  return vil_image_view<T>(im.memory_chunk(), im.top_left_ptr(),
                           im.ni(), (im.nj()+1)/2, im.nplanes(),
                           im.istep(), im.jstep()*2, im.planestep());
}


/// helper function to return an image view of the odd rows of an image
template <typename T>
inline vil_image_view<T> odd_rows(const vil_image_view<T>& im)
{
  return vil_image_view<T>(im.memory_chunk(), im.top_left_ptr()+im.jstep(),
                           im.ni(), (im.nj())/2, im.nplanes(),
                           im.istep(), im.jstep()*2, im.planestep());
}


bool
metadata_burnin_filter_process
::step()
{
  vil_image_view<vxl_byte> grey_img, box_vert_smooth;

  // if the image is not already grey, convert to grey
  if (image_.nplanes() == 3)
  {
    vil_convert_planes_to_grey(image_, grey_img);
  }
  else
  {
    grey_img = image_;
  }

  // make the output image and create views of each plane
  filtered_image_ = vil_image_view<vxl_byte>(grey_img.ni(), grey_img.nj(), 3);
  vil_image_view<vxl_byte> filter_x = vil_plane(filtered_image_, 0);
  vil_image_view<vxl_byte> filter_y = vil_plane(filtered_image_, 1);
  vil_image_view<vxl_byte> filter_xy = vil_plane(filtered_image_, 2);
  // comptue the vertically smoothed image
  box_vert_smooth.set_size(grey_img.ni(), grey_img.nj(), 1);
  if (interlaced_)
  {
    // if interlaced, split the image into odd and even views
    // transpose all input and ouput images so that the horizontal
    // smoothing function produces vertical smoothing
    vil_image_view<vxl_byte> im_even_t = vil_transpose(even_rows(grey_img));
    vil_image_view<vxl_byte> im_odd_t = vil_transpose(odd_rows(grey_img));

    vil_image_view<vxl_byte> smooth_even_t = vil_transpose(even_rows(box_vert_smooth));
    vil_image_view<vxl_byte> smooth_odd_t = vil_transpose(odd_rows(box_vert_smooth));

    // use a half size odd kernel since images are half height
    int half_kernel_height = kernel_height_ / 2;
    if (half_kernel_height % 2 == 0)
    {
      ++half_kernel_height;
    }

    box_filter_1d(im_even_t, smooth_even_t, half_kernel_height);
    box_filter_1d(im_odd_t, smooth_odd_t, half_kernel_height);
  }
  else
  {
    // if not interlaced, transpose inputs and outputs and apply horizontal smoothing.
    vil_image_view<vxl_byte> grey_img_t = vil_transpose(grey_img);
    vil_image_view<vxl_byte> smooth_t = vil_transpose(box_vert_smooth);
    box_filter_1d(grey_img_t, smooth_t, kernel_height_);
  }
  // apply horizontal smoothing to the vertical smoothed image to get a 2D box filter
  // temporarily store the result in filter_xy
  box_filter_1d(box_vert_smooth, filter_xy, kernel_width_);

  vil_image_view<vxl_byte> white_det;
  unsigned long num_white, num_black;
  // apply the white burnin detector and count the number of maximum responses
  num_white = detect_burnin(grey_img, filter_xy, white_det, false);
  // apply the blac burnin detector in place and count the number of maximum responses
  num_black = detect_burnin(grey_img, filter_xy, filter_xy, true);

  // if white burnin had higher count, switch output to white
  is_black_ = true;
  if (num_white > num_black)
  {
    filter_xy.deep_copy(white_det);
    is_black_ = false;
  }

  // use the vertically smoothed image to detect horizontal response
  detect_burnin(grey_img, box_vert_smooth, filter_x, is_black_);
  // smooth the image horizontally and detect the vertical response
  box_filter_1d(grey_img, filter_y, kernel_width_);
  detect_burnin(grey_img, filter_y, filter_y, is_black_);


  return true;
}


void
metadata_burnin_filter_process
::set_source_image( vil_image_view<vxl_byte> const& img )
{
  image_ = img;
}


vil_image_view<vxl_byte> const&
metadata_burnin_filter_process
::metadata_image() const
{
  return filtered_image_;
}


bool
metadata_burnin_filter_process
::metadata_is_black() const
{
  return is_black_;
}


/// helper function to clamp a value between low and high
template <typename T>
inline T clamp(T val, T low, T high)
{
  return val<low?low:(val>high?high:val);
}


/// \brief detect metadata burnin
/// The detection algorithm produces the normalized product of two images:
/// an adaptive threshold image and an absolute threshold image.
/// Each image is clamped between lower and upper thresholds and shifted
/// to a base value of zero before multiplying.
/// The adaptive threshold image is produced by subracting as smoothed
/// version of the image from the original image.
/// Black burnin detection is the same as white burnin detection but
/// all the image intensities are negated.
/// \returns A count of the number pixels above both upper thresholds.
unsigned long
metadata_burnin_filter_process
::detect_burnin(const vil_image_view<vxl_byte>& image,
                const vil_image_view<vxl_byte>& smooth,
                vil_image_view<vxl_byte>& weight,
                bool black)
{
  unsigned ni = image.ni(), nj = image.nj(), np = image.nplanes();
  assert(smooth.ni() == ni);
  assert(smooth.nj() == nj);
  assert(smooth.nplanes() == np);
  weight.set_size(ni,nj,np);

  double scale = 255.0 / (adapt_thresh_high_ * (abs_thresh_high_ - abs_thresh_low_));

  vcl_ptrdiff_t istepI=image.istep(),  jstepI=image.jstep(),  pstepI=image.planestep();
  vcl_ptrdiff_t istepS=smooth.istep(), jstepS=smooth.jstep(), pstepS=smooth.planestep();
  vcl_ptrdiff_t istepW=weight.istep(), jstepW=weight.jstep(), pstepW=weight.planestep();

  const vxl_byte*   planeI = image.top_left_ptr();
  const vxl_byte*   planeS = smooth.top_left_ptr();
  vxl_byte*         planeW = weight.top_left_ptr();

  unsigned long max_count = 0;
  for (unsigned p=0; p<np; ++p, planeI+=pstepI, planeS+=pstepS, planeW+=pstepW)
  {
    const vxl_byte* rowI = planeI;
    const vxl_byte* rowS = planeS;
    vxl_byte*       rowW = planeW;
    for (unsigned j=0; j<nj; ++j, rowI += jstepI, rowS += jstepS, rowW += jstepW)
    {
      const vxl_byte* pixelI = rowI;
      const vxl_byte* pixelS = rowS;
      vxl_byte*       pixelW = rowW;
      for (unsigned i=0; i<ni; ++i, pixelI += istepI, pixelS += istepS, pixelW += istepW)
      {
        int diff = *pixelI - *pixelS;
        int val = *pixelI;
        if (black)
        {
          diff *= -1;
          val = 255 - val;
        }
        diff = clamp(diff, 0, adapt_thresh_high_);

        val = clamp(val, abs_thresh_low_, abs_thresh_high_);
        val -= abs_thresh_low_;

        *pixelW = static_cast<vxl_byte>(scale * (val * diff) + 0.5);
        if (*pixelW == 255)
        {
          ++max_count;
        }
      }
    }
  }
  return max_count;
}

} // end namespace vidtk
