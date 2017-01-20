/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "scene_obstruction_detector_process.h"

#include <video_transforms/floating_point_image_hash_functions.h>

#include <utilities/config_block.h>
#include <utilities/point_view_to_region.h>

#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <vil/vil_math.h>
#include <vil/algo/vil_threshold.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_gauss_filter.h>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "scene_obstruction_detector_process" );


// Process Definition
template <typename PixType>
scene_obstruction_detector_process<PixType>
::scene_obstruction_detector_process( std::string const& _name )
  : process( _name, "scene_obstruction_detector_process" ),
    reset_buffer_length_( 0 ),
    frames_since_reset_( 0 )
{
  // Operating Mode Parameters
  config_.add_parameter( "initial_threshold",
                         "0.0",
                         "Initial approximation classifier threshold, used when determining "
                         "which pixels likely belong to the mask for determining breaks. This "
                         "unitless value has a possible range which depends on the supplied model.");
  config_.add_parameter( "use_appearance_filter",
                         "false",
                         "True if we want to initially use the appearance-only classifier "
                         "for the first x frames before switching to the main classifier that "
                         "includes temporal features as well." );
  config_.add_parameter( "appearance_frame_count",
                         "8",
                         "Number of frames to use the appearance classifier with before "
                         "switching to the appearance based one." );
  config_.add_parameter( "intial_classifier",
                         "default_initial_mask_classifier",
                         "Relative filepath to intial classifier file." );
  config_.add_parameter( "appearance_classifier",
                         "default_appearance_mask_classifier",
                         "Relative filepath to the appearance classifier file." );
  config_.add_parameter( "use_spatial_prior_feature",
                         "true",
                         "Should we use a spatial prior feature during classification?" );
  config_.add_parameter( "spatial_prior_filename",
                         "",
                         "Spatial prior image filename. If not specified, a grided image will be "
                         "used as the spatial image." );

  // Detection "mask" break parameters
  config_.add_parameter( "enable_mask_break_detection",
                         "true",
                         "Do we want to enable the internal change detection?" );
  config_.add_parameter( "mask_count_history_length",
                         "20",
                         "Number of frames to record the prior mask detection pixel count"
                         "history for." );
  config_.add_parameter( "mask_intensity_history_length",
                         "40",
                         "Number of frames to record mask pixel intensity history for." );
  config_.add_parameter( "count_percent_change_req",
                         "3.0",
                         "Required percent mask pixel count change for triggering a mask break. "
                         "A value of 3 aka (300%) would require a change in detected mask pixels "
                         "to be either 3 times larger, or 3 times smaller than the current moving "
                         "average.");
  config_.add_parameter( "count_std_dev_req",
                         "5.0",
                         "Required mask pixel count change (in std deviations) for a mask break." );
  config_.add_parameter( "min_hist_for_intensity_diff",
                         "30",
                         "Min number of frames required before we can trigger a mask break "
                         "based on changing color characteristics alone." );
  config_.add_parameter( "intensity_diff_req",
                         "90",
                         "Mask est intensity difference from the windowed average to trigger a break" );
  config_.add_parameter( "map_colors_to_nearest_extreme",
                         "false",
                         "Threshold colors to pure black, white, green, or blue only." );
  config_.add_parameter( "map_colors_near_extremes_only",
                         "false",
                         "Threshold colors near pure black, white, green, or blue if they "
                         "are near them." );

  // Adaptive thresholding parameters
  config_.add_parameter( "use_adaptive_thresholding",
                         "false",
                         "True if we want to adjust the averaged classifier output "
                         "based on the frame number to account for initial uncertainty "
                         "in temporal features." );
  config_.add_parameter( "adaptive_pivot_1",
                         "8",
                         "Frame number of the first piecewise pivot." );
  config_.add_parameter( "adaptive_pivot_2",
                         "16",
                         "Frame number of the second piecewise pivot." );
  config_.add_parameter( "interval_1_adjustment",
                         "0.000",
                         "Signed adjustment applied to the classification image if the number "
                         "of frames observed so far is less than the first pivot" );
  config_.add_parameter( "interval_2_adjustment",
                         "0.015",
                         "Signed adjustment applied to the classification image if the number "
                         "of frames observed so far is between the first and second pivot" );
  config_.add_parameter( "interval_3_adjustment",
                         "0.040",
                         "Signed adjustment applied to the classification image if the number "
                         "of frames observed so far is more than the second pivot." );
  config_.add_parameter( "reset_buffer_length",
                         "0",
                         "Number of frames after an obstruction reset to buffer frames for "
                         "before outputting them." );

  // Training Mode Parameters
  config_.add_parameter( "is_training_mode",
                         "false",
                         "Should we save the input features to some file?" );
  config_.add_parameter( "output_feature_images",
                         "false",
                         "Should we output the input feature images to some file?" );
  config_.add_parameter( "output_classifier_images",
                         "false",
                         "Should we output the classification images to some file?" );
  config_.add_parameter( "groundtruth_dir",
                         "",
                         "Directory storing groundtruth for this video." );
  config_.add_parameter( "output_filename",
                         "training_data.dat",
                         "Output file to store the training data in." );

}


template <typename PixType>
scene_obstruction_detector_process<PixType>
::~scene_obstruction_detector_process()
{
}


template <typename PixType>
config_block
scene_obstruction_detector_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
scene_obstruction_detector_process<PixType>
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    // Configure Run Mode
    options_.is_training_mode_ = blk.get<bool>( "is_training_mode" );
    options_.output_feature_image_mode_ = blk.get<bool>( "output_feature_images" );
    options_.output_classifier_image_mode_ = blk.get<bool>( "output_classifier_images" );
    options_.use_spatial_prior_feature_ = blk.get<bool>( "use_spatial_prior_feature" );

    if( options_.use_spatial_prior_feature_ )
    {
      options_.spatial_prior_filename_ = blk.get<std::string>( "spatial_prior_filename" );
    }

    if( options_.is_training_mode_ )
    {
      options_.groundtruth_dir_ = blk.get<std::string>( "groundtruth_dir" );
      options_.output_filename_ = blk.get<std::string>( "output_filename" );

      if( options_.groundtruth_dir_.size() == 0 )
      {
        throw config_block_parse_error( "Groundtruth dir must be specified!" );
      }
    }

    // Load initial classifiers
    if( !options_.is_training_mode_ )
    {
      // Configure classifiers
      options_.initial_threshold_ = blk.get<double>( "initial_threshold" );
      options_.use_appearance_classifier_ = blk.get<bool>( "use_appearance_filter" );
      options_.appearance_frames_ = blk.get<unsigned>( "appearance_frame_count" );
      options_.primary_classifier_filename_ = blk.get<std::string>( "intial_classifier" );

      if( options_.use_appearance_classifier_ )
      {
        options_.appearance_classifier_filename_ = blk.get<std::string>( "appearance_classifier" );
      }
    }

    // Load color-related parameters
    options_.map_colors_to_nearest_extreme_ = blk.get<bool>( "map_colors_to_nearest_extreme" );
    options_.map_colors_near_extremes_only_ = blk.get<bool>( "map_colors_near_extremes_only" );

    // Load adaptive thresholding parameters
    options_.use_adaptive_thresh_ = blk.get<bool>( "use_adaptive_thresholding" );
    if( options_.use_adaptive_thresh_ )
    {
      options_.at_pivot_1_ = blk.get<unsigned>( "adaptive_pivot_1" );
      options_.at_pivot_2_ = blk.get<unsigned>( "adaptive_pivot_2" );
      options_.at_interval_1_adj_ = blk.get<double>( "interval_1_adjustment" );
      options_.at_interval_2_adj_ = blk.get<double>( "interval_2_adjustment" );
      options_.at_interval_3_adj_ = blk.get<double>( "interval_3_adjustment" );
    }

    // Load break detection parameters
    options_.enable_mask_break_detection_ = blk.get<bool>( "enable_mask_break_detection" );
    if( options_.enable_mask_break_detection_ )
    {
      options_.mask_count_history_length_ = blk.get<unsigned>( "mask_count_history_length" );
      options_.mask_intensity_history_length_ = blk.get<unsigned>( "mask_intensity_history_length" );
      options_.count_percent_change_req_ = blk.get<double>( "count_percent_change_req" );
      options_.count_std_dev_req_ = blk.get<double>( "count_std_dev_req" );
      options_.min_hist_for_intensity_diff_ = blk.get<unsigned>( "min_hist_for_intensity_diff" );
      options_.intensity_diff_req_ = blk.get<double>( "intensity_diff_req" );
    }

    // Load other parameters
    reset_buffer_length_ = blk.get<unsigned>( "reset_buffer_length" );
  }
  catch( config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Set internal config block
  config_.update( blk );

  // Configure algorithm
  return detector_.configure( options_ );
}


template <typename PixType>
bool
scene_obstruction_detector_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
process::step_status
scene_obstruction_detector_process<PixType>
::step2()
{
  // Perform Validation of Inputs
  if( !validate_inputs() )
  {
    LOG_ERROR( this->name() << ": Invalid Inputs!" );
    return process::FAILURE;
  }

  // Reset outputs
  output_image_.clear();

  // Call algorithm update function
  detector_.process_frame( rgb_,
                           features_,
                           border_,
                           var_,
                           output_image_,
                           output_properties_ );

  // Reset input variables
  reset_inputs();

  // Handle internal buffering if enabled
  if( reset_buffer_length_ > 1 )
  {
    vil_image_view<double> current_image = output_image_;
    mask_properties_type current_properties = output_properties_;

    if( output_properties_.break_flag_ )
    {
      flush_buffer();
      frames_since_reset_ = 0;
    }

    if( frames_since_reset_++ < reset_buffer_length_ )
    {
      image_buffer_.push_back( current_image );
      property_buffer_.push_back( current_properties );

      return process::NO_OUTPUT;
    }
    else if( !image_buffer_.empty() )
    {
      flush_buffer();
      output_image_ = current_image;
      output_properties_ = current_properties;
    }
  }

  return process::SUCCESS;
}

template <typename PixType>
bool
scene_obstruction_detector_process<PixType>
::validate_inputs()
{
  if( !rgb_ || features_.size() == 0 )
  {
    return false;
  }

  return true;
}

template <typename PixType>
void
scene_obstruction_detector_process<PixType>
::reset_inputs()
{
  rgb_.clear();
  features_.clear();
}

template <typename PixType>
void
scene_obstruction_detector_process<PixType>
::flush_buffer()
{
  if( image_buffer_.empty() )
  {
    return;
  }

  vil_image_view<double> last_plane = vil_plane( image_buffer_.back(), 0 );

  for( unsigned i = 0; i < image_buffer_.size(); ++i )
  {
    output_image_ = image_buffer_[i];
    output_properties_ = property_buffer_[i];

    if( i != image_buffer_.size() - 1 )
    {
      vil_image_view<double> output_plane = vil_plane( output_image_, 0 );
      vil_copy_reformat( last_plane, output_plane );
    }

    push_output( process::SUCCESS );
  }

  image_buffer_.clear();
  property_buffer_.clear();
}


// ------------------ Pipeline Accessor Functions ----------------------

template <typename PixType>
void
scene_obstruction_detector_process<PixType>
::set_border( vgl_box_2d<int> const& rect )
{
  border_ = rect;
}

template <typename PixType>
void
scene_obstruction_detector_process<PixType>
::set_color_image( vil_image_view<PixType> const& src )
{
  rgb_ = src;
}

template <typename PixType>
void
scene_obstruction_detector_process<PixType>
::set_variance_image( vil_image_view<double> const& src )
{
  var_ = src;
}

template <typename PixType>
void
scene_obstruction_detector_process<PixType>
::set_input_features( feature_array const& array )
{
  features_ = array;
}

template <typename PixType>
vil_image_view<double>
scene_obstruction_detector_process<PixType>
::classified_image() const
{
  return output_image_;
}

template <typename PixType>
scene_obstruction_properties<PixType>
scene_obstruction_detector_process<PixType>
::mask_properties() const
{
  return output_properties_;
}


} // end namespace vidtk
