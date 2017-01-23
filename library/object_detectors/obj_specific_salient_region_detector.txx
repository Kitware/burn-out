/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "obj_specific_salient_region_detector.h"
#include "obj_specific_salient_region_classifier.h"

#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/vil_math.h>
#include <vil/vil_resample_bilin.h>
#include <vil/vil_resample_nearest.h>

#include <vil/algo/vil_threshold.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_gauss_filter.h>
#include <vil/algo/vil_blob.h>
#include <vil/algo/vil_blob.h>
#include <vgl/vgl_convex.h>
#include <vgl/vgl_area.h>
#include <vil/vil_crop.h>

#include <utilities/point_view_to_region.h>
#include <utilities/polygon_centroid.h>

#include <cmath>
#include <fstream>
#include <sstream>
#include <limits>

#include <descriptors/integral_hog_descriptor.h>

#ifdef USE_VLFEAT
#include <descriptors/dsift_bow_descriptor.h>
#endif

#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#include <utilities/vxl_to_cv_converters.h>
#endif

#include <boost/make_shared.hpp>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER( "obj_specific_salient_region_detector" );

template< typename PixType >
obj_specific_salient_region_detector< PixType >
::obj_specific_salient_region_detector()
{
  configure( ossrd_settings() );
}

template< typename PixType >
obj_specific_salient_region_detector< PixType >
::obj_specific_salient_region_detector( const ossrd_settings& params )
{
  if( !configure( params ) )
  {
    LOG_FATAL( "Unable to configure object specific saliency detector" );
  }
}

template< typename PixType >
obj_specific_salient_region_detector< PixType >
::~obj_specific_salient_region_detector()
{
  kill_all_threads();
}

template< typename PixType >
bool
obj_specific_salient_region_detector< PixType >
::configure( const ossrd_settings& settings )
{
  // Copy settings
  settings_ = settings;

  // Load models
  if( !settings_.initial_eo_model_fn_.empty() &&
      !initial_eo_classifier_.load_from_file( settings_.initial_eo_model_fn_ ) )
  {
    return false;
  }
  if( !settings_.initial_ir_model_fn_.empty() &&
      !initial_ir_classifier_.load_from_file( settings_.initial_ir_model_fn_ ) )
  {
    return false;
  }

  // Create a new model if required
  if( settings.enable_online_learning_ )
  {
    model_data_.reset( new model_data() );
  }

#ifndef USE_VLFEAT
  if( settings.enable_dense_bow_descriptor_ )
  {
    LOG_ERROR( "Dense BoW descriptor requires a build with VLFeat" );
    return false;
  }
#endif

  if( settings.color_space_ != RGB &&
      !is_conversion_supported<PixType>( RGB, settings.color_space_ ) )
  {
    LOG_ERROR( "Unsupported color space received" );
    return false;
  }

  // Reset internal member variables and buffers
  reset();
  use_learned_model_ = false;
  return true;
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::reset( const bool is_eo )
{
  is_eo_flag_ = is_eo;

  if( settings_.use_threads_ && threads_.size() == 0 )
  {
    launch_filter_thread( 0, boost::bind( &self_t::high_pass_filter1, this ) );
    launch_filter_thread( 1, boost::bind( &self_t::high_pass_filter2, this ) );
    launch_filter_thread( 2, boost::bind( &self_t::color_classifier_filter, this ) );
    launch_filter_thread( 3, boost::bind( &self_t::segmentation_filter, this ) );
    launch_filter_thread( 4, boost::bind( &self_t::motion_filter, this ) );
    launch_filter_thread( 5, boost::bind( &self_t::color_conversion_filter, this ) );
    launch_filter_thread( 6, boost::bind( &self_t::distance_filter, this ) );
    launch_filter_thread( 7, boost::bind( &self_t::cut_filter, this ) );
    //launch_filter_thread( 8, boost::bind( &self_t::hog_model_filter, this ) );
    //launch_filter_thread( 9, boost::bind( &self_t::dense_bow_model_filter, this ) );
  }

  // Clear feature image vector
  for( unsigned i = 0; i < max_feature_count; i++ )
  {
    feature_image_[i].clear();
  }
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::launch_filter_thread( unsigned id, filter_func_t func )
{
  LOG_ASSERT( id < max_thread_count, "Thread ID is invalid" );

  thread_status_[ id ] = UNTASKED;

  threads_.push_back(
    boost::make_shared< boost::thread >(
      boost::bind(
        &obj_specific_salient_region_detector< PixType >::filter_thread_job,
        this,
        id,
        func
      )
    )
  );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::task_thread( unsigned id )
{
  LOG_ASSERT( id < max_thread_count, "Thread ID is invalid" );

  thread_status_[ id ] = TASKED;
  thread_cond_var_[ id ].notify_one();
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::wait_for_threads_to_finish( unsigned max_id )
{
  LOG_ASSERT( max_id <= max_thread_count, "Max Thread ID is invalid" );

  for( unsigned id = 0; id < max_id; id++ )
  {
    if( thread_status_[ id ] == TASKED )
    {
      boost::unique_lock<boost::mutex> lock( thread_muti_[ id ] );

      while( thread_status_[ id ] != COMPLETE )
      {
        thread_cond_var_[ id ].timed_wait( lock, boost::posix_time::milliseconds(500) );
      }

      thread_status_[ id ] = UNTASKED;
    }
  }
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::kill_all_threads()
{
  for( unsigned i = 0; i < max_thread_count; i++ )
  {
    thread_status_[i] = KILLED;
    thread_cond_var_[i].notify_one();
  }
  for( unsigned i = 0; i < threads_.size(); i++ )
  {
    threads_[i]->join();
  }
  threads_.clear();
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::filter_thread_job( unsigned id, filter_func_t func )
{
  boost::mutex *thread_mutex = &thread_muti_[id];
  boost::condition_variable *thread_cond_var = &thread_cond_var_[id];
  thread_status *current_status = &thread_status_[id];

  while( *current_status != KILLED )
  {
    {
      boost::unique_lock<boost::mutex> lock( *thread_mutex );

      while( *current_status == UNTASKED )
      {
        thread_cond_var->wait( lock );
      }
    }

    if( *current_status == KILLED )
    {
      break;
    }
    else if( *current_status == TASKED )
    {
      func();

      *current_status = COMPLETE;
      thread_cond_var->notify_one();
    }
  }
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::apply_model( const input_image_t& image,
               const image_region_t& region,
               const mask_image_t& motion_mask,
               const motion_image_t& motion_image,
               const double gsd,
               weight_image_t& output )
{
  input_color_image_ = image;
  input_region_ = region;
  input_motion_image_ = motion_image;
  input_motion_mask_ = motion_mask;
  input_gsd_ = gsd;

  format_region_images();

  for( unsigned id = 0; id < 5; id++ )
  {
    task_thread( id );
  }

  wait_for_threads_to_finish( 5 );

  apply_classifier( 11, output );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::apply_model( const input_image_t& image,
               const image_region_t& region,
               const mask_image_t& motion_mask,
               const motion_image_t& motion_image,
               const double gsd,
               const image_point_t& center,
               weight_image_t& output )
{
  LOG_INFO( "Extracting features for target: " << center );

  input_color_image_ = image;
  input_region_ = region;
  input_motion_image_ = motion_image;
  input_motion_mask_ = motion_mask;
  input_gsd_ = gsd;
  input_location_ = center;

  if( !format_region_images( false ) )
  {
    LOG_DEBUG( "Could not format input images" );
    LOG_DEBUG( "  Region: " << region );
    LOG_DEBUG( "  Image Size: " << image.ni() << " " << image.nj() );
    LOG_DEBUG( "  GSD: " << gsd );
    output.clear();
    return;
  }

  for( unsigned id = 0; id < utilized_threads; id++ )
  {
    task_thread( id );
  }

  wait_for_threads_to_finish( utilized_threads );

  weight_image_t scaled_classifier;

  apply_classifier( utilized_features, scaled_classifier );

  image_region_t intersected_bbox = input_region_;
  check_region_boundaries( intersected_bbox, input_color_image_ );

  vil_resample_bilin( scaled_classifier, output, intersected_bbox.width(), intersected_bbox.height() );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::extract_features( const input_image_t& image,
                    const image_region_t& region,
                    const mask_image_t& motion_mask,
                    const motion_image_t& motion_image,
                    const double gsd,
                    const image_point_t& center,
                    const label_image_t& labels,
                    const std::string& output_fn,
                    const std::string& feature_prefix )
{
  LOG_INFO( "Extracting features for target: " << center );

  input_color_image_ = image;
  input_region_ = region;
  input_motion_image_ = motion_image;
  input_motion_mask_ = motion_mask;
  input_gsd_ = gsd;
  input_location_ = center;
  input_labels_ = labels;

  if( !format_region_images( true ) )
  {
    LOG_DEBUG( "Could not format input images" );
    LOG_DEBUG( "  Region: " << region );
    LOG_DEBUG( "  Image Size: " << image.ni() << " " << image.nj() );
    LOG_DEBUG( "  GSD: " << gsd );
    return;
  }

  for( unsigned id = 0; id < utilized_threads; id++ )
  {
    task_thread( id );
  }

  wait_for_threads_to_finish( utilized_threads );

  if( feature_prefix.size() > 0 )
  {
    LOG_DEBUG( "Printing out feature images" );
    static unsigned qccount = 0;
    qccount++;
    std::string prefixz = "q" + boost::lexical_cast<std::string>(qccount) + "_feature_";

    feature_image_[utilized_features] = normalized_labels_;

    std::vector< feature_image_t > feature_vector;

    for( unsigned i = 0; i < utilized_features + 1; i++ )
    {
      feature_vector.push_back( feature_image_[i] );
    }

    if( initial_eo_classifier_.is_valid() )
    {
      vil_image_view<double> pixel_weights;
      vil_image_view<bool> test_mask;
      vil_image_view<vxl_byte> test_feat1, test_feat2;

      apply_classifier( utilized_features, pixel_weights );

      vil_threshold_above( pixel_weights, test_mask, 0.0 );
      vil_convert_cast( test_mask, test_feat1 );
      feature_vector.push_back( test_feat1 );
      vil_convert_stretch_range( pixel_weights, test_feat2 );
      feature_vector.push_back( test_feat2 );
    }

    write_feature_images( feature_vector, feature_prefix + "/" + prefixz );
  }

  std::ofstream out( output_fn.c_str(), std::ios::app );

  // Write features and groundtruth to file
  const int ni = static_cast<int>(feature_image_[0].ni());
  const int nj = static_cast<int>(feature_image_[0].nj());
  const int border = 1;

  for( int j = border; j < nj - border; j++ )
  {
    for( int i = border; i < ni - border; i++ )
    {
      out << static_cast<int>(normalized_labels_(i,j)) << " ";

      for( unsigned k = 0; k < utilized_features; k++ )
      {
        out << static_cast<int>(feature_image_[k](i,j)) << " ";
      }
      out << std::endl;
    }
  }

  out.close();
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::update_model( const input_image_t& /*image*/,
                const image_region_t& /*region*/,
                const mask_image_t& /*region_mask*/,
                const mask_image_t& /*motion_mask*/,
                const motion_image_t& /*motion_image*/,
                const double /*gsd*/ )
{
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::apply_classifier( const unsigned feature_count, weight_image_t& output )
{
  if( is_eo_flag_ )
  {
    initial_eo_classifier_.classify_images( feature_image_,
                                            feature_count,
                                            output );
  }
  else
  {
    initial_ir_classifier_.classify_images( feature_image_,
                                            feature_count,
                                            output );
  }
}

template< typename PixType >
bool
obj_specific_salient_region_detector< PixType >
::format_region_images( bool format_label_image )
{
  const double resize_flux_factor = 0.10;

  const double input_ni = static_cast<double>( input_region_.width() );
  const double input_nj = static_cast<double>( input_region_.height() );
  const double input_area = input_ni * input_nj;

  double scale_factor = input_gsd_ / settings_.desired_filter_gsd_;

  unsigned scaled_ni = static_cast<unsigned>( input_ni * scale_factor );
  unsigned scaled_nj = static_cast<unsigned>( input_nj * scale_factor );
  unsigned scaled_area = scaled_ni * scaled_nj;

  image_region_t checked_region = input_region_;
  check_region_boundaries( checked_region, input_color_image_ );
  input_image_t cropped_region = point_view_to_region( input_color_image_, input_region_ );

  if( cropped_region.ni() == 0 || cropped_region.nj() == 0 )
  {
    return false;
  }

  feature_image_t original;

  if( vil_pixel_format_of(PixType()) == vil_pixel_format_of(feature_t()) )
  {
    original = cropped_region;
  }
  else
  {
    vil_convert_stretch_range_limited<PixType>( cropped_region,
                                                original,
                                                0, std::numeric_limits<PixType>::max(),
                                                0, std::numeric_limits<feature_t>::max() );
  }

  if( scale_factor < 1.0 - resize_flux_factor )
  {
    vil_resample_bilin( original, normalized_region_color_, scaled_ni, scaled_nj );
    normalized_gsd_ = input_gsd_ / scale_factor;
  }
  else if( scale_factor > 1.0 + resize_flux_factor &&
           !( input_area > settings_.max_filter_area_pixels_ ) )
  {
    if( scaled_area > settings_.max_filter_area_pixels_ )
    {
      scale_factor = std::sqrt( settings_.max_filter_area_pixels_ / input_area );
      scaled_ni = static_cast<unsigned>( input_ni * scale_factor );
      scaled_nj = static_cast<unsigned>( input_nj * scale_factor );
    }
    vil_resample_bilin( original, normalized_region_color_, scaled_ni, scaled_nj );
    normalized_gsd_ = input_gsd_ / scale_factor;
  }
  else
  {
    scale_factor = 1.0;
    normalized_region_color_ = original;
    normalized_gsd_ = input_gsd_;
  }

  if( is_eo_flag_ )
  {
    vil_convert_planes_to_grey( normalized_region_color_,
                                normalized_region_gray_ );
  }
  else
  {
    vil_math_mean_over_planes( normalized_region_color_,
                               normalized_region_gray_ );
  }

  // point conversion
  normalized_location_ = image_point_t( ( input_location_.x() - checked_region.min_x() ) * scale_factor,
                                        ( input_location_.y() - checked_region.min_y() ) * scale_factor );

  if( format_label_image )
  {
    feature_image_t casted_cropped;
    label_image_t cropped_labels = point_view_to_region( input_labels_, input_region_ );
    vil_convert_cast( cropped_labels, casted_cropped );
    vil_resample_nearest( casted_cropped, normalized_labels_, normalized_region_gray_.ni(), normalized_region_gray_.nj() );
  }

  return true;
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::high_pass_filter1()
{
  unsigned radius = ( 7 / normalized_gsd_ );
  radius = radius | 0x01;
  if( radius < 5 ) radius = 5;
  if( radius > 17 ) radius = 17;

  box_high_pass_filter( normalized_region_gray_,
                        feature_image_[1],
                        radius,
                        radius,
                        false );

  feature_image_[1] = vil_plane( feature_image_[1], 2 );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::high_pass_filter2()
{
  unsigned radius = ( 14 / normalized_gsd_ );
  radius = radius | 0x01;
  if( radius < 11 ) radius = 11;

  box_high_pass_filter( normalized_region_gray_,
                        feature_image_[2],
                        radius,
                        radius,
                        false );

  feature_image_[2] = vil_plane( feature_image_[2], 2 );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::color_conversion_filter()
{
  feature_image_[0] = normalized_region_gray_;

  convert_color_space( normalized_region_color_,
                       feature_image_[8],
                       RGB,
                       settings_.color_space_ );

  feature_image_[10] = vil_plane( feature_image_[8], 2 );
  feature_image_[9] = vil_plane( feature_image_[8], 1 );
  feature_image_[8] = vil_plane( feature_image_[8], 0 );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::distance_filter()
{
  feature_image_t& fi = feature_image_[7];

  fi.set_size( normalized_region_gray_.ni(), normalized_region_gray_.nj() );

  const unsigned ci = normalized_location_.x();
  const unsigned cj = normalized_location_.y();

  for( unsigned j = 0; j < fi.nj(); j++ )
  {
    for( unsigned i = 0; i < fi.ni(); i++ )
    {
      unsigned dist = ( ci > i ? ci - i : i - ci );
      dist += ( cj > j ? cj - j : j - cj );
      dist = static_cast<unsigned>( 1.75 * normalized_gsd_ * dist );
      if( dist > 255 )
        dist = 255;
      fi(i,j) = dist;
    }
  }
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::color_classifier_filter()
{
  color_commonality_filter( normalized_region_color_,
                            feature_image_[3],
                            cc_settings_ );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::segmentation_filter()
{
  segment_image_kmeans( normalized_region_gray_,
                        feature_image_[6],
                        4, 1000 );

  feature_image_t& feat = feature_image_[6];

  feat.set_size( normalized_region_gray_.ni(), normalized_region_gray_.nj() );
  unsigned hist[4] = {0};

  for( unsigned j = 0; j < feat.nj(); j++ )
    for( unsigned i = 0; i < feat.ni(); i++ )
      hist[ feat(i,j) ]++;

  const unsigned denom = feat.nj() * feat.ni();

  for( unsigned i = 0; i < 4; i++ )
  {
    hist[i] = ( 200u * static_cast<unsigned>(hist[i]) ) / denom;
  }

  // Feature 1, commonality
  for( unsigned j = 0; j < feat.nj(); j++ )
    for( unsigned i = 0; i < feat.ni(); i++ )
      feat(i,j) = hist[feat(i,j)];
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::cut_filter()
{
  feature_image_t& feat2 = feature_image_[11];
  feat2.set_size( normalized_region_color_.ni(), normalized_region_color_.nj() );

  if( feat2.ni() < 10 || feat2.nj() < 10 )
  {
    feat2.fill( 0 );
    return;
  }

#ifdef USE_OPENCV

  cv::Mat ocv_bgr_image;
  deep_cv_conversion( normalized_region_color_, ocv_bgr_image );

  cv::Mat ocv_labels( ocv_bgr_image.size(), CV_8UC1 );
  ocv_labels = cv::Scalar(0);

  int ci = static_cast<int>( normalized_location_.x() );
  int cj = static_cast<int>( normalized_location_.y() );

  if( ci > ocv_labels.cols - 2 )
    ci = ocv_labels.cols - 2;
  if( cj > ocv_labels.rows - 2 )
    cj = ocv_labels.rows - 2;
  if( ci < 1 )
    ci = 1;
  if( cj < 1 )
    cj = 1;

  int traverse_dist = static_cast< int >( 5.4 / normalized_gsd_ );

  int top_i = std::min( ci + traverse_dist, ocv_labels.cols-1 );
  int bot_i = std::max( ci - traverse_dist, 0 );
  int top_j = std::min( cj + traverse_dist, ocv_labels.rows-1 );
  int bot_j = std::max( cj - traverse_dist, 0 );

  cv::Rect r1( cv::Point( bot_j, bot_i ), cv::Point( top_j, top_i ) );

  cv::Mat tmp1, tmp2;
  cv::grabCut( ocv_bgr_image, ocv_labels, r1, tmp1, tmp2, 10, cv::GC_INIT_WITH_RECT );

  if( ocv_labels.rows == static_cast<int>(feat2.nj()) &&
      ocv_labels.cols == static_cast<int>(feat2.ni()) )
  {
    for( int j = 0; j < ocv_labels.rows; j++ )
    {
      for( int i = 0; i < ocv_labels.cols; i++ )
      {
        feat2( i, j ) = ocv_labels.at< unsigned char >( j, i );
      }
    }
  }
  else
  {
    feat2.fill( 0 );
  }

#else

  feat2.fill( 0 );

#endif
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::motion_filter()
{
  // TODO ACCOUNT FOR DIFF_SP CROP!
  unsigned crop_i = ( input_color_image_.ni() - input_motion_mask_.ni() ) / 2;
  unsigned crop_j = ( input_color_image_.nj() - input_motion_mask_.nj() ) / 2;

  image_region_t decropped_region = image_region_t( ( input_region_.min_x() > crop_i ? input_region_.min_x() - crop_i : 0 ),
                                                    ( input_region_.max_x() > crop_i ? input_region_.max_x() - crop_i : 0 ),
                                                    ( input_region_.min_y() > crop_j ? input_region_.min_y() - crop_j : 0 ),
                                                    ( input_region_.max_y() > crop_j ? input_region_.max_y() - crop_j : 0 ) );

  motion_image_t motion_region_image =
    point_view_to_region( input_motion_image_, decropped_region );
  mask_image_t mask_region_image =
    point_view_to_region( input_motion_mask_, decropped_region );

  motion_image_t scaled_motion_image;
  feature_image_t converted_mask_image;

  vil_convert_cast( mask_region_image, converted_mask_image );

  vil_resample_bilin( motion_region_image,
                      scaled_motion_image,
                      normalized_region_gray_.ni(),
                      normalized_region_gray_.nj() );

  vil_resample_bilin( converted_mask_image,
                      feature_image_[4],
                      normalized_region_gray_.ni(),
                      normalized_region_gray_.nj() );

  scale_float_img_to_interval( scaled_motion_image,
                               feature_image_[5],
                               1.0f,
                               255.0f );
}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::hog_model_filter()
{

}

template< typename PixType >
void
obj_specific_salient_region_detector< PixType >
::dense_bow_model_filter()
{

}

template< typename PixType >
class
obj_specific_salient_region_detector< PixType >::model_data
{

public:

  model_data() : update_counter_(0) {}
  ~model_data() {}

  unsigned update_counter_;

  histogram_t obj_hist_;

  descriptor_t hog_model_;
  integral_hog_descriptor< PixType > hog_descriptor_;

  moving_training_data_container< PixType > positive_examples_;
  moving_training_data_container< PixType > negative_examples_;

#ifdef USE_VLFEAT
  descriptor_t dense_bow_model_;
  dsift_bow_descriptor dense_descriptor_;
#endif

};

} // end namespace vidtk
