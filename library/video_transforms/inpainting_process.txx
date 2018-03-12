/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "inpainting_process.h"

#include <video_transforms/nearest_neighbor_inpaint.h>
#include <video_transforms/warp_image.h>

#include <vil/vil_convert.h>
#include <vil/vil_fill.h>
#include <vil/vil_copy.h>
#include <vil/vil_save.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include <logger/logger.h>

#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

VIDTK_LOGGER( "inpainting_process_txx" );

#ifdef INPAINTER_WRITE_WARPED
static unsigned counter = 0;
#endif

namespace vidtk
{

template <typename PixType>
inpainting_process<PixType>
::inpainting_process( std::string const& _name )
  : process( _name, "inpainting_process" ),
    disabled_( false ),
    algorithm_( NEAREST ),
    radius_( 8.0 ),
    stab_image_factor_( 0.0 ),
    unstab_image_factor_( 0.0 ),
    border_dilation_( 1 ),
    vert_center_ignore_( 0.0 ),
    hori_center_ignore_( 0.0 ),
    use_mosaic_( false ),
    max_buffer_size_( 0 ),
    min_size_for_hist_( 0 ),
    flush_trigger_( 0.90 ),
    illum_trigger_( 0.00 ),
    buffering_enabled_( false ),
    is_buffering_( false )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Disable this process, causing the image to pass through unmodified." );

  config_.add_parameter(
    "algorithm",
    "nearest",
    "The algorithm to use when inpainting. Options: nearest, telea, or "
    "navier for nearest neighbor inpainting, Telea's fast marching method, "
    "or the Navier-Strokes method respectively." );

  config_.add_parameter(
    "radius",
    "8.0",
    "Radius around the inpainted area that is considered for the Telea and "
    "NS algorithms." );

  config_.add_parameter(
    "stab_image_factor",
    "0.0",
    "If a stabilized image is provided, the percent of weight to place on "
    "on using it in place of the single frame inpainting method." );

  config_.add_parameter(
    "unstab_image_factor",
    "0.0",
    "Amount to average inpainted pixels on the current frame with the last "
    "inpainted frame." );

  config_.add_parameter(
    "use_mosaic",
    "false",
    "If stab_image_factor is non-zero, whether or not to use an image mosaic "
    "as opposed to individual frame warpings." );

  config_.add_parameter(
    "core_count",
    "8",
    "This parameter will attempt to optimize the internal threading system "
    "for a CPU containing this number of cores. Setting the parameter to 1 "
    "will disable multi-threading." );

  config_.add_parameter(
    "border_method",
    "fill_solid",
    "Option for how detections within the image border are handled. Can either "
    "be fill_solid or inpaint." );

  config_.add_parameter(
    "border_dilation",
    "0",
    "Pixel count for how much the border should be dilated before the inpainting "
    "or fill operation." );

  config_.add_parameter(
    "max_buffer_size",
    "0",
    "If we are using a mosaic to find replacement pixel values, the amount of "
    "frames at the beginning of each shot to buffer at maximum." );

  config_.add_parameter(
    "min_size_for_hist",
    "0",
    "Minimum shot size required to use historical information." );

  config_.add_parameter(
    "flush_trigger",
    "0.90",
    "If buffering is enabled, if the overlap between the current and original "
    "frame is less than this threshold, the buffer will be flushed." );

  config_.add_parameter(
    "illum_trigger",
    "0.00",
    "If buffering is enabled and there is a large illumination change, reset "
    "the internal mosaic.");

  config_.add_parameter(
    "retain_center",
    "disabled",
    "Option for what to do in the center of images. Can either be disabled (to "
    "inpaint detected crosshairs), enabled (to adaptively estimate how much "
    "center should be left unmodified), or two values specified as a pair "
    "\"h:w\" where h is percentage of pixels in the height dimension to leave "
    "untouched, and w for width (e.g. a value of 0.5:0.5 will leave the inner "
    "half of the image unmodified)." );

  config_.merge( moving_mosaic_settings().config() );
}


template <typename PixType>
inpainting_process<PixType>
::~inpainting_process()
{
}

template <typename PixType>
config_block
inpainting_process<PixType>
::params() const
{
  return config_;
}

template <typename PixType>
bool
inpainting_process<PixType>
::set_params( config_block const& blk )
{
  std::string algorithm, retain_center;

  try
  {
    disabled_ = blk.get<bool>( "disabled" );

    if( !disabled_ )
    {
      algorithm = blk.get<std::string>( "algorithm" );
      radius_ = blk.get<double>( "radius" );
      stab_image_factor_ = blk.get<double>( "stab_image_factor" );
      unstab_image_factor_ = blk.get<double>( "unstab_image_factor" );
      border_dilation_ = blk.get<unsigned>( "border_dilation" );

      if( algorithm == "telea" )
      {
        this->algorithm_ = TELEA;
      }
      else if( algorithm == "navier" )
      {
        this->algorithm_ = NAVIER;
      }
      else if( algorithm == "nearest" )
      {
        this->algorithm_ = NEAREST;
      }
      else if( algorithm == "none" )
      {
        this->algorithm_ = NONE;
      }
      else
      {
        throw config_block_parse_error( "Invalid inpainting algorithm: " + algorithm );
      }

      std::string border_method = blk.get<std::string>( "border_method" );

      if( border_method == "fill_solid" )
      {
        this->border_method_ = FILL_SOLID;
      }
      else if( border_method == "inpaint" )
      {
        this->border_method_ = INPAINT;
      }
      else
      {
        throw config_block_parse_error( "Invalid border method: " + border_method );
      }

      unsigned core_count = blk.get<unsigned>( "core_count" );

      unsigned thread_grid_width, thread_grid_height;

      if( core_count <= 1 || algorithm_ == NEAREST )
      {
        thread_grid_width = 1;
        thread_grid_height = 1;
      }
      else if( core_count >= 35 )
      {
        thread_grid_width = 7;
        thread_grid_height = 5;
      }
      else if( core_count >= 25 )
      {
        thread_grid_width = 5;
        thread_grid_height = 5;
      }
      else if( core_count >= 15 )
      {
        thread_grid_width = 5;
        thread_grid_height = 3;
      }
      else
      {
        thread_grid_width = 3;
        thread_grid_height = 3;
      }

      if( algorithm_ != NONE )
      {
        int boundary_adj = ( algorithm_ != NEAREST && core_count > 1 ? 1 : 0 );
        threads_.reset( new thread_sys_t( thread_grid_width, thread_grid_height ) );
        threads_->set_function( boost::bind( &self_type::inpaint, this, _1, _2, _3 ) );
        threads_->set_border_mode( ( border_method_ != FILL_SOLID ), 0, boundary_adj );
      }

      use_mosaic_ = blk.get< bool >( "use_mosaic" );
      max_buffer_size_ = blk.get< unsigned >( "max_buffer_size" );
      min_size_for_hist_ = blk.get< unsigned >( "min_size_for_hist" );
      flush_trigger_ = blk.get< double >( "flush_trigger" );
      illum_trigger_ = blk.get< double >( "illum_trigger" );

      if( use_mosaic_ )
      {
        moving_mosaic_settings mmg_options;
        mmg_options.read_config( blk );

        if( !mmg_.configure( mmg_options ) )
        {
          throw config_block_parse_error( "Unable to configure mosaic generator" );
        }

        if( max_buffer_size_ > 0 )
        {
          buffering_enabled_ = true;
          is_buffering_ = true;
        }
        else
        {
          is_buffering_ = false;
          buffering_enabled_ = false;
        }
      }
      else if( max_buffer_size_ > 0 )
      {
        throw config_block_parse_error( "Mosaicing must be turned on to use buffer" );
      }
      else
      {
        is_buffering_ = false;
        buffering_enabled_ = false;
      }
    }

    retain_center = blk.get< std::string >( "retain_center" );

    if( retain_center != "disable" &&
        retain_center != "disabled" &&
        retain_center != "false" )
    {
      if( retain_center == "enable" ||
          retain_center == "enabled" ||
          retain_center == "true" )
      {
        vert_center_ignore_ = -1;
        hori_center_ignore_ = -1;
      }
      else
      {
        size_t index1 = retain_center.find( ":" ) + 1;

        vert_center_ignore_ = boost::lexical_cast< double >(
          retain_center.substr( 0, index1-1 ) );
        hori_center_ignore_ = boost::lexical_cast< double >(
          retain_center.substr( index1 ) );
      }
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( "Set_params failed: " << e.what() );
    return false;
  }
  catch( const boost::bad_lexical_cast& )
  {
    LOG_ERROR( "Unable to parse retain_center option " << retain_center );
    return false;
  }

  if( this->algorithm_ == TELEA || this->algorithm_ == NAVIER )
  {
#ifndef USE_OPENCV
    LOG_ERROR( "VIDTK must be built with opencv to use Telea or NS algorithms" );
    return false;
#else
    if( vil_pixel_format_of( PixType() ) != vil_pixel_format_of( vxl_byte() ) )
    {
      LOG_ERROR( "OpenCV inpaint only supports 8-bit inputs at this time" );
      return false;
    }
#endif
  }

  config_.update( blk );
  return true;
}

template <typename PixType>
bool
inpainting_process<PixType>
::initialize()
{
  return true;
}

namespace {

template <typename PixType>
void merge_in_warped( vil_image_view< PixType >& source_image,
                      const vil_image_view< bool >& source_mask,
                      const vil_image_view< PixType >& warped_image,
                      const vil_image_view< bool >& warped_mask,
                      const double warped_weight )
{
  LOG_ASSERT( source_image.ni() == source_mask.ni(), "Dimensions do not agree" );
  LOG_ASSERT( source_image.nj() == source_mask.nj(), "Dimensions do not agree" );
  LOG_ASSERT( source_mask.ni() == warped_image.ni(), "Dimensions do not agree" );
  LOG_ASSERT( source_mask.nj() == warped_image.nj(), "Dimensions do not agree" );
  LOG_ASSERT( warped_image.ni() == warped_mask.ni(), "Dimensions do not agree" );
  LOG_ASSERT( warped_image.nj() == warped_mask.nj(), "Dimensions do not agree" );

  for( unsigned j = 0; j < source_image.nj(); ++j )
  {
    const bool* mask_ptr = &source_mask( 0, j );

    for( unsigned i = 0; i < source_image.ni(); ++i, mask_ptr += source_mask.istep() )
    {
      if( *mask_ptr && !warped_mask( i, j ) )
      {
        for( unsigned p = 0; p < source_image.nplanes(); ++p )
        {
          source_image( i, j, p ) = static_cast< PixType >(
            ( 1.0 - warped_weight ) * source_image( i, j, p ) +
            ( warped_weight ) * warped_image( i, j, p ) + 0.5 );
        }
      }
    }
  }
}

template <typename PixType>
void fill_in_border( vil_image_view< PixType >& image,
                     const image_border& border,
                     PixType value,
                     int border_dilation = 0 )
{
  for( int i = 0; i < border.min_x() + border_dilation + 1; ++i )
  {
    vil_fill_col( image, i, value );
  }
  for( int i = border.max_x() - border_dilation - 1;
           i < static_cast<int>( image.ni() ); ++i )
  {
    vil_fill_col( image, i, value );
  }
  for( int j = 0; j < border.min_y() + border_dilation + 1; ++j )
  {
    vil_fill_row( image, j, value );
  }
  for( int j = border.max_y() - border_dilation - 1;
           j < static_cast<int>( image.nj() ); ++j )
  {
    vil_fill_row( image, j, value );
  }
}

inline bool
union_functor( bool x1, bool x2 )
{
  return x1 || x2;
}

inline bool
filter_functor( bool x1, bool x2 )
{
  return x1 || x2;
}

}

template <typename PixType>
process::step_status
inpainting_process<PixType>
::step2()
{
  // Validate inputs
  if( disabled_ )
  {
    inpainted_image_ = input_image_;
    return process::SUCCESS;
  }

  if( input_mask_.ni() == 0 &&
      input_mask_.nj() == 0 )
  {
    input_mask_.set_size( input_image_.ni(), input_image_.nj() );
    input_mask_.fill( false );
  }

  if( input_border_.area() == 0 )
  {
    input_border_ = border_t( 0, input_image_.ni(), 0, input_image_.nj() );
  }

  if( input_image_.ni() != input_mask_.ni() ||
      input_image_.nj() != input_mask_.nj() )
  {
    LOG_ERROR( "Input image and mask do not have the same size." );
    return process::FAILURE;
  }

  if( input_image_.nplanes() != 1 && input_image_.nplanes() != 3 )
  {
    LOG_ERROR( "Input image does not have either 1 or 3 channels." );
    return process::FAILURE;
  }

  if( input_mask_.nplanes() != 1 )
  {
    LOG_ERROR( "Mask image does not have a single channel." );
    return process::FAILURE;
  }

  // Check illumination shot break criteria if enabled
  bool illumination_change = false;

  if( illum_trigger_ > 0.0 &&
      last_input_image_.ni() == input_image_.ni() &&
      last_input_image_.nj() == input_image_.nj() &&
      input_image_.size() > 0 )
  {
    // Perform check
    double diff = vil_math_image_abs_difference( input_image_, last_input_image_ );

    if( diff / input_image_.size() > illum_trigger_ )
    {
      illumination_change = true;
    }
  }

  // Handle buffering mode if enabled
  if( buffering_enabled_ )
  {
    bool new_shot =
      ( !input_homography_.is_valid() ||
        input_ts_ != input_homography_.get_source_reference() ||
        input_homography_.get_dest_reference() != last_homography_.get_dest_reference() ||
        input_homography_.is_new_reference() );

    if( new_shot || illumination_change )
    {
      flush_buffer( illumination_change );
      is_buffering_ = true;
    }
    else if( is_buffering_ && buffer_.size() >= max_buffer_size_ )
    {
      mmg_.inpaint_mosaic();
      flush_buffer( false );
      is_buffering_ = false;
    }
  }
  else if( illumination_change && use_mosaic_ )
  {
    // Handle illumination change detection if no buffering
    mmg_.reset_mosaic();
  }

  // Create new output image
  inpainted_image_ = image_t( input_image_.ni(),
                              input_image_.nj(),
                              1,
                              input_image_.nplanes() );

  // Compute adjusted motion mask if provided
  if( input_motion_mask_.size() > 0 )
  {
    if( input_motion_mask_.ni() != input_image_.ni() ||
        input_motion_mask_.nj() != input_image_.nj() )
    {
      adj_motion_mask_ = mask_t( input_image_.ni(), input_image_.nj() );
      vil_fill( adj_motion_mask_, false );

      border_t center_region( 0, input_motion_mask_.ni(), 0, input_motion_mask_.nj() );
      center_region.set_centroid_x( input_image_.ni() / 2 );
      center_region.set_centroid_y( input_image_.nj() / 2 );

      mask_t output_region = point_view_to_region( adj_motion_mask_, center_region );
      vil_copy_reformat( input_motion_mask_, output_region );
    }
    else
    {
      adj_motion_mask_ = input_motion_mask_;
    }
  }

  // Compute regions to inpaint
  mask_t inpaint_mask;
  border_t center_region;

  if( vert_center_ignore_ == 0.0 || hori_center_ignore_ == 0.0 )
  {
    inpaint_mask = input_mask_;
  }
  else
  {
    vil_copy_deep( input_mask_, inpaint_mask );

    if( vert_center_ignore_ == -1 || hori_center_ignore_ == -1 )
    {
      if( inpaint_mask.size() < 500000 )
      {
        center_region = border_t( 0, 0.500 * input_border_.width(),
                                  0, 0.520 * input_border_.height() );
      }
      else
      {
        center_region = border_t( 0, 0.560 * input_border_.width(),
                                  0, 0.575 * input_border_.height() );
      }
    }
    else
    {
      center_region = border_t( 0, hori_center_ignore_ * input_mask_.ni(),
                                0, vert_center_ignore_ * input_mask_.nj() );
    }

    center_region.set_centroid_x( inpaint_mask.ni() / 2 );
    center_region.set_centroid_y( inpaint_mask.nj() / 2 );

    point_view_to_region( inpaint_mask, center_region ).fill( false );
  }

  // Perform actual inpainting
  if( stab_image_factor_ > 0.0 )
  {
    if( algorithm_ != NONE )
    {
      boost::thread aux_thread(
        boost::bind( &inpainting_process<PixType>::compute_warped_image, this ) );

      threads_->apply_function( input_border_, input_image_, inpaint_mask, inpainted_image_ );

      aux_thread.join();
    }
    else
    {
      compute_warped_image();
    }
  }
  else if( algorithm_ != NONE )
  {
    threads_->apply_function( input_border_, input_image_, inpaint_mask, inpainted_image_ );
  }

  // Average in warped image if set
  if( stab_image_factor_ > 0.0 && warped_mask_.size() > 0 && !is_buffering_ )
  {
    if( adj_motion_mask_.size() > 0 )
    {
      vil_transform( warped_mask_,
                     adj_motion_mask_,
                     warped_mask_,
                     filter_functor );
    }

    if( input_border_.area() > 0 )
    {
      image_t inpainted_region = point_view_to_region( inpainted_image_, input_border_ );

      merge_in_warped( inpainted_region,
                       point_view_to_region( inpaint_mask, input_border_ ),
                       point_view_to_region( warped_image_, input_border_ ),
                       point_view_to_region( warped_mask_, input_border_ ),
                       stab_image_factor_ );
    }
    else
    {
      merge_in_warped( inpainted_image_,
                       inpaint_mask,
                       warped_image_,
                       warped_mask_,
                       stab_image_factor_ );
    }
  }

  // Average in unwarped image if set.
  if( unstab_image_factor_ > 0.0 && last_output_image_.size() > 0 )
  {
    if( input_border_.area() > 0 )
    {
      image_t inpainted_region = point_view_to_region( inpainted_image_, input_border_ );

      merge_in_warped( inpainted_region,
                       point_view_to_region( inpaint_mask, input_border_ ),
                       point_view_to_region( last_output_image_, input_border_ ),
                       point_view_to_region( last_unpainted_mask_, input_border_ ),
                       unstab_image_factor_ );
    }
    else
    {
      merge_in_warped( inpainted_image_,
                       inpaint_mask,
                       last_output_image_,
                       last_unpainted_mask_,
                       unstab_image_factor_ );
    }
  }

  // Fill border in inpainted image if enabled
  if( input_border_.area() > 0 && border_method_ == FILL_SOLID )
  {
    const PixType fill_val = 0;

    for( unsigned p = 0; p < inpainted_image_.nplanes(); ++p )
    {
      image_t single_plane = vil_plane( inpainted_image_, p );
      fill_in_border( single_plane, input_border_, fill_val, border_dilation_ );
    }
  }

  // Generate uninpainted mask, remember previous outputs
  if( unstab_image_factor_ > 0.0 || stab_image_factor_ > 0.0 )
  {
    last_unpainted_mask_.set_size( inpainted_image_.ni(), inpainted_image_.nj() );
    vil_fill( last_unpainted_mask_, false );

    if( input_border_.area() > 0 )
    {
      fill_in_border( last_unpainted_mask_, input_border_, true, border_dilation_ );
    }

    if( center_region.area() > 0 )
    {
      mask_t input_reg = point_view_to_region( input_mask_, center_region );
      mask_t output_reg = point_view_to_region( last_unpainted_mask_, center_region );

      vil_copy_reformat( input_reg, output_reg );
    }

    if( input_sbf_.is_shot_end() )
    {
      last_input_image_ = image_t();
      last_output_image_ = image_t();
      last_unpainted_mask_ = mask_t();
      last_homography_ = homography_t();
    }
    else
    {
      last_input_image_ = input_image_;
      last_output_image_ = inpainted_image_;
      last_homography_ = input_homography_;
    }
  }

  // Handle buffering option if enabled
  if( is_buffering_ )
  {
    buffer_.push_back(
      buffered_entry_t(
        inpainted_image_,
        inpaint_mask,
        input_homography_,
        input_border_,
        input_ts_ ) );

    if( reference_overlap_ < flush_trigger_ )
    {
      flush_buffer();
      is_buffering_ = false;
    }

    return process::NO_OUTPUT;
  }

#ifdef INPAINTER_WRITE_WARPED
  std::string id = boost::lexical_cast< std::string >( counter );

  while( id.size() < 3 )
  {
    id = "0" + id;
  }

  std::string fn3 = "inpainted" + id + ".ppm";
  vil_save( inpainted_image_, fn3.c_str() );
#endif

  return process::SUCCESS;
}

template <typename PixType>
void
inpainting_process<PixType>
::inpaint( const image_t input, const mask_t mask, image_t output ) const
{
  // Validate input image
  if( !input || input.size() == 0 || algorithm_ == NONE )
  {
    return;
  }

  // Perform algorithm
  if( algorithm_ == NEAREST )
  {
    vil_copy_reformat( input, output );
    nn_inpaint( output, mask );
  }
#ifdef USE_OPENCV
  else if( algorithm_ == TELEA || algorithm_ == NAVIER )
  {
    // Convert image and mask to opencv format
    image_t copied_input = image_t( input.ni(),
                                    input.nj(),
                                    1,
                                    input.nplanes() );

    vil_copy_reformat( input, copied_input );

    vil_image_view< vxl_byte > byte_mask;
    vil_convert_stretch_range< bool >( mask, byte_mask );

    cv::Mat ocv_input( copied_input.nj(),
                       copied_input.ni(),
                       ( copied_input.nplanes() == 1 ) ? CV_8UC1 : CV_8UC3,
                       copied_input.top_left_ptr(),
                       copied_input.jstep() * sizeof( PixType ) );

    cv::Mat ocv_mask( byte_mask.nj(),
                      byte_mask.ni(),
                      CV_8UC1,
                      byte_mask.top_left_ptr(),
                      byte_mask.jstep() * sizeof( vxl_byte ) );

    // Create output image
    cv::Mat output_wrapper( output.nj(),
                            output.ni(),
                            ocv_input.type(),
                            output.top_left_ptr(),
                            output.jstep() * sizeof( PixType ) );

    // Run opencv inpaint
    int method = ( algorithm_ == TELEA ? CV_INPAINT_TELEA : CV_INPAINT_NS );
    cv::inpaint( ocv_input, ocv_mask, output_wrapper, radius_, method );
  }
#endif
}

template <typename PixType>
void
inpainting_process<PixType>
::compute_warped_image()
{
  if( !use_mosaic_ )
  {
    if( last_output_image_.size() == 0 ||
        input_homography_.is_new_reference() ||
        !input_homography_.is_valid() ||
        input_homography_.get_dest_reference() != last_homography_.get_dest_reference() ||
        input_ts_ != input_homography_.get_source_reference() )
    {
      warped_image_ = image_t();
      warped_mask_ = mask_t();
      return;
    }

    warped_image_.set_size( input_image_.ni(), input_image_.nj(), input_image_.nplanes() );
    warped_mask_.set_size( input_image_.ni(), input_image_.nj(), 1 );

    // Compute warping homography
    vgl_h_matrix_2d< double > prior_to_current;

    prior_to_current = ( last_homography_.get_transform().get_inverse() *
                         input_homography_.get_transform() );

    // Perform warping only for required pixels
    warp_image_parameters wip;

    warp_image( last_output_image_, warped_image_, prior_to_current, wip );

    wip.set_interpolator( warp_image_parameters::NEAREST );
    wip.set_unmapped_value( 1.0 );

    warp_image( last_unpainted_mask_, warped_mask_, prior_to_current, wip );
  }
  else
  {
    if( input_homography_.is_valid() &&
        input_ts_ == input_homography_.get_source_reference() )
    {
      mask_t mask_w_border;

      if( input_border_.area() > 0 )
      {
        mask_w_border.deep_copy( input_mask_ );
        fill_in_border( mask_w_border, input_border_, true, border_dilation_+1 );
      }
      else
      {
        mask_w_border = input_mask_;
      }

      if( adj_motion_mask_.size() > 0 )
      {
        vil_transform( mask_w_border,
                       adj_motion_mask_,
                       mask_w_border,
                       union_functor );
      }

      reference_overlap_ = mmg_.add_image( input_image_, mask_w_border, input_homography_ );

      if( is_buffering_ ||
          !mmg_.get_pixels( input_mask_, input_homography_, warped_image_, warped_mask_ ) )
      {
        warped_image_ = image_t();
        warped_mask_ = mask_t();
      }
    }
    else
    {
      warped_image_ = image_t();
      warped_mask_ = mask_t();
    }
  }

  // Debug code
#ifdef INPAINTER_WRITE_WARPED
  if( !is_buffering_ )
  {
    std::string id = boost::lexical_cast< std::string >( ++counter );

    while( id.size() < 3 )
    {
      id = "0" + id;
    }

    std::string fn1 = "image" + id + ".ppm";
    std::string fn2 = "mask" + id + ".ppm";

    vil_save( warped_image_, fn1.c_str() );
    vil_save( warped_mask_, fn2.c_str() );
  }
#endif
}

template <typename PixType>
void
inpainting_process<PixType>
::flush_buffer( bool reset_mosaic )
{
  for( unsigned i = 0; i < buffer_.size(); ++i )
  {
    buffered_entry_t& entry = buffer_[i];

    if( entry.ts != entry.homog.get_source_reference() ||
        buffer_.size() < min_size_for_hist_ ||
        !mmg_.get_pixels( entry.mask, entry.homog, warped_image_, warped_mask_ ) )
    {
      warped_image_ = image_t();
      warped_mask_ = mask_t();
    }

    if( stab_image_factor_ > 0.0 && warped_mask_.size() > 0 )
    {
      if( entry.border.area() > 0 )
      {
        image_t inpainted_region = point_view_to_region( entry.image, entry.border );

        merge_in_warped( inpainted_region,
                         point_view_to_region( entry.mask, entry.border ),
                         point_view_to_region( warped_image_, entry.border ),
                         point_view_to_region( warped_mask_, entry.border ),
                         stab_image_factor_ );
      }
      else
      {
        merge_in_warped( entry.image,
                         entry.mask,
                         warped_image_,
                         warped_mask_,
                         stab_image_factor_ );
      }
    }

    inpainted_image_ = entry.image;

    if( entry.border.area() > 0 && border_method_ == FILL_SOLID )
    {
      const PixType fill_val = 0;

      for( unsigned p = 0; p < inpainted_image_.nplanes(); ++p )
      {
        image_t single_plane = vil_plane( inpainted_image_, p );
        fill_in_border( single_plane, entry.border, fill_val, border_dilation_ );
      }
    }

#ifdef INPAINTER_WRITE_WARPED
    std::string id = boost::lexical_cast< std::string >( ++counter );

    while( id.size() < 3 )
    {
      id = "0" + id;
    }

    std::string fn1 = "image" + id + ".ppm";
    std::string fn2 = "mask" + id + ".ppm";
    std::string fn3 = "inpainted" + id + ".ppm";

    vil_save( warped_image_, fn1.c_str() );
    vil_save( warped_mask_, fn2.c_str() );
    vil_save( inpainted_image_, fn3.c_str() );
#endif

    push_output( process::SUCCESS );
  }

  if( reset_mosaic )
  {
    mmg_.reset_mosaic();
  }

  buffer_.clear();
}

template <typename PixType>
void
inpainting_process<PixType>
::set_source_image( image_t const& img )
{
  input_image_ = img;
}

template <typename PixType>
void
inpainting_process<PixType>
::set_source_timestamp( vidtk::timestamp const& ts )
{
  input_ts_ = ts;
}

template <typename PixType>
void
inpainting_process<PixType>
::set_mask( mask_t const& img )
{
  input_mask_ = img;
}

template <typename PixType>
void
inpainting_process<PixType>
::set_motion_mask( mask_t const& img )
{
  input_motion_mask_ = img;
}

template <typename PixType>
void
inpainting_process<PixType>
::set_border( border_t const& border )
{
  input_border_ = border;
}

template <typename PixType>
void
inpainting_process<PixType>
::set_homography( homography_t const& h )
{
  input_homography_ = h;
}

template <typename PixType>
void
inpainting_process<PixType>
::set_type( std::string const& s )
{
  input_type_ = s;
}

template <typename PixType>
void
inpainting_process<PixType>
::set_shot_break_flags( shot_break_flags const& sbf )
{
  input_sbf_ = sbf;
}

template <typename PixType>
typename inpainting_process<PixType>::image_t
inpainting_process<PixType>
::inpainted_image() const
{
  return this->inpainted_image_;
}

} // end namespace vidtk
