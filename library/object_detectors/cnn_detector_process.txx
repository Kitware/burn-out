/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "cnn_detector_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/config_block.h>
#include <utilities/video_modality.h>

#include <tracking_data/image_object_util.h>

#include <vil/vil_resample_nearest.h>
#include <vil/vil_resample_bilin.h>
#include <vgl/vgl_intersection.h>

#include <string>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "cnn_detector_process" );


template< class PixType >
cnn_detector_process< PixType >
::cnn_detector_process( std::string const& _name )
 : process( _name, "cnn_detector_process" ),
   disabled_( false ),
   scale_( -1.0 ),
   burst_frame_count_( 1 ),
   burst_skip_count_( 0 ),
   burst_skip_mode_( false ),
   burst_counter_( 0 ),
   initial_skip_count_( 0 ),
   initial_no_output_count_( 0 ),
   processed_counter_( 0 ),
   target_scale_( -1.0 ),
   min_scale_factor_( 0.001 ),
   max_scale_factor_( 100.0 ),
   max_operate_scale_( -1.0 ),
   border_pixel_count_( 2 )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Completely disable this process and pass no outputs." );
  config_.add_parameter(
    "burst_frame_count",
    "1",
    "If burst_skip_count is not 0, the number of frames to run the "
    "detector for (in bursts) before skipping a few frames." );
  config_.add_parameter(
    "burst_skip_count",
    "0",
    "After running the detector for burst_frame_count frames, don't"
    "run the detection algorithm for this many frames." );
  config_.add_parameter(
    "initial_skip_count",
    "0",
    "Number of frames to skip (i.e., to not process and just buffer) "
    "when first receiving frames." );
  config_.add_parameter(
    "initial_no_output_count",
    "0",
    "Number of frames to generate a no output pipeline signal for "
    "(i.e., to not process and buffer) when first receiving frames." );
  config_.add_parameter(
    "target_scale",
    "-1.0",
    "If set, resize the input image to this scale before running "
    "the detection algorithm on it." );
  config_.add_parameter(
    "min_scale_factor",
    "0.001",
    "Smallest allowed scale factor, if target scale set." );
  config_.add_parameter(
    "max_scale_factor",
    "100.0",
    "Largest allowed scale factor, if target scale set." );
  config_.add_parameter(
    "max_operate_scale",
    "-1.0",
    "Smallest allowed scale to process, if under this no output will "
    "be produced." );
  config_.add_parameter(
    "lock_settings",
    "false",
    "If set to true, the internal detectors will never be reloaded "
    "even if set_params is called multiple times for this process." );

  config_.merge( cnn_detector_settings().config() );
}


template< class PixType >
cnn_detector_process< PixType >
::~cnn_detector_process()
{
}


template< class PixType >
config_block
cnn_detector_process< PixType >
::params() const
{
  return config_;
}


template< class PixType >
bool
cnn_detector_process< PixType >
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    disabled_ = blk.get<bool>( "disabled" );

    if( disabled_ )
    {
      return true;
    }

    cnn_detector_settings cnn_options;
    cnn_options.read_config( blk );

    if( !detector_ || !blk.get<bool>( "lock_settings" ) )
    {
      detector_.reset( new cnn_detector< PixType >() );

      if( !detector_->configure( cnn_options ) )
      {
        throw config_block_parse_error( "Unable to configure ocv cnn model." );
      }
    }

    burst_frame_count_ = blk.get<unsigned>( "burst_frame_count" );
    burst_skip_count_ = blk.get<unsigned>( "burst_skip_count" );

    initial_skip_count_ = blk.get<unsigned>( "initial_skip_count" );
    initial_no_output_count_ = blk.get<unsigned>( "initial_no_output_count" );

    target_scale_ = blk.get<double>( "target_scale" );

    min_scale_factor_ = blk.get<double>( "min_scale_factor" );
    max_scale_factor_ = blk.get<double>( "max_scale_factor" );

    max_operate_scale_ = blk.get<double>( "max_operate_scale" );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Set internal config block
  config_.update( blk );

  // Reset mask input
  mask_ = vil_image_view< bool >();

  return true;
}


template< class PixType >
bool
cnn_detector_process< PixType >
::initialize()
{
  return true;
}


template< class PixType >
bool
cnn_detector_process< PixType >
::step()
{
  detections_.clear();

  if( disabled_ )
  {
    return true;
  }

  // Handle internal frame downsamplers if enabled
  if( burst_skip_count_ > 0 )
  {
    burst_counter_++;

    if( burst_skip_mode_ && burst_counter_ <= burst_skip_count_ )
    {
      return true;
    }
    else if( burst_skip_mode_ )
    {
      burst_counter_ = 1;
      burst_skip_mode_ = false;
    }
    else if( burst_counter_ >= burst_frame_count_ )
    {
      burst_counter_ = 0;
      burst_skip_mode_ = true;
    }
  }

  // Check minimum scale bounds
  if( max_operate_scale_ > 0.0 && scale_ > max_operate_scale_ )
  {
    fg_ = vil_image_view< bool >( image_.ni(), image_.nj() );
    fg_.fill( false );

    heatmap_ = vil_image_view< float >( image_.ni(), image_.nj() );
    heatmap_.fill( -1.0f );
    return true;
  }

  // Run detector on either the input image or a resized version of it if enabled
  typename cnn_detector< PixType >::detection_vec_t detected_bboxes;

  if( target_scale_ > 0.0 && scale_ > 0.0 )
  {
    vil_image_view< PixType > resized_image;

    double scale_factor = std::min( scale_ / target_scale_, max_scale_factor_ );
    scale_factor = std::max( scale_factor, min_scale_factor_ );

    vil_resample_bilin( image_,
                        resized_image,
                        scale_factor * image_.ni(),
                        scale_factor * image_.nj() );

    typename cnn_detector< PixType >::detection_map_t tmp1;
    typename cnn_detector< PixType >::detection_mask_t tmp2;

    detector_->detect( resized_image, tmp1, tmp2, detected_bboxes );

    for( unsigned i = 0; i < detected_bboxes.size(); ++i )
    {
      detected_bboxes[i].first.scale_about_origin( 1.0 / scale_factor );
    }

    vil_resample_bilin( tmp1, heatmap_, image_.ni(), image_.nj() );
    vil_resample_nearest( tmp2, fg_, image_.ni(), image_.nj() );
  }
  else
  {
    detector_->detect( image_, heatmap_, fg_, detected_bboxes );
  }

  // Handle initial skips if enabled
  processed_counter_++;

  if( processed_counter_ <= initial_skip_count_ )
  {
    return process::SKIP;
  }
  else if( processed_counter_ <= initial_skip_count_ + initial_no_output_count_ )
  {
    return process::NO_OUTPUT;
  }

  // Convert detections from bbox form into vidtk detection format
  vgl_box_2d< unsigned > image_boundary(
    border_pixel_count_, image_.ni() - border_pixel_count_,
    border_pixel_count_, image_.nj() - border_pixel_count_ );

  for( unsigned int i = 0; i < detected_bboxes.size(); ++i )
  {
    image_object_sptr obj = new image_object;

    obj->set_bbox( vgl_intersection( detected_bboxes[i].first, image_boundary ) );
    vgl_box_2d< unsigned > const& bbox = obj->get_bbox();

    obj->set_image_area( bbox.area() );
    obj->set_confidence( detected_bboxes[i].second );

    if( obj->get_image_area() == 0 )
    {
      continue;
    }

    if( mask_ && mask_( bbox.centroid_x(), bbox.centroid_y() ) )
    {
      continue;
    }

    if( fg_ )
    {
      vil_image_view< bool > mask_chip;

      mask_chip.deep_copy( clip_image_for_detection( fg_, obj ) );

      obj->set_object_mask(
        mask_chip,
        image_object::image_point_type(
          obj->get_bbox().min_x(), obj->get_bbox().min_y() ) );
    }

    obj->set_image_loc( bbox.centroid_x(), bbox.centroid_y() );
    obj->set_world_loc( obj->get_image_loc()[0], obj->get_image_loc()[1], 0 );

    detections_.push_back( obj );
  }

  return true;
}


template< class PixType >
void
cnn_detector_process< PixType >
::set_source_image( vil_image_view< PixType > const& image )
{
  image_ = image;
}


template< class PixType >
void
cnn_detector_process< PixType >
::set_source_mask( vil_image_view< bool > const& mask )
{
  mask_ = mask;
}


template< class PixType >
void
cnn_detector_process< PixType >
::set_source_scale( double scale )
{
  scale_ = scale;
}


template< class PixType >
std::vector<image_object_sptr>
cnn_detector_process< PixType >
::detections() const
{
  return detections_;
}


template< class PixType >
vil_image_view< bool >
cnn_detector_process< PixType >
::fg_image() const
{
  return fg_;
}


template< class PixType >
vil_image_view< float >
cnn_detector_process< PixType >
::heatmap_image() const
{
  return heatmap_;
}


} // end namespace vidtk
