/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "diff_pixel_feature_infusion_process.h"

#include <utilities/point_view_to_region.h>

#include <video_properties/border_detection.h>

#include <vil/vil_math.h>
#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>

#include <vgl/vgl_intersection.h>

#include <boost/lexical_cast.hpp>

#include <cassert>
#include <algorithm>

#include <logger/logger.h>


VIDTK_LOGGER("diff_pixel_feature_infusion_process_txx");

namespace vidtk
{

template <class PixType>
diff_pixel_feature_infusion_process<PixType>
::diff_pixel_feature_infusion_process( std::string const& _name )
  : process( _name, "diff_pixel_feature_infusion_process" )
  , classifier_( new classifier_t() ),
    frame_counter_(0)
{
  // Operating Mode Parameters
  config_.add_parameter( "classifier",
    "",
    "Relative filepath to the classifier model file." );
  config_.add_parameter( "motion_image_crop",
    "0 0",
    "How much the motion image was cropped" );
  config_.add_parameter( "border_fill_value",
    "-100",
    "Fill value for pixels outside the image border" );
  config_.add_parameter( "model_identifier",
    "object_pixel_model",
    "Global identifier for the internal model to be "
    "used for the cross-process resource manager." );

  // Training Mode Parameters
  config_.add_parameter( "is_training_mode",
    "false",
    "Should we save the input features to some file?" );
  config_.add_parameter( "feature_output_folder",
    "",
    "If non-empty, a folder to output feature images to" );
  config_.add_parameter( "groundtruth_dir",
    "",
    "Directory storing groundtruth for this video." );
  config_.add_parameter( "groundtruth_pattern",
    "gt-%d.png",
    "Pattern for groundtruth for this video." );
  config_.add_parameter( "output_filename",
    "training_data.dat",
    "Output file to store the training data in." );
}


template <class PixType>
diff_pixel_feature_infusion_process<PixType>
::~diff_pixel_feature_infusion_process()
{
}


template <class PixType>
config_block
diff_pixel_feature_infusion_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
diff_pixel_feature_infusion_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    // Configure run mode
    is_training_mode_ = blk.get<bool>( "is_training_mode" );
    feature_output_folder_ = blk.get<std::string>( "feature_output_folder" );
    border_fill_value_ = blk.get<weight_t>( "border_fill_value" );
    output_feature_images_ = !feature_output_folder_.empty();

    blk.get( "motion_image_crop", motion_image_crop_ );

    if( is_training_mode_ )
    {
      feature_writer_.reset( new pixel_feature_writer< PixType >() );

      pixel_feature_writer_settings trainer_options;
      trainer_options.gt_loader.path = blk.get<std::string>( "groundtruth_dir" );
      trainer_options.gt_loader.pattern = blk.get<std::string>( "groundtruth_pattern" );
      trainer_options.output_file = blk.get<std::string>( "output_filename" );
      trainer_options.gt_loader.load_mode = pixel_annotation_loader_settings::DIRECTORY;

      if( !feature_writer_->configure( trainer_options ) )
      {
        throw config_block_parse_error( "Unable to configure feature writer" );
      }
    }
    else
    {
      ossr_classifier_settings classifier_options;

      classifier_options.seed_model_fn = blk.get<std::string>( "classifier" );
      classifier_options.use_external_thread = true;

      if( !classifier_->configure( classifier_options ) )
      {
        throw config_block_parse_error( "Unable to configure classifier" );
      }
    }

    // Set global pixel classifier resource
    std::string model_identifier = blk.get<std::string>( "model_identifier" );
    resource_trait< classifier_sptr_t > model_trait( model_identifier );

    this->use_resource( model_trait );

    if( this->resource_available( model_trait ) )
    {
      this->set_resource( model_trait, classifier_ );
    }
    else
    {
      this->create_resource( model_trait, classifier_ );
    }
  }
  catch( config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
diff_pixel_feature_infusion_process<PixType>
::initialize()
{
  return true;
}


template <class PixType>
bool
diff_pixel_feature_infusion_process<PixType>
::reset()
{
  return this->initialize();
}

template <class PixType>
bool
diff_pixel_feature_infusion_process<PixType>
::step()
{
  LOG_ASSERT( input_features_->size() > 0, "No input features provided!" );
  LOG_ASSERT( diff_image_, "Differencing image must be specified" );

  features_sptr_.reset( new pixel_feature_array< pixel_t >() );
  feature_array_t& full_feature_array = features_sptr_->features;
  full_feature_array.resize( input_features_ ->size() );

  unsigned output_ni = diff_image_->ni();
  unsigned output_nj = diff_image_->nj();

  output_ = classified_image_t();
  output_.set_size( output_ni, output_nj );

  image_border_t motion_crop( 0, output_ni, 0, output_nj );

  image_border_t feature_crop( motion_image_crop_[0], motion_image_crop_[0] + output_ni,
                               motion_image_crop_[1], motion_image_crop_[1] + output_nj );

  if( border_ )
  {
    feature_crop = vgl_intersection( feature_crop, *border_ );

    motion_crop = feature_crop;
    motion_crop.set_centroid_x( feature_crop.centroid_x() - motion_image_crop_[0] );
    motion_crop.set_centroid_y( feature_crop.centroid_y() - motion_image_crop_[1] );

    output_ = point_view_to_region( output_, motion_crop );
    fill_border_pixels( output_, motion_crop, border_fill_value_ );
  }

  features_sptr_->region = feature_crop;

  for( unsigned i = 0; i < input_features_->size(); i++ )
  {
    point_view_to_region( (*input_features_)[i], feature_crop, full_feature_array[i] );
  }

  if( diff_mask_ )
  {
    vil_convert_cast( point_view_to_region( *diff_mask_, motion_crop ), new_feature_ );
    full_feature_array.push_back( new_feature_ );
  }

  if( diff_image_ )
  {
    full_feature_array.push_back( point_view_to_region( *diff_image_, motion_crop ) );
  }

  if( output_feature_images_ )
  {
    std::string frame_id = boost::lexical_cast<std::string>( frame_counter_ );
    std::string prefix = feature_output_folder_ + "/frame_" + frame_id + "_feature_";
    write_feature_images( full_feature_array, prefix, ".png", true );
  }

  if( !is_training_mode_ )
  {
    LOG_ASSERT( classifier_->is_valid(), "Internal model is not valid!" );

    if( gfi_.has_frame_signals() )
    {
      classifier_->reset_model( (gfi_.frame_signals()[0]).qid_ );
    }

    classifier_->classify_images( full_feature_array, output_ );
  }
  else
  {
    feature_writer_->step( full_feature_array, feature_crop );
    output_.fill( border_fill_value_ );
  }

  frame_counter_++;
  reset_inputs();
  return true;
}

template <class PixType>
void
diff_pixel_feature_infusion_process<PixType>
::reset_inputs()
{
  input_features_ = NULL;
  diff_mask_ = NULL;
  diff_image_ = NULL;
  border_ = NULL;
}

// I/O Functions

template <class PixType>
void
diff_pixel_feature_infusion_process<PixType>
::set_diff_mask( mask_image_t const& src )
{
  diff_mask_ = &src;
}

template <class PixType>
void
diff_pixel_feature_infusion_process<PixType>
::set_diff_image( motion_image_t const& src )
{
  diff_image_ = &src;
}

template <class PixType>
void
diff_pixel_feature_infusion_process<PixType>
::set_input_features( feature_array_t const& src )
{
  input_features_ = &src;
}

template <class PixType>
void
diff_pixel_feature_infusion_process<PixType>
::set_border( image_border_t const& src )
{
  border_ = &src;
}

template <class PixType>
void
diff_pixel_feature_infusion_process<PixType>
::set_gui_feedback( gui_frame_info const& src )
{
  gfi_ = src;
}

template <class PixType>
vil_image_view< float >
diff_pixel_feature_infusion_process<PixType>
::classified_image() const
{
  return output_;
}

template <class PixType>
typename diff_pixel_feature_infusion_process<PixType>::features_sptr_t
diff_pixel_feature_infusion_process<PixType>
::feature_array() const
{
  return features_sptr_;
}

} // end namespace vidtk
