/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "scene_obstruction_detector.h"

#include <video_transforms/floating_point_image_hash_functions.h>

#include <utilities/point_view_to_region.h>

#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/vil_math.h>
#include <vil/vil_resample_bilin.h>

#include <vil/algo/vil_threshold.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_gauss_filter.h>

#include <cmath>
#include <limits>
#include <fstream>
#include <sstream>
#include <limits>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "scene_obstruction_detector" );

// Number of bins for the internal color histogram used to estimate mask properties
const static unsigned hist_bins = 16;

template <typename PixType, typename FeatureType>
scene_obstruction_detector<PixType,FeatureType>
::scene_obstruction_detector( const scene_obstruction_detector_settings& options )
{
  if( !this->configure( options ) )
  {
    // Log error, but don't crash unless we actually run an invalid detector
    LOG_ERROR("Unable to configure scene_obstructor_detector!");
  }
}


template <typename PixType, typename FeatureType>
bool
scene_obstruction_detector<PixType,FeatureType>
::configure( const scene_obstruction_detector_settings& options )
{
  is_valid_ = false;
  frame_counter_ = 0;
  frames_since_last_break_ = 0;

  // Set local copy of settings
  options_ = options;

  // Load classifiers
  if( !options_.is_training_mode_ )
  {
    if( !initial_classifier_.load_from_file( options.primary_classifier_filename_ ) )
    {
      LOG_ERROR( "Invalid initial classifier specified" );
      return false;
    }
    if( options.use_appearance_classifier_ )
    {
      if( !appearance_classifier_.load_from_file( options.appearance_classifier_filename_ ) )
      {
        LOG_ERROR( "Invalid appearance classifier specified" );
        return false;
      }
    }
  }

  // Configure ring buffer
  mask_count_history_.set_capacity( options_.mask_count_history_length_ );
  mask_intensity_history_.set_capacity( options_.mask_intensity_history_length_ );

  // Compute bitshift based on the input type which maps each pixel to a 16 binned
  // histogram. Is used for quickly estimating mask properties when determining
  // when breaks have occured in later computation.
  unsigned scale_factor =
    ( static_cast<unsigned>( std::numeric_limits<PixType>::max() ) + 1 ) / hist_bins;

  color_hist_scale_ = scale_factor;
  color_hist_bitshift_ = 0;
  while( ( scale_factor >>= 1 ) != 0 ) // (log2 for unsigned)
  {
    color_hist_bitshift_++;
  }

  // Set training mode parameters
  if( options.is_training_mode_ )
  {
    training_data_extractor_.reset( new pixel_feature_writer< FeatureType >() );

    pixel_feature_writer_settings trainer_options;
    trainer_options.output_file = options.output_filename_;
    trainer_options.gt_loader.load_mode = pixel_annotation_loader_settings::DIRECTORY;
    trainer_options.gt_loader.path = options.groundtruth_dir_;

    if( !training_data_extractor_->configure( trainer_options ) )
    {
      LOG_ERROR( "Error setting up training data extractor" );
      return false;
    }
  }

  if( options.use_spatial_prior_feature_ && !options.spatial_prior_filename_.empty() )
  {
    spatial_prior_image_ = vil_load( options.spatial_prior_filename_.c_str() );

    if( spatial_prior_image_.size() == 0 )
    {
      LOG_ERROR( "Unable to load spatial prior image: " << options.spatial_prior_filename_ );
      return false;
    }

    if( spatial_prior_image_.nplanes() != 1 )
    {
      LOG_ERROR( "Spatial prior image must contain 1 plane!" );
      return false;
    }
  }

  is_valid_ = true;
  return true;
}

template <typename PixType, typename FeatureType>
void
scene_obstruction_detector<PixType,FeatureType>
::process_frame( const source_image& input_image,
                 const feature_array& input_features,
                 const image_border& input_border,
                 const variance_image& pixel_variance,
                 classified_image& output_image,
                 properties& output_properties )
{
  LOG_ASSERT( is_valid_, "Internal model is not valid!" );
  LOG_ASSERT( input_features.size() > 0, "No input features provided!" );
  LOG_ASSERT( input_features[0].ni() == input_image.ni(), "Input widths do not match." );
  LOG_ASSERT( input_features[0].nj() == input_image.nj(), "Input heights do not match." );

  props_.break_flag_ = false;
  feature_array full_feature_array = input_features;

  // Initialize new buffers on the first frame
  if( frame_counter_ == 0 )
  {
    this->trigger_mask_break( input_image );

    if( options_.use_spatial_prior_feature_ )
    {
      this->configure_spatial_prior( input_image );
    }
  }

  // Increment frame counters
  frame_counter_++;
  frames_since_last_break_++;

  // Update total variance image (since last shot break)
  vil_math_image_sum( pixel_variance, summed_variance_, summed_variance_ );
  double scale_factor = options_.variance_scale_factor_ / frames_since_last_break_;
  double max_input_value = std::numeric_limits<FeatureType>::max() / scale_factor;
  scale_float_img_to_interval( summed_variance_, normalized_variance_, scale_factor, max_input_value );

  // Add variance image and region id image to feature list
  full_feature_array.push_back( normalized_variance_ );

  if( options_.use_spatial_prior_feature_ )
  {
    full_feature_array.push_back( spatial_prior_image_ );
  }

  // Reset output image
  output_image.set_size( input_image.ni(),input_image.nj(), 2 );
  vil_image_view<double> statics = vil_plane( output_image, 0 );
  vil_image_view<double> everything = vil_plane( output_image, 1 );

  // Optional debug feature output
  if( options_.output_feature_image_mode_ )
  {
    this->output_feature_images( full_feature_array );
  }

  // If in training mode, extract feature data
  if( options_.is_training_mode_ )
  {
    this->training_data_extractor_->step( full_feature_array );
    return;
  }

  // Generate initial per-pixel mask classification
  this->perform_initial_approximation( full_feature_array,
                                       input_border,
                                       output_image );

  // Estimate mask properties
  bool break_detected = this->estimate_mask_properties( input_image,
                                                        input_border,
                                                        output_image );

  // Guess what color the mask is, under the assumption its a solid color
  if( options_.enable_mask_break_detection_ && break_detected )
  {
    bool reclassify = options_.use_appearance_classifier_ &&
                      frames_since_last_break_ > options_.appearance_frames_;

    this->trigger_mask_break( input_image );

    // Reperform initial approximation using appearance metrics only
    if( reclassify )
    {
      this->perform_initial_approximation( full_feature_array,
                                           input_border,
                                           output_image );
    }
  }

  // Output weighted classification image for each feature
  if( options_.output_classifier_image_mode_ )
  {
    this->output_classifier_images( full_feature_array,
                                    input_border,
                                    output_image );
  }

  // Set output properties
  output_properties = props_;
}

template <typename PixType, typename FeatureType>
void
scene_obstruction_detector<PixType,FeatureType>
::configure_spatial_prior( const source_image& input )
{
  if( spatial_prior_image_.size() == 0 )
  {
    spatial_prior_image_.set_size( input.ni(),input.nj(), 1 );

    for( unsigned i = 0; i < input.ni(); i++ )
    {
      for( unsigned j = 0; j < input.nj(); j++ )
      {
        vxl_byte i_id = ( options_.spatial_prior_grid_length_ * i ) / input.ni();
        vxl_byte j_id = ( options_.spatial_prior_grid_length_ * j ) / input.nj();
        spatial_prior_image_(i,j) = options_.spatial_prior_grid_length_ * j_id + i_id;
      }
    }
  }
  else if( spatial_prior_image_.ni() != input.ni() ||
           spatial_prior_image_.nj() != input.nj() )
  {
    vil_image_view< FeatureType > new_spatial_prior_image( input.ni(), input.nj(), 1 );
    vil_resample_bilin( spatial_prior_image_, new_spatial_prior_image, input.ni(), input.nj() );
    spatial_prior_image_ = new_spatial_prior_image;
  }
}

template <typename PixType, typename FeatureType>
void
scene_obstruction_detector<PixType,FeatureType>
::trigger_mask_break( const source_image& input )
{
  summed_history_.set_size( input.ni(),input.nj(), 1 );
  summed_history_.fill( 0 );
  summed_variance_.set_size( input.ni(),input.nj(), 1 );
  summed_variance_.fill( 0 );

  cumulative_intensity_ = 0;
  cumulative_mask_count_ = 0;
  frames_since_last_break_ = 0;

  props_.break_flag_ = true;
}

// Decide on the adaptive thresholding contribution
template <typename PixType, typename FeatureType>
double
scene_obstruction_detector<PixType,FeatureType>
::adaptive_threshold_contribution( const unsigned& frame_num )
{
  if( !options_.use_adaptive_thresh_ )
  {
    return 0.0;
  }
  if( frame_num > options_.at_pivot_2_ )
  {
    return options_.at_interval_3_adj_;
  }
  else if( frame_num > options_.at_pivot_1_ )
  {
    return options_.at_interval_2_adj_;
  }
  return options_.at_interval_1_adj_;
}

template <typename PixType, typename FeatureType>
void
scene_obstruction_detector<PixType,FeatureType>
::perform_initial_approximation( const feature_array& features,
                                 const image_border& border,
                                 classified_image& output_image )
{
  // Formulate classifier input (vector of single plane byte images)
  vil_image_view<double> approx_output = vil_plane( output_image, 1 );

  // A small (negatively classified) value used for initially filling
  // the output classification. For the most part is not used, except
  // for pixels which don't get processed (ie those outside of the identified
  // border).
  const double negative_fill_value = options_.initial_threshold_-std::numeric_limits<double>::epsilon();

  // Seed the output approximate
  approx_output.fill( negative_fill_value );

  // Create new input array to feed to classifiers
  feature_array input_array;

  // Scale views for image border if present
  if( border.area() > 0 )
  {
    for( unsigned i = 0; i < features.size(); i++ )
    {
      input_array.push_back( point_view_to_region( features[i], border ) );
    }
    approx_output = point_view_to_region( approx_output, border );
  }
  else
  {
    input_array = features;
  }

  // Perform classification
  double offset = adaptive_threshold_contribution( frames_since_last_break_ );
  if( !options_.use_appearance_classifier_ || frames_since_last_break_ > options_.appearance_frames_ )
  {
    initial_classifier_.classify_images( input_array, approx_output, offset );
  }
  else
  {
    appearance_classifier_.classify_images( input_array, approx_output, offset );
  }

  // Add classification to history and copy history to output
  vil_image_view<double> initial = vil_plane( output_image, 1 );
  vil_image_view<double> statics = vil_plane( output_image, 0 );
  vil_math_image_sum( initial, summed_history_, summed_history_ );
  statics.deep_copy( summed_history_ );
}

unsigned
calculate_border_degree( const double* ptr,
                         std::ptrdiff_t istep,
                         std::ptrdiff_t jstep,
                         double threshold,
                         const unsigned max_search = 3 )
{
  for( unsigned s = 1; s < max_search; ++s )
  {
    std::ptrdiff_t sistep = s*istep;
    std::ptrdiff_t sjstep = s*jstep;

    if( *(ptr+sistep) < threshold || *(ptr-sistep) < threshold ||
        *(ptr+sjstep) < threshold || *(ptr-sjstep) < threshold )
    {
      return s;
    }
  }

  return max_search;
}

unsigned
calculate_long_degree( const double* ptr,
                       std::ptrdiff_t istep,
                       std::ptrdiff_t jstep,
                       double threshold,
                       const unsigned itr_step = 5,
                       const unsigned max_search = 3 )
{
  unsigned best_degree = 0;

  for( unsigned s = 1; s < max_search + 1; ++s )
  {
    std::ptrdiff_t sistep = s*itr_step*istep;

    if( *(ptr+sistep) > threshold && *(ptr-sistep) > threshold )
    {
      best_degree = std::max( best_degree, s );
    }
    else
    {
      break;
    }
  }

  for( unsigned s = 1; s < max_search + 1; ++s )
  {
    std::ptrdiff_t sjstep = s*itr_step*jstep;

    if( *(ptr+sjstep) > threshold && *(ptr-sjstep) > threshold )
    {
      best_degree = std::max( best_degree, s );
    }
    else
    {
      break;
    }
  }

  return best_degree;
}

template <typename PixType, typename FeatureType>
bool
scene_obstruction_detector<PixType,FeatureType>
::estimate_mask_properties( const source_image& input,
                            const image_border& border,
                            const classified_image& output )
{
#ifdef SOD_DEBUG_IMAGE
  source_image to_output;
  to_output.deep_copy( input );
#endif

  LOG_ASSERT( input.nplanes() == 3 || input.nplanes() == 1, "Invalid plane count" );

  // Estimate colour and intensity (take a quick cross-channel mode).
  // Concurrently count # of pixels in the detected mask approximation.
  const bool was_border_set = ( border.area() > 0 );
  const bool is_color_image = ( input.nplanes() == 3 );

  const unsigned lower_i = ( was_border_set ? border.min_x() : 0 );
  const unsigned upper_i = ( was_border_set ? border.max_x() : output.ni() );
  const unsigned lower_j = ( was_border_set ? border.min_y() : 0 );
  const unsigned upper_j = ( was_border_set ? border.max_y() : output.nj() );

  unsigned hist[3][hist_bins] = {{0}};

  const std::ptrdiff_t oistep = output.istep();
  const std::ptrdiff_t ojstep = output.jstep();

  const std::ptrdiff_t ipstep = input.planestep();
  const std::ptrdiff_t ip2step = 2 * ipstep;

  const double threshold = options_.initial_threshold_;

  // Automatically discount any pixels directly near the estimated border
  // because they might be warped towards the border color due to compression artifacts.
  const unsigned borderadj = 20;

  unsigned init_clfr_counter = 0;

  for( unsigned j = lower_j+borderadj; j < upper_j-borderadj; j++ )
  {
    const double *init_clfr_ptr = &output(lower_i+borderadj, j, 1);

    for( unsigned i = lower_i+borderadj; i < upper_i-borderadj; i++, init_clfr_ptr+=oistep )
    {
      if( *init_clfr_ptr > threshold )
      {
        init_clfr_counter++;

        if( output(i,j,0) > threshold &&
            calculate_border_degree( init_clfr_ptr, oistep, ojstep, threshold, 4 ) > 3 &&
            calculate_long_degree( init_clfr_ptr, oistep, ojstep, threshold, 6, 3 ) > 2 )
        {
          const PixType* color = &input(i,j);
          hist[0][ *(color) >> color_hist_bitshift_ ]++;

#ifdef SOD_DEBUG_IMAGE
          to_output(i,j,0) = 255;
          to_output(i,j,2) = 255;
#endif

          if( is_color_image )
          {
            hist[1][ *(color + ipstep) >> color_hist_bitshift_ ]++;
            hist[2][ *(color + ip2step) >> color_hist_bitshift_ ]++;
          }
        }
      }
    }
  }

#ifdef SOD_DEBUG_IMAGE
  static int counter = 0;
  counter++;
  std::string fn = "frame" + boost::lexical_cast< std::string >( counter ) + ".png";
  vil_save( to_output, fn.c_str() );
#endif

  // Calculate mode and cumulative histograms
  const unsigned div = hist_bins / 3;

  unsigned top_ind[3] = {0};
  unsigned max_val[3] = {0};

  unsigned cum_hist[3][hist_bins] = {{0}};
  unsigned lmh_sum[3][3] = {{0}};

  for( unsigned c = 0; c < 3; ++c )
  {
    for( unsigned i = 0; i < hist_bins; ++i )
    {
      if( hist[c][i] > max_val[c] )
      {
        max_val[c] = hist[c][i];
        top_ind[c] = i;
      }

      if( i != 0 )
      {
        cum_hist[c][i] = cum_hist[c][i-1] + hist[c][i];
      }
    }

    lmh_sum[c][0] = cum_hist[c][div-1];
    lmh_sum[c][1] = cum_hist[c][hist_bins-1-div] - cum_hist[c][div-1];
    lmh_sum[c][2] = cum_hist[c][hist_bins-1] - cum_hist[c][hist_bins-1-div];
  }

  // Set color in property options, apply any settings
  props_.color_.r = top_ind[0] * color_hist_scale_ + color_hist_scale_/2;

  if( is_color_image )
  {
    props_.color_.g = top_ind[1] * color_hist_scale_ + color_hist_scale_/2;
    props_.color_.b = top_ind[2] * color_hist_scale_ + color_hist_scale_/2;
  }
  else
  {
    props_.color_.g = props_.color_.r;
    props_.color_.b = props_.color_.r;
  }

  const unsigned invalid_color = 5;
  unsigned color_index = invalid_color;
  unsigned color_weights[invalid_color] = {0};

  color_weights[0] = lmh_sum[0][0] + lmh_sum[1][0] + lmh_sum[2][0];
  color_weights[1] = lmh_sum[0][2] + lmh_sum[1][2] + lmh_sum[2][2];
  color_weights[2] = lmh_sum[0][1] + lmh_sum[1][1] + lmh_sum[2][1];

  if( is_color_image )
  {
    color_weights[3] = lmh_sum[0][0] + lmh_sum[1][2] + lmh_sum[2][0];
    color_weights[4] = lmh_sum[0][0] + lmh_sum[1][0] + lmh_sum[2][2];
  }

  if( options_.no_gray_filter_ )
  {
    color_weights[2] = 0;
  }

  if( options_.map_colors_to_nearest_extreme_ )
  {
    color_index = std::distance( color_weights,
      std::max_element( color_weights, color_weights + 4 ) );
  }
  else if( options_.map_colors_near_extremes_only_ )
  {
    color_index = std::distance( color_weights,
      std::max_element( color_weights, color_weights + 4 ) );

    unsigned max_option = 0;

    for( unsigned i = 0; i < 3; ++i )
    {
      for( unsigned j = 0; j < 3; ++j )
      {
        for( unsigned k = 0; k < 3; ++k )
        {
          unsigned color_count = lmh_sum[0][i] + lmh_sum[1][j] + lmh_sum[2][k];
          max_option = std::max( max_option, color_count );
        }
      }
    }

    if( color_weights[ color_index ] < max_option )
    {
      color_index = invalid_color;
    }
  }

  switch( color_index )
  {
    case 0:
      props_.color_.r = 0;
      props_.color_.g = 0;
      props_.color_.b = 0;
      break;
    case 1:
      props_.color_.r = std::numeric_limits<FeatureType>::max();
      props_.color_.g = std::numeric_limits<FeatureType>::max();
      props_.color_.b = std::numeric_limits<FeatureType>::max();
      break;
    case 2:
      props_.color_.r = std::numeric_limits<FeatureType>::max() / 2;
      props_.color_.g = std::numeric_limits<FeatureType>::max() / 2;
      props_.color_.b = std::numeric_limits<FeatureType>::max() / 2;
      break;
    case 3:
      props_.color_.r = 0;
      props_.color_.g = std::numeric_limits<FeatureType>::max();
      props_.color_.b = 0;
      break;
    case 4:
      props_.color_.r = 0;
      props_.color_.g = 0;
      props_.color_.b = std::numeric_limits<FeatureType>::max();
      break;
    default:
      break;
  }

  props_.intensity_ = props_.color_.grey();

  // Now that we have our recorded properties, enter them into our
  // history and determine if there may have been a break.
  //
  // Feature1 = % difference between the pixel count and the pcount average
  // Feature2 = # of std devs away from pixel count average
  // Feature3 = Difference of raw color output and color average
  double avg = 0.0, dev = 0.0;
  double pix_count = static_cast<double>( init_clfr_counter );

  if( mask_count_history_.size() == 0 )
  {
    mask_count_history_.insert( pix_count );
    mask_intensity_history_.insert( props_.intensity_ );
    return false;
  }

  for( unsigned i = 0; i < mask_count_history_.size(); i++ )
  {
    avg += mask_count_history_.datum_at( i );
  }

  avg /= mask_count_history_.size();

  for( unsigned i = 0; i < mask_count_history_.size(); i++ )
  {
    double term = mask_count_history_.datum_at( i ) - avg;
    dev += term * term;
  }

  dev = std::sqrt( dev );

  double feature1 = 0.0, feature2 = 0.0;

  if( avg > pix_count )
  {
    feature1 = avg / ( pix_count+1 );
  }
  else
  {
    feature1 = pix_count / ( avg+1 );
  }

  if( dev != 0 )
  {
    feature2 = std::fabs( ( pix_count - avg ) / dev );
  }

  double color_avg = 0.0;

  for( unsigned i = 0; i < mask_intensity_history_.size(); i++ )
  {
    color_avg += mask_intensity_history_.datum_at(i);
  }

  color_avg /= mask_intensity_history_.size();

  // Threshold all features via the given parameters
  if( feature1 > options_.count_percent_change_req_ ||
      feature2 > options_.count_std_dev_req_ ||
      ( mask_intensity_history_.size() > options_.min_hist_for_intensity_diff_ &&
      std::abs( static_cast<double>( props_.intensity_ ) - color_avg ) > options_.intensity_diff_req_ ) )
  {
    mask_count_history_.clear();
    mask_intensity_history_.clear();
    mask_count_history_.insert( pix_count );
    mask_intensity_history_.insert( props_.intensity_ );
    LOG_INFO( "HUD change detected on frame: " << frame_counter_ );
    return true;
  }

  mask_count_history_.insert( pix_count );
  mask_intensity_history_.insert( props_.intensity_ );
  return false;
}

// -------------------- Training Mode Functions ------------------------

// Debug function used for outputting all utilized input features
template <typename PixType, typename FeatureType>
void
scene_obstruction_detector<PixType,FeatureType>
::output_feature_images( const feature_array& features )
{
  std::string frame_id = boost::lexical_cast<std::string>( frame_counter_-1 );
  std::string prefix = "frame_" + frame_id + "_feature";
  std::string postfix = ".png";
  write_feature_images( features, prefix, postfix, true );
}

// Debug function used for outputing a single classified image, scaled to value max_to_rep
void
print_scaled_classifier_image( std::string fn,
                               const vil_image_view<double>& classifier_image,
                               double max_to_rep )
{
  vil_image_view<double> input_copy;
  input_copy.deep_copy( classifier_image );
  vil_image_view<vxl_byte> writeme( classifier_image.ni(), classifier_image.nj(), 3 );
  vil_image_view<double> temp1, temp2;
  writeme.fill( 0 );
  vil_math_scale_and_offset_values( input_copy, 255.0 / max_to_rep, 0.0 );
  temp1.deep_copy( input_copy );
  temp2.deep_copy( input_copy );
  vil_math_truncate_range( temp1, 1.0, 254.0 );
  vil_math_truncate_range( temp2, -254.0, -1.0 );
  vil_math_scale_values( temp2, -1.0 );
  vil_image_view<vxl_byte> red_plane = vil_plane( writeme, 0 );
  vil_image_view<vxl_byte> green_plane = vil_plane( writeme, 1 );
  vil_convert_cast( temp1, green_plane );
  vil_convert_cast( temp2, red_plane );
  vil_save( writeme, fn.c_str() );
}

// Debug function used for outputting classifier outputs
template <typename PixType, typename FeatureType>
void
scene_obstruction_detector<PixType,FeatureType>
::output_classifier_images( const feature_array& features,
                            const image_border& border,
                            const classified_image& output )
{
  std::string frame_id = boost::lexical_cast<std::string>( frame_counter_ - 1 );

  // Output classification images
  double max = 0.0, min = 0.0;
  std::vector< vil_image_view<double> > classifier_image_list;
  for( unsigned i = 0; i < features.size(); i++ )
  {
    vil_image_view<double> clfied;
    initial_classifier_.generate_weight_image( features[i], clfied, i );
    classifier_image_list.push_back( clfied );
    double lmax, lmin;
    vil_image_view< double > reg = classifier_image_list[i];
    if( border.area() > 0 )
    {
      reg = point_view_to_region( classifier_image_list[i], border );
    }
    vil_math_value_range( reg, lmin, lmax );
    if( max < lmax )
    {
      max = lmax;
    }
    if( min > lmin )
    {
      min = lmin;
    }
  }
  double abs_max = std::max( -min, max );
  abs_max *= 0.5;
  for( unsigned i = 0; i < features.size(); i++ )
  {
    std::string id = boost::lexical_cast<std::string>(i);
    std::string fn = "frame_" + frame_id + "_pclassification_" + id + ".png";
    print_scaled_classifier_image( fn, classifier_image_list[i], abs_max );
  }

  // Output classification and averaged images
  std::string ini_fn = "frame_" + frame_id + "_pclassification_net_ini.png";
  std::string avg_fn = "frame_" + frame_id + "_pclassification_net_avg.png";
  vil_image_view<double> temp = vil_plane(output, 1);
  print_scaled_classifier_image( ini_fn, temp, abs_max );
  temp = vil_plane(output, 0);
  print_scaled_classifier_image( avg_fn, temp, abs_max );
}


} // end namespace vidtk
