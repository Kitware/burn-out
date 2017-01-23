/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ocv_hog_detector.h"

#include <utilities/vxl_to_cv_converters.h>

#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <vgl/vgl_intersection.h>

#include <logger/logger.h>

VIDTK_LOGGER( "ocv_hog_detector" );


namespace vidtk
{

ocv_hog_detector
::ocv_hog_detector()
 : dpm_detector_( NULL ),
   dt_detector_( NULL )
{
}

ocv_hog_detector
::~ocv_hog_detector()
{
}


//----------------------------------------------------------------
bool
ocv_hog_detector
::configure( const ocv_hog_detector_settings& settings )
{
  this->settings_ = settings;

  if( settings.location_type == "centroid" )
  {
    loc_type_ = centroid;
  }
  else if( settings.location_type == "bottom" )
  {
    loc_type_ = bottom;
  }

  if( settings.ignore_regions.empty() )
  {
    ignore_regions_.clear();
  }
  else if( settings.ignore_regions.size() % 4 != 0 )
  {
    LOG_ERROR( "Invalid ignore regions field" );
    return false;
  }
  else
  {
    for( unsigned i = 0; i < settings.ignore_regions.size(); i+=4 )
    {
      ignore_regions_.push_back(
        region_t( settings.ignore_regions[i],
                                settings.ignore_regions[i+1],
                                settings.ignore_regions[i+2],
                                settings.ignore_regions[i+3] ) );
    }
  }

  if( settings.use_threads )
  {
    threads_.reset( new threaded_detector< image_t >( 3, 3 ) );

    threads_->set_function(
      boost::bind( &ocv_hog_detector::compute_detections, this, _1, _2 ) );
  }

  return this->load_model( settings.model_filename );
}


//----------------------------------------------------------------
void
ocv_hog_detector
::reset_models()
{
  dpm_detector_.reset();
  dt_detector_.reset();
}


//----------------------------------------------------------------
bool
ocv_hog_detector
::load_model( const std::string& fname )
{
  return this->load_models( std::vector<std::string>( 1, fname ) );
}


//----------------------------------------------------------------
bool
ocv_hog_detector
::load_models( const std::vector< std::string >& fnames )
{
  if( fnames.empty() )
  {
    LOG_ERROR( "No filenames specified!" );
    return false;
  }

  if( settings_.use_dpm_detector )
  {
    if( !dpm_detector_ )
    {
      dpm_detector_.reset( new cv::LatentSvmDetector() );
    }

    if( fnames.empty() )
    {
      LOG_ERROR( "No input DPM model files provided" );
      return false;
    }
    else if( !dpm_detector_->load( fnames ) )
    {
      LOG_ERROR( "Failed to open DPM model file(s)" );
      return false;
    }
  }
  else
  {
    if ( !dt_detector_ )
    {
      dt_detector_.reset(
        new cv::HOGDescriptor(
          cv::Size( ( settings_.low_res_model ? 48 : 64 ),
                    ( settings_.low_res_model ? 96 : 128 ) ),
          cv::Size( 16, 16 ),
          cv::Size( 8, 8 ),
          cv::Size( 8, 8 ),
          9, 1, -1,
          cv::HOGDescriptor::L2Hys,
          0.2, false,
          settings_.levels ) );
    }

    if( fnames.empty() || ( fnames.size() == 1 && fnames[0].empty() ) )
    {
      LOG_INFO( "Using default person model for HOG detector" );

      if( !settings_.low_res_model )
      {
        dt_detector_->setSVMDetector( cv::HOGDescriptor::getDefaultPeopleDetector() );
      }
      else
      {
        dt_detector_->setSVMDetector( cv::HOGDescriptor::getDaimlerPeopleDetector() );
      }
    }
    else if( fnames.size() == 1 )
    {
      if( !dt_detector_->load( fnames[0] ) )
      {
        LOG_ERROR( "Failed to open model file " << fnames[0] );
        return false;
      }
    }
    else
    {
      LOG_ERROR( "Standard HOG does not support multiple model files at this time" );
      return false;
    }
  }

  return true;
}


//----------------------------------------------------------------
void
ocv_hog_detector
::compute_detections( const image_border& region,
                      const image_t& img )
{
  cv::Mat cv_img;
  cv::Mat cv_img_scaled;

  deep_cv_conversion( img, cv_img );

  cv::resize( cv_img, cv_img_scaled,
              cv::Size( img.ni() * scale_factor_, img.nj() * scale_factor_ ) );

  // Run specified detection algorithm on input image
  if( settings_.use_dpm_detector )
  {
    if( !dpm_detector_ || dpm_detector_->empty() )
    {
      LOG_FATAL( "No internal HOG DPM model is loaded!" );
    }

    std::vector< cv::LatentSvmDetector::ObjectDetection > cv_results;

    // Run detection algorithm
    dpm_detector_->detect( cv_img_scaled,
                           cv_results,
                           settings_.overlap_threshold,
                           settings_.num_threads );

    // Convert detections from cv format into vxl bbox format
    boost::unique_lock< boost::mutex > lock( attach_mutex_ );

    for( unsigned int i = 0; i < cv_results.size(); ++i )
    {
      if( cv_results[i].score >= settings_.detector_threshold )
      {
        detection_bboxes_.push_back(
          region_t( cv_results[i].rect.x / scale_factor_ + region.min_x(),
                    ( cv_results[i].rect.x + cv_results[i].rect.width ) / scale_factor_ + region.min_x(),
                    cv_results[i].rect.y / scale_factor_ + region.min_y(),
                    ( cv_results[i].rect.y + cv_results[i].rect.height ) / scale_factor_ + region.min_y() ) );

        confidence_values_.push_back( cv_results[i].score );
      }
    }
  }
  else
  {
    if( !dt_detector_ )
    {
      LOG_FATAL( "No internal HOG model is loaded!" );
    }

    std::vector< cv::Rect > cv_results;

    // Run detector
    dt_detector_->detectMultiScale( cv_img_scaled,
                                    cv_results,
                                    settings_.detector_threshold,
                                    cv::Size( 8, 8 ),
                                    cv::Size( 32, 32 ),
                                    settings_.level_scale,
                                    2 );

    // Convert detections from cv format into vxl bbox format
    boost::unique_lock< boost::mutex > lock( attach_mutex_ );

    for( unsigned int i = 0; i < cv_results.size(); ++i )
    {
      detection_bboxes_.push_back(
        region_t( cv_results[i].x / scale_factor_ + region.min_x(),
                  ( cv_results[i].x + cv_results[i].width ) / scale_factor_ + region.min_x(),
                  cv_results[i].y / scale_factor_ + region.min_y(),
                  ( cv_results[i].y + cv_results[i].height ) / scale_factor_ + region.min_y() ) );
    }

    // Optionally output any detection images to folder
    if( !settings_.output_detection_folder.empty() )
    {
      static unsigned detection_counter = 1;

      std::string fn = settings_.output_detection_folder + "/detection" +
        boost::lexical_cast< std::string >( detection_counter ) + ".png";

      detection_counter++;

      cv::Mat copied_image;

      if( cv_img_scaled.channels() == 3 )
      {
        cv_img_scaled.copyTo( copied_image );
      }
      else
      {
        cv::cvtColor( cv_img_scaled, copied_image, CV_GRAY2BGR );
      }

      for( unsigned i = 0; i < cv_results.size(); ++i )
      {
        cv::rectangle( copied_image, cv_results[i], cv::Scalar( 0, 0, 255 ), 2 );
      }

      cv::imwrite( fn, copied_image );
    }
  }

}

//----------------------------------------------------------------
std::vector< image_object_sptr >
ocv_hog_detector
::detect( const vil_image_view<vxl_byte>& img,
          const double scale_factor )
{
  std::vector< image_object_sptr > results;

  detection_bboxes_.clear();
  confidence_values_.clear();
  scale_factor_ = scale_factor;

  // Optionally apply threading
  if( !settings_.use_threads )
  {
    compute_detections( image_border( 0, img.ni(), 0, img.nj() ), img );
  }
  else
  {
    threads_->apply_function( img );
  }

  // Generate image bounding box
  region_t image_boundary( settings_.border_pixel_count,
                           img.ni() - settings_.border_pixel_count,
                           settings_.border_pixel_count,
                           img.nj() - settings_.border_pixel_count );

  // Convert detections from bbox form into vidtk detection format
  for( unsigned int i = 0; i < detection_bboxes_.size(); ++i )
  {
    image_object_sptr obj = new image_object;

    obj->set_bbox( vgl_intersection( detection_bboxes_[i], image_boundary ) );
    region_t const& bbox = obj->get_bbox();

    if( !confidence_values_.empty() )
    {
      obj->set_confidence( confidence_values_[i] );
    }

    obj->set_image_area( bbox.area() );

    if( obj->get_image_area() == 0 )
    {
      continue;
    }

    if( !ignore_regions_.empty() )
    {
      for( unsigned j = 0; j < ignore_regions_.size(); ++j )
      {
        if( vgl_intersection( ignore_regions_[j], bbox ).area() > 0 )
        {
          continue;
        }
      }
    }

    if( loc_type_ == centroid )
    {
      obj->set_image_loc( bbox.centroid_x(), bbox.centroid_y() );
    }
    else if( loc_type_ == bottom )
    {
      float_type x = ( bbox.min_x() + bbox.max_x() ) / float_type( 2 );
      float_type y = float_type( bbox.max_y() );
      obj->set_image_loc( x, y );
    }

    obj->set_world_loc( obj->get_image_loc()[0], obj->get_image_loc()[1], 0 );

    results.push_back( obj );
  }

  return results;
}

} // end namespace
