/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_reinit_detector.h"

#include <vil/vil_resample_bilin.h>
#include <vil/vil_convert.h>

#include <utilities/training_thread.h>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#ifdef USE_VLFEAT
#include <descriptors/dsift_bow_descriptor.h>
#include <descriptors/mfeat_bow_descriptor.h>
#endif

#include <descriptors/integral_hog_descriptor.h>

#include <learning/adaboost.h>
#include <learning/stump_weak_learners.h>
#include <learning/learner_data_class_simple_wrapper.h>

#ifdef USE_OPENCV
#include <opencv/ml.h>
#endif

#ifdef TIMING_TESTS
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

#include <utilities/training_thread.h>

#include <logger/logger.h>

VIDTK_LOGGER("track_reinit_detector");

namespace vidtk
{


// Internal typedefs
typedef adaboost default_model_t;
typedef boost::shared_ptr< default_model_t > default_model_sptr_t;

#ifdef USE_OPENCV
typedef CvBoost ocv_model_t;
typedef boost::shared_ptr< ocv_model_t > ocv_model_sptr_t;
typedef CvBoostParams ocv_params_t;
#endif


// Internal model constants
const unsigned color_feat_count = 99;
const unsigned area_feat_count = 1;
const unsigned linking_desc_size = 41;
const unsigned min_training_examples = 11;
const unsigned min_chip_side_length = 4;


// Prior state information
struct prior_state_info
{
  bool is_set_;
  std::vector< double > last_descriptor_;
  vnl_vector_fixed< double, 2 > last_img_loc_;
  vnl_vector_fixed< double, 2 > last_world_loc_;
  vnl_vector_fixed< double, 3 > last_world_vel_;
  double last_time_;
  unsigned last_mover_category_;
  vgl_box_2d< unsigned > last_bbox_;
  double last_scale_;
  unsigned last_track_length_;

  prior_state_info()
  {
    is_set_ = false;
    last_img_loc_ = vnl_vector_fixed<double,2>(0,0);
    last_world_loc_ = vnl_vector_fixed<double,2>(0,0);
    last_world_vel_ = vnl_vector_fixed<double,3>(0,0,0);
    last_time_ = 0;
    last_mover_category_ = 0;
    last_descriptor_.clear();
    last_scale_ = 0;
    last_track_length_ = 0;
  }

};


// The internal object model and related computational resources
class track_reinit_detector::object_model
{
public:

  typedef training_thread thread_t;
  typedef boost::shared_ptr< thread_t > thread_sptr_t;

  object_model();
  ~object_model();

  // Configure internal descriptors
  bool configure( const track_reinit_settings& settings );

  // Update internal learned model
  void update( const training_data_t& positive_samples,
               const training_data_t& negative_samples );

  // Reset internal model, force thread to be available
  void reset();

  // Feature count and pre-computed steps given our settings
  unsigned feature_count;
  unsigned area_offset;
  unsigned hog_offset;
  unsigned dense_bow_offset;
  unsigned ip_bow_offset;

  // Classify features according to the correct classifier
  double classify_features( const std::vector< double >& features );

  // Status of the internal training thread and related variables
  thread_sptr_t thread_;

  // A stored copy of the externally set options
  track_reinit_settings settings_;

  // Descriptor classes
#ifdef USE_VLFEAT
  dsift_bow_descriptor dense_desc_;
  mfeat_bow_descriptor ip_desc_;
#endif
  custom_ii_hog_model ii_model_;

  // Model classes
#ifdef USE_OPENCV
  ocv_model_sptr_t classifier_ocv_;
  ocv_params_t classifier_ocv_params_;
#endif
  default_model_sptr_t classifier_;

  // Was any classifier successfully trained since last reset?
  bool classifier_trained_;

  // Last training sample count
  unsigned pos_count_;
  unsigned neg_count_;

  // Previous state information
  prior_state_info last_state_info_1_;
  prior_state_info last_state_info_2_;

  // Reset above variables
  void reset_state_vars();

  // Locked when classifiers are being updated
  boost::shared_mutex update_mutex_;

  friend class model_update_threaded_task;
};


// Thread task object for parallelization, updates above model
class track_reinit_detector::model_update_threaded_task
: public training_thread_task
{
public:

  explicit model_update_threaded_task(
    object_model* model,
    const training_data_t& pos,
    const training_data_t& neg )
  {
    model_ = model;
    pos_ = pos;
    neg_ = neg;
  }

  ~model_update_threaded_task()
  {
  }

  // Thread execute function
  bool execute();

  // Internal variables
  object_model* model_;
  training_data_t pos_;
  training_data_t neg_;
};


track_reinit_detector::object_model
::object_model()
{
  // Reset internals
  feature_count = 0;

  reset_state_vars();

  // Create default iihog model
  ii_model_.blocks_.clear();
  ii_model_.blocks_.resize( 2 );

  for( unsigned i = 0; i < 16; i++ )
  {
    unsigned x = i / 4;
    unsigned y = i % 4;

    ii_model_.blocks_[0].push_back(i);

    ii_model_.cells_.push_back(
      vgl_box_2d< double >(
        x * 0.25, ( x + 1 ) * 0.25,
        y * 0.25, ( y + 1 ) * 0.25
      )
    );
  }

  ii_model_.blocks_[1].resize( 4 );
  ii_model_.blocks_[1][0] = 5;
  ii_model_.blocks_[1][0] = 6;
  ii_model_.blocks_[1][0] = 9;
  ii_model_.blocks_[1][0] = 10;

  classifier_trained_ = false;

  pos_count_ = 0;
  neg_count_ = 0;

  thread_.reset( new thread_t() );
}


bool
track_reinit_detector::object_model
::configure( const track_reinit_settings& settings )
{
  settings_ = settings;

#ifdef USE_VLFEAT
  if( settings_.use_dense_bow_descriptor &&
      !dense_desc_.configure( settings.dense_bow_vocab ) )
  {
    LOG_ERROR( "Unable to load dense bow vocabulary!" );
    return false;
  }
  if( settings_.use_ip_bow_descriptor &&
      !ip_desc_.configure( settings.dense_bow_vocab ) )
  {
    LOG_ERROR( "Unable to load ip bow vocabulary!" );
    return false;
  }
#else
  if( settings_.use_dense_bow_descriptor || settings_.use_ip_bow_descriptor )
  {
    LOG_ERROR( "Bag of words descriptors requires build with VLFeat" );
    return false;
  }
#endif

#ifdef USE_OPENCV
  if( settings_.model_type == track_reinit_settings::ADABOOST_OCV )
  {
    int boosting_weight_schema = CvBoost::REAL;

    if( settings_.boost_type == track_reinit_settings::GENTLE )
    {
      boosting_weight_schema = CvBoost::GENTLE;
    }

    classifier_ocv_params_ = ocv_params_t( boosting_weight_schema,
                                           settings_.max_iterations,
                                           0.97,
                                           settings_.max_tree_depth,
                                           false,
                                           0 );
  }
#else
  if( settings_.model_type == track_reinit_settings::ADABOOST_OCV )
  {
    LOG_ERROR( "Build with opencv required for specified model type boost_ocv" );
    return false;
  }
#endif

  area_offset = ( settings.use_color_descriptor ? color_feat_count : 0 );
  hog_offset = area_offset + ( settings.use_area_descriptor ? area_feat_count : 0 );
  dense_bow_offset = hog_offset + ( settings.use_hog_descriptor ?  ii_model_.feature_count() : 0 );

#ifdef USE_VLFEAT
  ip_bow_offset = dense_bow_offset + ( settings.use_dense_bow_descriptor ? dense_desc_.vocabulary().word_count() : 0 );
  feature_count = ip_bow_offset + ( settings.use_ip_bow_descriptor ? ip_desc_.vocabulary().word_count() : 0 );
#else
  ip_bow_offset = dense_bow_offset;
  feature_count = ip_bow_offset;
#endif

  return true;
}

track_reinit_detector::object_model
::~object_model()
{
  thread_.reset();
}

track_reinit_detector
::track_reinit_detector( const track_reinit_settings& options )
  : internal_model_( new object_model() )
{
  if( !internal_model_->configure( options ) )
  {
    LOG_FATAL( "Unable to configure object model!" );
  }
}

track_reinit_detector
::~track_reinit_detector()
{
}

template< typename PixType >
bool
compute_image_chip( const vil_image_view<PixType>& img,
                    const image_region& region,
                    vil_image_view<float>& output,
                    double bb_scale_factor = 1.35,
                    unsigned bb_pixel_shift = 0,
                    double desired_min_area = 5500.0,
                    double desired_max_area = 7000.0 )
{
  // Validate input area
  if( region.area() == 0 )
  {
    return false;
  }

  // Expand bbox size, union with image bounds
  image_region new_region = region;
  scale_region( new_region, bb_scale_factor, bb_pixel_shift );

  // Create new single channel image
  vil_image_view<PixType> rgb_region = point_view_to_region( img, new_region );

  // Validate extracted region size
  if( rgb_region.ni() < min_chip_side_length || rgb_region.nj() < min_chip_side_length )
  {
    return false;
  }

  if( rgb_region.nplanes() == 3 )
  {
    vil_convert_planes_to_grey( rgb_region, output, 0.3333, 0.3333, 0.3333 );
  }
  else
  {
    vil_convert_cast( rgb_region, output );
  }

  // Validate input region
  vil_image_view<float> resized;

  int region_area = output.size();

  if( region_area == 0 )
  {
    return false;
  }

  double resize_factor = 1.0;

  if( region_area >= desired_min_area && region_area <= desired_max_area )
  {
    return true;
  }
  else if( region_area < desired_min_area )
  {
    resize_factor = std::sqrt( desired_min_area / region_area );
  }
  else
  {
    resize_factor = std::sqrt( desired_max_area / region_area );
  }

  // Calculate and validate resizing parameters
  unsigned new_width = static_cast<unsigned>(resize_factor * output.ni());
  unsigned new_height = static_cast<unsigned>(resize_factor * output.nj());

  if( new_width < min_chip_side_length || new_height < min_chip_side_length )
  {
    return false;
  }

  // Resample
  vil_image_view<float> resized_image;
  vil_resample_bilin( output, resized_image, new_width, new_height );
  output = resized_image;

  return true;
}

/*template< typename PixType >
bool
compute_image_chip_2( const cv::Mat& img,
                      const image_region& region,
                      vil_image_view<float>& output,
                      const double chip_angle = 0,
                      double bb_scale_factor = 1.35,
                      unsigned bb_pixel_shift = 0,
                      double desired_min_area = 5500.0,
                      double desired_max_area = 7000.0 )
{
  // Validate input area
  if( region.area() == 0 )
  {
    return false;
  }

  // Expand bbox size, union with image bounds
  image_region new_region = region;
  scale_region( new_region, bb_scale_factor, bb_pixel_shift );

  // Create new single channel image
  vil_image_view<PixType> rgb_region; // = point_view_to_region( img, new_region );

  // Validate extracted region size
  if( rgb_region.ni() < min_chip_side_length || rgb_region.nj() < min_chip_side_length )
  {
    return false;
  }

  if( rgb_region.nplanes() == 3 )
  {
    vil_convert_planes_to_grey( rgb_region, output, 0.3333, 0.3333, 0.3333 );
  }
  else
  {
    vil_convert_cast( rgb_region, output );
  }

  // Validate input region
  vil_image_view<float> resized;

  int region_area = output.size();

  if( region_area == 0 )
  {
    return false;
  }

  double resize_factor = 1.0;

  if( region_area >= desired_min_area && region_area <= desired_max_area )
  {
    return true;
  }
  else if( region_area < desired_min_area )
  {
    resize_factor = std::sqrt( desired_min_area / region_area );
  }
  else
  {
    resize_factor = std::sqrt( desired_max_area / region_area );
  }

  // Calculate and validate resizing parameters
  unsigned new_width = static_cast<unsigned>(resize_factor * output.ni());
  unsigned new_height = static_cast<unsigned>(resize_factor * output.nj());

  if( new_width < min_chip_side_length || new_height < min_chip_side_length )
  {
    return false;
  }

  // Resample
  vil_image_view<float> resized_image;
  vil_resample_bilin( output, resized_image, new_width, new_height );
  output = resized_image;

  return true;
}*/

void simple_32bin_hist( const vil_image_view<vxl_byte>& image,
                        double* normalized_histogram )
{
  unsigned hist[32] = { 0 };

  const std::ptrdiff_t istep = image.istep();
  const std::ptrdiff_t jstep = image.jstep();
  vxl_byte const* row = image.top_left_ptr();

  for( unsigned j=0; j<image.nj(); ++j, row += jstep )
  {
    vxl_byte const* pixel = row;
    for( unsigned i=0; i<image.ni(); ++i, pixel+=istep )
    {
      hist[ (*pixel) >> 3 ]++;
    }
  }

  unsigned entries = image.ni() * image.nj();
  entries = ( entries == 0 ? 1 : entries );

  for( unsigned i = 0; i < 32; i++ )
  {
    normalized_histogram[i] = static_cast<double>( hist[i] ) / entries;
  }
}

void simple_64bin_color_hist( const vil_image_view<vxl_byte>& image,
                              double* normalized_histogram,
                              double* average_color )
{
  const unsigned hist_steps[3] = { 16, 4, 1 };

  unsigned hist[64] = { 0 };
  unsigned color[3] = { 0 };

  // Get image properties
  const unsigned ni = image.ni();
  const unsigned nj = image.nj();
  const unsigned np = image.nplanes();
  const std::ptrdiff_t istep = image.istep();
  const std::ptrdiff_t jstep = image.jstep();
  const std::ptrdiff_t pstep = image.planestep();

  // Filter image
  const vxl_byte* row = image.top_left_ptr();
  for (unsigned j=0;j<nj;++j,row+=jstep)
  {
    const vxl_byte* pixel = row;
    for (unsigned i=0;i<ni;++i,pixel+=istep)
    {
      unsigned step = 0;
      const vxl_byte* plane = pixel;
      for (unsigned p=0;p<np;++p,plane+=pstep)
      {
        vxl_byte pixel_value = *plane;
        step += hist_steps[p] * ( pixel_value >> 6 );
        color[p] += static_cast<unsigned>( pixel_value );
      }
      (*(hist + step))++;
    }
  }

  unsigned entries = image.ni() * image.nj();
  entries = ( entries == 0 ? 1 : entries );

  for( unsigned i = 0; i < 3; i++ )
  {
    average_color[i] = static_cast<double>( color[i] ) / entries;
  }

  for( unsigned i = 0; i < 64; i++ )
  {
    normalized_histogram[i] = static_cast<double>( hist[i] ) / entries;
  }
}

bool
track_reinit_detector
::extract_features( const image_t& image,
                    const image_region_t& region,
                    const double region_scale,
                    double* output_loc,
                    const double /*rotation*/ ) const
{
  const track_reinit_settings& settings = internal_model_->settings_;

  // Extract HoG and dense BoW, if required
  if( settings.use_hog_descriptor ||
      settings.use_dense_bow_descriptor ||
      settings.use_ip_bow_descriptor )
  {
    vil_image_view<float> formatted_image;

    if( !compute_image_chip( image, region, formatted_image,
                             settings.chip_scale_factor,
                             settings.chip_shift_factor,
                             settings.min_chip_area,
                             settings.max_chip_area ) )
    {
      return false;
    }

    if( settings.use_hog_descriptor )
    {
      image_region_t full_region( 0, formatted_image.ni(), 0, formatted_image.nj() );

      integral_hog_descriptor<float> ii_hog( formatted_image );

      ii_hog.generate_hog_descriptor( full_region,
                                      internal_model_->ii_model_,
                                      output_loc + internal_model_->hog_offset );
    }

#ifdef USE_VLFEAT
    if( settings.use_dense_bow_descriptor )
    {
      dsift_bow_descriptor& dense_desc = internal_model_->dense_desc_;

      std::vector<float> feature_space;
      std::vector<double> mapping_space;

      dense_desc.compute_features( formatted_image, feature_space );
      dense_desc.compute_bow_descriptor( feature_space, mapping_space );

      const unsigned dense_offset = internal_model_->dense_bow_offset;

      for( unsigned i = 0; i < mapping_space.size(); i++ )
      {
        output_loc[ dense_offset + i ] = mapping_space[i];
      }

      if( mapping_space.size() == 0 )
      {
        for( unsigned i = 0; i < dense_desc.vocabulary().word_count(); i++ )
        {
          output_loc[ dense_offset + i ] = 0;
        }
      }
    }

    if( settings.use_ip_bow_descriptor )
    {
      mfeat_bow_descriptor& ip_desc = internal_model_->ip_desc_;

      std::vector<float> feature_space;
      std::vector<double> mapping_space;

      ip_desc.compute_features( formatted_image, feature_space );
      ip_desc.compute_bow_descriptor( feature_space, mapping_space );

      const unsigned ip_offset = internal_model_->ip_bow_offset;

      for( unsigned i = 0; i < mapping_space.size(); i++ )
      {
        output_loc[ ip_offset + i ] = mapping_space[i];
      }

      if( mapping_space.size() == 0 )
      {
        for( unsigned i = 0; i < ip_desc.vocabulary().word_count(); i++ )
        {
          output_loc[ ip_offset + i ] = 0;
        }
      }
    }
#endif
  }

  // Simple color features
  if( settings.use_color_descriptor )
  {
    vil_image_view< vxl_byte > color_roi = point_view_to_region( image, region );
    vil_image_view< vxl_byte > gray_roi;
    vil_convert_planes_to_grey( color_roi, gray_roi );

    if( color_roi.size() > 0 )
    {
      simple_32bin_hist( gray_roi, output_loc );
      simple_64bin_color_hist( color_roi, output_loc + 32, output_loc + 96 );
    }
    else
    {
      return false;
    }
  }

  // Simple area feature
  if( settings.use_area_descriptor )
  {
    output_loc[ internal_model_->area_offset ] = region_scale * region.area();
  }

  return true;
}

void
track_reinit_detector
::update_model( const training_data_t& positive_samples,
                const training_data_t& negative_samples )
{
  if( !positive_samples.empty() && !negative_samples.empty() &&
      positive_samples.size() + negative_samples.size() >= min_training_examples )
  {
    internal_model_->update( positive_samples, negative_samples );
  }
}

void
track_reinit_detector::object_model
::update( const training_data_t& positive_samples,
          const training_data_t& negative_samples )
{
  training_thread_task_sptr to_do(
    new model_update_threaded_task( this, positive_samples, negative_samples )
  );

  thread_->execute_if_able( to_do );
}

bool
track_reinit_detector::model_update_threaded_task
::execute()
{
#ifdef TIMING_TESTS
  boost::posix_time::ptime t1, t2;
  t1 = boost::posix_time::microsec_clock::local_time();
#endif

  const track_reinit_settings& settings = model_->settings_;
  const training_data_t& positive_samples = pos_;
  const training_data_t& negative_samples = neg_;

  default_model_sptr_t new_classifier;
#ifdef USE_OPENCV
  ocv_model_sptr_t new_classifier_ocv;
#endif

  if( settings.model_type == track_reinit_settings::ADABOOST )
  {
    std::vector< weak_learner_sptr > learners;
    learners.push_back( weak_learner_sptr( new stump_weak_learner( "stump", 0 ) ) );
    new_classifier.reset( new default_model_t( learners, settings.max_iterations ) );

    std::vector< learner_training_data_sptr > feature_data;

    for( unsigned i = 0; i < positive_samples.size(); i++ )
    {
      feature_data.push_back( learner_training_data_sptr( new classifier_data_wrapper( positive_samples.entry( i ),
                              model_->feature_count, 1 ) ) );
    }
    for( unsigned i = 0; i < negative_samples.size(); i++ )
    {
      feature_data.push_back( learner_training_data_sptr( new classifier_data_wrapper( negative_samples.entry( i ),
                              model_->feature_count, -1 ) ) );
    }

    new_classifier->train( feature_data );
  }
#ifdef USE_OPENCV
  else if( settings.model_type == track_reinit_settings::ADABOOST_OCV )
  {
    // declare variable here or we get unused variable warning when OpenCV disabled.
    const unsigned total_samples = pos_.size() + neg_.size();
    new_classifier_ocv.reset( new ocv_model_t() );
    ocv_params_t params = model_->classifier_ocv_params_;

    if( positive_samples.size() < 10 || negative_samples.size() < 10 )
    {
      params.weight_trim_rate = 1.0;
    }

    unsigned iterations = settings.max_iterations;

    if( total_samples < iterations )
    {
      iterations = total_samples;
    }

    params.weak_count = iterations;

    unsigned descriptor_count = positive_samples.size() + negative_samples.size();
    unsigned feature_count = model_->feature_count;

    cv::Mat input_features( descriptor_count, feature_count, CV_32F );
    cv::Mat input_classifications( descriptor_count, 1, CV_32S );
    cv::Mat var_type( feature_count + 1, 1, CV_8U );
    var_type = cv::Scalar( CV_VAR_ORDERED );
    var_type.at< unsigned char >( feature_count, 0 ) = CV_VAR_CATEGORICAL;
    cv::Mat sample_usage( descriptor_count, 1, CV_32S );
    sample_usage = cv::Scalar(1);
    cv::Mat var_usage( 1, feature_count, CV_8U );
    var_usage = cv::Scalar(1);

    const descriptor_t& positive_data = positive_samples.raw_data();
    const descriptor_t& negative_data = negative_samples.raw_data();

    for( unsigned j = 0; j < positive_samples.size(); j++ )
    {
      input_classifications.at< int >( j, 0 ) = 1;

      unsigned entry_offset =  j * feature_count;
      for( unsigned i = 0; i < feature_count; i++ )
      {
        input_features.at< float >( j, i ) = positive_data[ entry_offset + i ];
      }
    }

    unsigned positive_offset = positive_samples.size();

    for( unsigned j = 0; j < negative_samples.size(); j++ )
    {
      input_classifications.at< int >( positive_offset + j, 0 ) = -1;

      unsigned entry_offset =  j * feature_count;
      for( unsigned i = 0; i < feature_count; i++ )
      {
        input_features.at< float >( positive_offset + j, i ) = negative_data[ entry_offset + i ];
      }
    }

    new_classifier_ocv->train( input_features, CV_ROW_SAMPLE, input_classifications,
                               cv::Mat(), cv::Mat(), var_type, cv::Mat(), params, false );
  }
#endif

  // Lock and verify checksum
  {
    boost::unique_lock< boost::shared_mutex > lock( model_->update_mutex_ );

    if( !this->is_still_valid() )
    {
      return true;
    }

    model_->pos_count_ = positive_samples.size();
    model_->neg_count_ = negative_samples.size();

    if( settings.model_type == track_reinit_settings::ADABOOST )
    {
      model_->classifier_ = new_classifier;
      model_->classifier_trained_ = true;
    }
  #ifdef USE_OPENCV
    else if( settings.model_type == track_reinit_settings::ADABOOST_OCV )
    {
      if( new_classifier_ocv->get_weak_predictors() != NULL )
      {
        model_->classifier_ocv_ = new_classifier_ocv;
        model_->classifier_trained_ = true;
      }
    }
  #endif
    else
    {
      LOG_FATAL( "Invalid classifier type set" );
    }
  }

  LOG_INFO( "Finishing training obj classifier with samples: " <<
            positive_samples.size() << " " << negative_samples.size() );

#ifdef TIMING_TESTS
  t2 = boost::posix_time::microsec_clock::local_time();
  boost::posix_time::time_duration dur = t2 - t1;
  LOG_INFO( "TIMER: Classifier training stage took: " << dur.total_milliseconds() << " mseconds !!" );
#endif
  return true;
}

bool
track_reinit_detector
::classify_obj( const vil_image_view<vxl_byte>& image,
                const image_region_t& region,
                const double region_scale,
                const double /*rotation*/ )
{
  boost::shared_lock<boost::shared_mutex> lock( internal_model_->update_mutex_ );

  if( !internal_model_->classifier_trained_ )
  {
    return false;
  }

  std::vector< double > local_features( features_per_descriptor() );
  classifier_data_wrapper input_data( &local_features[0], local_features.size() );
  extract_features( image, region, region_scale, &local_features[0] );

  if( internal_model_->classifier_->classify( input_data ) > 0 )
  {
    return true;
  }

  return false;
}

unsigned
mover_category( const track_sptr& trk )
{
  unsigned low = 0, mid = 0, high = 0;

  const std::vector< track_state_sptr >& history = trk->history();

  unsigned start_index = history.size();

  if( start_index < 1 )
  {
    return 0;
  }

  if( start_index > 10 )
  {
    start_index -= 10;
  }
  else
  {
    start_index = 0;
  }

  for( unsigned i = start_index; i < history.size(); i++ )
  {
    double vel_mag = history[i]->vel_.magnitude();

    if( vel_mag > 9 )
    {
      high++;
    }
    else if( vel_mag > 2 )
    {
      mid++;
    }
    else
    {
      low++;
    }
  }

  if( high > low && high > mid )
  {
    return 3;
  }
  else if( mid > high && mid > low )
  {
    return 2;
  }

  return 1;
}

double
hist_intersection( double const* p1, double const* p2, unsigned int n )
{
  double min_sum = 0.0;

  for( double const* p1_end = p1 + n; p1 < p1_end; p1++, p2++ )
  {
    min_sum += std::min( *p1, *p2 );
  }

  return 1.0 - min_sum;
}

double
distance_l2( double const* p1, double const* p2, unsigned int n )
{
  double sum = 0.0, diff;

  for( double const* p1_end = p1 + n; p1 < p1_end; p1++, p2++ )
  {
    diff = *p1 - *p2;
    sum += ( diff * diff );
  }

  return std::sqrt( sum );
}

double
similarity_metric( double d1, double d2 )
{
  if( d1 == 0 || d2 == 0 )
  {
    return 0;
  }

  return ( d1 < d2 ? d1 / d2 : d2 / d1 );
}

double
color_dist_metric( const double* c1, const double* c2 )
{
  double t1 = c1[0] - c2[0];
  double t2 = c1[1] - c2[1];
  double t3 = c1[2] - c2[2];

  return std::sqrt( t1*t1 + t2*t2 + t3*t3 );
}

bool
find_matching_frame( const ring_buffer< timestamp >& ts_buffer,
                     const timestamp& input_ts,
                     unsigned & index )
{
  for( unsigned i = 0; i < ts_buffer.size(); i++ )
  {
    if( ts_buffer.datum_at( i ) == input_ts )
    {
      index = i;
      return true;
    }
  }
  return false;
}

double
bbox_ratio_score( const vgl_box_2d<unsigned>& bbox )
{
  return static_cast<double>( bbox.width() ) / ( 0.000001 + bbox.height() );
}

bool
is_boundary( const vgl_box_2d<unsigned>& bbox, const unsigned ni, const unsigned nj )
{
  vgl_box_2d< unsigned > check_bbox( 0.20 * ni, 0.8 * ni, 0.2 * nj, 0.8 * nj );

  if( vgl_intersection( check_bbox, bbox ).area() > 0 )
  {
    return false;
  }
  return true;
}

double
track_reinit_detector::object_model
::classify_features( const std::vector< double >& features )
{
  if( settings_.model_type == track_reinit_settings::ADABOOST )
  {
    classifier_data_wrapper input_data( &features[0], features.size() );
    return classifier_->classify_raw( input_data );
  }
#ifdef USE_OPENCV
  else if( settings_.model_type == track_reinit_settings::ADABOOST_OCV )
  {
    cv::Mat sample( 1, features.size(), CV_32F );

    for( unsigned i = 0; i < features.size(); i++ )
    {
      sample.at< float >( 0, i ) = features[i];
    }

    return classifier_ocv_->predict( sample, cv::Mat(), cv::Range::all(), false, true );
  }
#endif

  LOG_FATAL( "Invalid classifier type specified" );
  return -1.0;
}

bool
track_reinit_detector
::create_linking_descriptor( const ring_buffer< image_t >& image_buffer,
                             const ring_buffer< timestamp >& ts_buffer,
                             const track_sptr& track,
                             const double region_scale,
                             const double current_time,
                             const homography_t& homog,
                             descriptor_t& linking_descriptor )
{
  const image_t& image = image_buffer.datum_at( image_buffer.head() );
  const track_state& latest_state = *(track->last_state());

  image_region_t region;

  prior_state_info& psi = internal_model_->last_state_info_1_;

  if( internal_model_->last_state_info_2_.is_set_ )
  {
    psi = internal_model_->last_state_info_2_;
  }

  if( !internal_model_->classifier_trained_ ||
      !latest_state.bbox( region ) ||
      !psi.is_set_ )
  {
    linking_descriptor.clear();
    return false;
  }

  // Extract appearance descriptor
  descriptor_t appearance_descriptor( features_per_descriptor() );
  descriptor_t tmp_descriptor( features_per_descriptor() );
  linking_descriptor.resize( linking_desc_size, 0.0 );

  if( !extract_features( image, region, region_scale, &appearance_descriptor[0] ) )
  {
    linking_descriptor.clear();
    return false;
  }

  // Extract simple general properties
  vnl_vector_fixed< double, 2 > img_loc, world_loc;
  vnl_vector_fixed< double, 3 > world_vel = latest_state.vel_;

  if( !latest_state.get_image_loc( img_loc[0], img_loc[1] ) )
  {
    img_loc[0] = region.centroid_x();
    img_loc[1] = region.centroid_y();
  }

  const vnl_double_3x3& transform = homog.get_transform().get_matrix();
  const track_reinit_settings& settings = internal_model_->settings_;

  vnl_double_3 op( img_loc[0], img_loc[1], 1.0 );
  vnl_double_3 tp = transform * op;

  if( tp[2] != 0 )
  {
    world_loc[0] = tp[0] / tp[2];
    world_loc[1] = tp[1] / tp[2];
  }
  else
  {
    world_loc[0] = 0;
    world_loc[1] = 0;
  }

  // Linking descriptor
  //
  // Bin 0 - Raw decision based on appearance model
  // Bin 1 - Output classification weight from appearance model
  // Bin 2 - Past output classification weight on appearance model
  // Bin 3 - Further past output classification weight on appearance model
  // Bin 4 - Color similarity 1 (if applic.)
  // Bin 5 - Color similarity 2 (if applic.)
  // Bin 6 - Color similarity 3 (if applic.)
  // Bin 7 - Size similarity 1 (if applic.)
  // Bin 8 - Size similarity 2 (if applic.)
  // Bin 9 - Pixel distance between last detect
  // Bin 10 - World distance between last detect
  // Bin 11 - World speed (velocity magnitude) of last detect
  // Bin 12 - Velocity similarity (if applic.)
  // Bin 13 - Time which has based since last detect
  // Bin 14 - Distance from estimated travel point
  // Bin 15 - Is the object within a feasible distance of the prior target?
  // Bin 16 - Classifier Rank
  // Bin 17 - Location rank given positive classification
  // Bin 18 - Are there more than one positive appearance classifications?
  // Bin 19 - Source mover category
  // Bin 20 - Dest mover category
  // Bin 21 - Source and dest have same mover category?
  // Bin 22 - Region Scale
  // Bin 23 - Source BBOX area (pixels)
  // Bin 24 - Target BBOX area (pixels)
  // Bin 25 - BBOX area similarity (pixels)
  // Bin 26 - Source BBOX ratio
  // Bin 27 - Target BBOX ratio
  // Bin 28 - BBOX ratio similarity
  // Bin 29 - Positive training sample count
  // Bin 30 - Negative training sample count
  // Bin 31 - Number of other tracks within consideration in short radius to target
  // Bin 32 - Length of seed track
  // Bin 33 - Is seed track near boundary
  // Bin 34 - Is target track near boundary
  // Bin 35 - Are seed and target both near boundary
  // Bin 36 - Percent of track region with positive FG mask
  // Bin 37 - Rank of track w.r.t last pixel location
  // Bin 38 - HoG similarity factor
  // Bin 39 - BoW similarity factor
  // Bin 40 - GSD-BBOX Scale Ratio
  {
    boost::shared_lock<boost::shared_mutex> lock( internal_model_->update_mutex_ );

    if( !internal_model_->classifier_trained_ )
    {
      linking_descriptor.clear();
      return false;
    }

    linking_descriptor[1] = internal_model_->classify_features( appearance_descriptor );
    linking_descriptor[0] = ( linking_descriptor[1] > 0 ? 1.0 : -1.0 );

    if( linking_descriptor[1] > 0 )
    {
      // Compute historical descriptors
      const track_state& middle_state = *(track->history()[ track->history().size()/2 ]);
      unsigned image_index;

      if( !middle_state.bbox( region ) ||
          middle_state.time_ == latest_state.time_ ||
          !find_matching_frame( ts_buffer, middle_state.time_, image_index ) )
      {
        linking_descriptor[2] = linking_descriptor[1];
      }
      else
      {
        extract_features( image_buffer.datum_at( image_index ), region, region_scale, &tmp_descriptor[0] );
        linking_descriptor[2] = internal_model_->classify_features( tmp_descriptor );
      }

      if( linking_descriptor[2] > 0 )
      {
        const track_state& first_state = *(track->first_state());

        if( !first_state.bbox( region ) ||
            first_state.time_ == middle_state.time_ ||
            !find_matching_frame( ts_buffer, first_state.time_, image_index ) )
        {
          linking_descriptor[3] = linking_descriptor[2];
        }
        else
        {
          extract_features( image_buffer.datum_at( image_index ), region, region_scale, &tmp_descriptor[0] );
          linking_descriptor[3] = internal_model_->classify_features( tmp_descriptor );
        }
      }
    }
  }

  const descriptor_t& last_desc = psi.last_descriptor_;
  double temporal_difference = current_time - psi.last_time_;

  if( settings.use_color_descriptor )
  {
    linking_descriptor[4] = hist_intersection( &last_desc[0], &appearance_descriptor[0], 32 );
    linking_descriptor[5] = hist_intersection( &last_desc[32], &appearance_descriptor[32], 64 );
    linking_descriptor[6] = color_dist_metric( &last_desc[96], &appearance_descriptor[96] );
  }

  if( settings.use_area_descriptor )
  {
    double current = appearance_descriptor[ internal_model_->area_offset ];
    double prior = last_desc[ internal_model_->area_offset ];

    linking_descriptor[7] = similarity_metric( current, prior );
    linking_descriptor[8] = 0.0;
  }

  linking_descriptor[9] = ( img_loc - psi.last_img_loc_ ).magnitude();
  linking_descriptor[10] = ( world_loc - psi.last_world_loc_ ).magnitude();
  linking_descriptor[11] = psi.last_world_vel_.magnitude();
  linking_descriptor[12] = similarity_metric( linking_descriptor[11], world_vel.magnitude() );
  linking_descriptor[13] = temporal_difference;

  linking_descriptor[14] = std::abs( linking_descriptor[11] * temporal_difference - linking_descriptor[10] );
  linking_descriptor[15] = ( linking_descriptor[14] / ( linking_descriptor[11] * temporal_difference + 1.0 ) );

  linking_descriptor[16] = 10.0;
  linking_descriptor[17] = 10.0;
  linking_descriptor[18] = 0.0;

  linking_descriptor[19] = ( mover_category( track ) );
  linking_descriptor[20] = ( psi.last_mover_category_ ? 1.0 : 0.0 );
  linking_descriptor[21] = ( linking_descriptor[19] == linking_descriptor[20] ? 1.0 : 0.0 );

  linking_descriptor[22] = region_scale;

  linking_descriptor[23] = psi.last_bbox_.area();
  linking_descriptor[24] = region.area();
  linking_descriptor[25] = similarity_metric( linking_descriptor[23], linking_descriptor[24] );

  double bbox_ratio = linking_descriptor[24] / ( linking_descriptor[23] + 0.00001 );
  double scale_ratio = region_scale / ( psi.last_scale_ + 0.00001 );

  linking_descriptor[40] = scale_ratio / ( bbox_ratio + 0.00001 );

  linking_descriptor[26] = bbox_ratio_score( psi.last_bbox_ );
  linking_descriptor[27] = bbox_ratio_score( region );
  linking_descriptor[28] = similarity_metric( linking_descriptor[26], linking_descriptor[27] );

  linking_descriptor[29] = internal_model_->pos_count_;
  linking_descriptor[30] = internal_model_->neg_count_;

  linking_descriptor[31] = 0.0;

  linking_descriptor[32] = psi.last_track_length_;

  linking_descriptor[33] = ( is_boundary( psi.last_bbox_, image.ni(), image.nj() ) ? 1.0 : 0.0 );
  linking_descriptor[34] = ( is_boundary( region, image.ni(), image.nj() ) ? 1.0 : 0.0 );
  linking_descriptor[35] = ( linking_descriptor[33] == linking_descriptor[34] ? 1.0 : 0.0 );

  linking_descriptor[36] = 0.0;
  linking_descriptor[37] = 10.0;
  linking_descriptor[38] = 0.0;

  if( settings.use_hog_descriptor )
  {
    unsigned offset = internal_model_->hog_offset;

    linking_descriptor[38] = distance_l2( &last_desc[offset], &appearance_descriptor[offset], internal_model_->ii_model_.feature_count() );
  }

  linking_descriptor[39] = 0.0;

#ifdef USE_VLFEAT
  if( settings.use_dense_bow_descriptor )
  {
    unsigned offset = internal_model_->dense_bow_offset;

    linking_descriptor[39] = hist_intersection( &last_desc[offset], &appearance_descriptor[offset], internal_model_->dense_desc_.vocabulary().word_count() );
  }
#else
  linking_descriptor[39] = 0.0;
#endif

  return true;
}


unsigned
track_reinit_detector
::features_per_descriptor() const
{
  return internal_model_->feature_count;
}

bool
track_reinit_detector
::is_trained() const
{
  return internal_model_->classifier_trained_;
}

void
track_reinit_detector::object_model
::reset()
{
  if( thread_ )
  {
    thread_->forced_reset();
  }

  classifier_trained_ = false;

  pos_count_ = 0;
  neg_count_ = 0;

  reset_state_vars();
}

void
track_reinit_detector::object_model
::reset_state_vars()
{
  last_state_info_1_ = prior_state_info();
  last_state_info_2_ = prior_state_info();
}

void
track_reinit_detector
::reset()
{
  internal_model_->reset();
}

bool
track_reinit_detector
::set_last_state_info( const homography_t& homog,
                       const track_t& trk,
                       const descriptor_t& descriptor,
                       const double region_scale )
{
  const track_state& state = *(trk->last_state());

  double x, y;

  if( !state.time_.has_time() ||
      !state.get_image_loc( x, y ) ||
      !homog.is_valid() )
  {
    return false;
  }

  internal_model_->last_state_info_2_ = internal_model_->last_state_info_1_;

  internal_model_->last_state_info_1_.is_set_ = true;
  internal_model_->last_state_info_1_.last_descriptor_ = descriptor;
  internal_model_->last_state_info_1_.last_time_ = state.time_.time_in_secs();
  internal_model_->last_state_info_1_.last_img_loc_ = vnl_vector_fixed<double,2>( x, y );
  internal_model_->last_state_info_1_.last_world_vel_ = state.vel_;
  internal_model_->last_state_info_1_.last_mover_category_ = mover_category( trk );
  internal_model_->last_state_info_1_.last_scale_ = region_scale;

  const vnl_double_3x3& transform = homog.get_transform().get_matrix();

  vnl_double_3 op( x, y, 1.0 );
  vnl_double_3 tp = transform * op;

  if( tp[2] != 0 )
  {
    internal_model_->last_state_info_1_.last_world_loc_ = vnl_vector_fixed<double,2>( tp[0] / tp[2], tp[1] / tp[2] );
  }

  state.bbox( internal_model_->last_state_info_1_.last_bbox_ );

  internal_model_->last_state_info_1_.last_track_length_ = trk->history().size();

  return true;
}

} // end namespace vidtk
