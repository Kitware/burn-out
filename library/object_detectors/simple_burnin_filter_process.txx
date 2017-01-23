/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "simple_burnin_filter_process.h"

#include <vxl_config.h>

#include <vil/vil_transpose.h>
#include <vil/vil_math.h>
#include <vil/vil_convert.h>

#include <video_transforms/high_pass_filter.h>

#include <utilities/point_view_to_region.h>

#include <limits>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_simple_burnin_filter_process_cxx__
VIDTK_LOGGER( "simple_burnin_filter_process_cxx" );


namespace vidtk
{


template< typename PixType >
simple_burnin_filter_process< PixType >
::simple_burnin_filter_process( std::string const& _name )
  : process( _name, "simple_burnin_filter_process" ),
    use_target_intensity_( false )
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
    "This value is relative to detecting white burnin" );

  config_.add_parameter(
    "absolute_thresh_max",
    "192",
    "The absolute threshold above which detections have maximum weight.\n"
    "This value is relative to detecting white burnin" );

  config_.add_parameter(
    "target_intensity",
    "",
    "If specified, this process will always assume that the burnin has "
    "this intensity." );
}


template< typename PixType >
simple_burnin_filter_process< PixType >
::~simple_burnin_filter_process()
{
}


template< typename PixType >
config_block
simple_burnin_filter_process< PixType >
::params() const
{
  return config_;
}


template< typename PixType >
bool
simple_burnin_filter_process< PixType >
::set_params( config_block const& blk )
{

  try
  {
    kernel_width_ = blk.get< int >( "kernel_width" );
    kernel_height_ = blk.get< int >( "kernel_height" );
    interlaced_ = blk.get< bool >( "interlaced" );
    adapt_thresh_high_ = blk.get< int >( "adaptive_thresh_max" );
    abs_thresh_low_ = blk.get< int >( "absolute_thresh_min" );
    abs_thresh_high_ = blk.get< int >( "absolute_thresh_max" );
    use_target_intensity_ = !blk.get< std::string >( "target_intensity" ).empty();

    if( use_target_intensity_ )
    {
      target_intensity_ = blk.get< PixType >( "target_intensity" );
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template< typename PixType >
bool
simple_burnin_filter_process< PixType >
::initialize()
{
  return true;
}


template< typename PixType >
bool
simple_burnin_filter_process< PixType >
::step()
{
  const bool has_osd_info = osd_info_ && osd_info_->actions;

  // First check if this operation is even required
  if( has_osd_info && !osd_info_->actions->template_matching )
  {
    filtered_image_ = vil_image_view< PixType >();
    return true;
  }

  // If the image is not already grey, convert to grey
  vil_image_view< PixType > grey_img, box_vert_smooth;

  if( image_.nplanes() != 1 )
  {
    vil_convert_planes_to_grey( image_, grey_img );
  }
  else
  {
    grey_img = image_;
  }

  // Make the output image and create views of each plane
  filtered_image_ = vil_image_view< PixType >( grey_img.ni(), grey_img.nj(), 3 );
  vil_image_view< PixType > filter_x = vil_plane( filtered_image_, 0 );
  vil_image_view< PixType > filter_y = vil_plane( filtered_image_, 1 );
  vil_image_view< PixType > filter_xy = vil_plane( filtered_image_, 2 );

  // Compute the vertically smoothed image
  box_vert_smooth.set_size( grey_img.ni(), grey_img.nj(), 1 );

  if( interlaced_ )
  {
    // If interlaced, split the image into odd and even views transpose
    // all input and ouput images so that the horizontal smoothing function
    // produces vertical smoothing
    vil_image_view< PixType > im_even_t = vil_transpose( even_rows( grey_img ) );
    vil_image_view< PixType > im_odd_t = vil_transpose( odd_rows( grey_img ) );

    vil_image_view< PixType > smooth_even_t = vil_transpose( even_rows( box_vert_smooth ) );
    vil_image_view <PixType > smooth_odd_t = vil_transpose( odd_rows( box_vert_smooth ) );

    // Use a half size odd kernel since images are half height
    int half_kernel_height = kernel_height_ / 2;

    if( half_kernel_height % 2 == 0 )
    {
      ++half_kernel_height;
    }

    box_average_1d( im_even_t, smooth_even_t, half_kernel_height );
    box_average_1d( im_odd_t, smooth_odd_t, half_kernel_height );
  }
  else
  {
    // If not interlaced, transpose inputs and outputs and apply horizontal smoothing.
    vil_image_view< PixType > grey_img_t = vil_transpose( grey_img );
    vil_image_view< PixType > smooth_t = vil_transpose( box_vert_smooth );

    box_average_1d( grey_img_t, smooth_t, kernel_height_ );
  }

  // Apply horizontal smoothing to the vertical smoothed image to get a 2D box filter
  // temporarily store the result in filter_xy
  box_average_1d( box_vert_smooth, filter_xy, kernel_width_ );

  const PixType black = 0;
  const PixType white = std::numeric_limits< PixType >::max();
  const PixType gray = white / 2;

  PixType avg_intensity;

  // If the burnin color was received as one of the inputs, use that to generate the
  // output filter, otherwise estimate if the color is white or black.
  if( has_osd_info || use_target_intensity_ )
  {
    avg_intensity = ( use_target_intensity_ ? target_intensity_ : osd_info_->properties.intensity_ );
    is_black_ = ( avg_intensity < gray );

    // Apply the burnin detector for the known color
    detect_burnin( grey_img, filter_xy, filter_xy, avg_intensity );
  }
  else
  {
    vil_image_view<PixType> white_det;
    unsigned long num_white, num_black;

    // Apply the white burnin detector and count the number of maximum responses
    num_white = detect_burnin( grey_img, filter_xy, white_det, white );

    // Apply the black burnin detector in place and count the number of maximum responses
    num_black = detect_burnin( grey_img, filter_xy, filter_xy, black );

    // If white burnin had higher count, switch output to white
    if( num_white > num_black )
    {
      filter_xy.deep_copy( white_det );
      is_black_ = false;
      avg_intensity = white;
    }
    else
    {
      is_black_ = true;
      avg_intensity = black;
    }
  }

  // Use the vertically smoothed image to detect horizontal response
  detect_burnin( grey_img, box_vert_smooth, filter_x, avg_intensity );

  // Smooth the image horizontally and detect the vertical response
  box_average_1d( grey_img, filter_y, kernel_width_ );
  detect_burnin( grey_img, filter_y, filter_y, avg_intensity );
  return true;
}


template< typename PixType >
void
simple_burnin_filter_process< PixType >
::set_source_image( vil_image_view<PixType> const& img )
{
  image_ = img;
}


template< typename PixType >
void
simple_burnin_filter_process< PixType >
::set_burnin_properties( osd_properties_sptr_t const& properties )
{
  osd_info_ = properties;
}


template< typename PixType >
vil_image_view<PixType>
simple_burnin_filter_process< PixType >
::metadata_image() const
{
  return filtered_image_;
}


template< typename PixType >
bool
simple_burnin_filter_process< PixType >
::metadata_is_black() const
{
  return is_black_;
}


/// helper function to clamp a value between low and high
template< typename T >
inline T clamp( T val, T low, T high )
{
  return val < low ? low : ( val > high ? high : val );
}


/// \brief Simple metadata burnin filter.
///
/// The detection algorithm produces the normalized product of two images:
/// an adaptive threshold image and an absolute threshold image.
/// Each image is clamped between lower and upper thresholds and shifted
/// to a base value of zero before multiplying. The adaptive threshold image
/// is produced by subracting as smoothed version of the image from the
/// original image.
///
/// \returns A count of the number pixels above both upper thresholds.
template< typename PixType >
unsigned long
simple_burnin_filter_process< PixType >
::detect_burnin( const vil_image_view< PixType >& image,
                 const vil_image_view< PixType >& smooth,
                 vil_image_view< PixType >& weight,
                 const int burnin_intensity )
{
  const unsigned ni = image.ni();
  const unsigned nj = image.nj();
  const unsigned np = image.nplanes();

  const PixType white_value = std::numeric_limits< PixType >::max();

  const bool is_white = ( burnin_intensity == white_value );
  const bool is_black = ( burnin_intensity == 0 );
  const bool is_gray = ( !is_white && !is_black );

  const int gt_adj_factor = white_value + burnin_intensity;
  const int lt_adj_factor = white_value - burnin_intensity;

  assert(smooth.ni() == ni);
  assert(smooth.nj() == nj);
  assert(smooth.nplanes() == np);
  weight.set_size(ni,nj,np);

  const double scale = static_cast<double>( white_value ) /
    ( adapt_thresh_high_ * ( abs_thresh_high_ - abs_thresh_low_ ) );

  std::ptrdiff_t istepI=image.istep(),  jstepI=image.jstep(),  pstepI=image.planestep();
  std::ptrdiff_t istepS=smooth.istep(), jstepS=smooth.jstep(), pstepS=smooth.planestep();
  std::ptrdiff_t istepW=weight.istep(), jstepW=weight.jstep(), pstepW=weight.planestep();

  const PixType* planeI = image.top_left_ptr();
  const PixType* planeS = smooth.top_left_ptr();
  PixType*       planeW = weight.top_left_ptr();

  unsigned long max_count = 0;
  for( unsigned p=0; p<np; ++p, planeI+=pstepI, planeS+=pstepS, planeW+=pstepW )
  {
    const PixType* rowI = planeI;
    const PixType* rowS = planeS;
    PixType*       rowW = planeW;

    for( unsigned j=0; j<nj; ++j, rowI += jstepI, rowS += jstepS, rowW += jstepW )
    {
      const PixType* pixelI = rowI;
      const PixType* pixelS = rowS;
      PixType*       pixelW = rowW;

      for( unsigned i=0; i<ni; ++i, pixelI += istepI, pixelS += istepS, pixelW += istepW )
      {
        int diff = *pixelI - *pixelS;
        int val = *pixelI;

        if( is_black )
        {
          diff *= -1;
          val = white_value - val;
        }
        else if( is_gray )
        {
          diff = std::abs( diff );
          val = ( val > burnin_intensity ? gt_adj_factor - val : lt_adj_factor + val );
        }

        diff = clamp( diff, 0, adapt_thresh_high_ );

        val = clamp( val, abs_thresh_low_, abs_thresh_high_ );
        val -= abs_thresh_low_;

        *pixelW = static_cast<PixType>( scale * ( val * diff ) + 0.5 );

        if( *pixelW == white_value )
        {
          ++max_count;
        }
      }
    }
  }
  return max_count;
}

} // end namespace vidtk
