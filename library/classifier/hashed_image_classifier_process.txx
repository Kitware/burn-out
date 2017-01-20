/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <classifier/hashed_image_classifier_process.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("hashed_image_classifier_process");


template <typename HashType, typename OutputType>
hashed_image_classifier_process<HashType,OutputType>
::hashed_image_classifier_process( std::string const& _name )
  : process( _name, "hashed_image_classifier_process" ),
    use_variable_models_( false ),
    lower_gsd_threshold_( 0.11 ),
    upper_gsd_threshold_( 0.22 )
{
  config_.add_parameter( "use_variable_models",
                         "false",
                         "Set to true if we should use different models "
                         "for different GSDs and modalities." );
  config_.add_parameter( "lower_gsd_threshold",
                         "0.11",
                         "GSD threshold seperating the lowest from middle "
                         "GSD intervals used with variable model selection." );
  config_.add_parameter( "upper_gsd_threshold",
                         "0.22",
                         "GSD threshold seperating the middle from highest "
                         "GSD intervals used with variable model selection." );
  config_.add_parameter( "default_filename",
                         "",
                         "Filename for the default model to use." );
  config_.add_parameter( "eo_narrow_filename",
                         "",
                         "Model filename for the low gsd eo mode." );
  config_.add_parameter( "eo_medium_filename",
                         "",
                         "Model filename for the low gsd eo mode." );
  config_.add_parameter( "eo_wide_filename",
                         "",
                         "Model filename for the low gsd eo mode." );
  config_.add_parameter( "ir_narrow_filename",
                         "",
                         "Model filename for the low gsd eo mode." );
  config_.add_parameter( "ir_medium_filename",
                         "",
                         "Model filename for the low gsd eo mode." );
  config_.add_parameter( "ir_wide_filename",
                         "",
                         "Model filename for the low gsd eo mode." );
}


template <typename HashType, typename OutputType>
config_block
hashed_image_classifier_process<HashType,OutputType>
::params() const
{
  return config_;
}


template <typename HashType, typename OutputType>
bool
hashed_image_classifier_process<HashType,OutputType>
::set_params( config_block const& blk )
{
  std::string filename;

  try
  {
    use_variable_models_ = blk.get<bool>( "use_variable_models" );

    // Load default model
#define LOAD_MODEL_FILE( MODEL, CONFIG_KEY ) \
    filename = blk.get<std::string>( CONFIG_KEY ); \
    if( filename.size() != 0 ) \
    { \
      if( ! MODEL .load_from_file( filename ) ) \
      { \
        std::string error_msg = "Unable to read model file: " + filename; \
        throw config_block_parse_error( error_msg ); \
      } \
    } \

    LOAD_MODEL_FILE( default_clfr_, "default_filename" );

    if( use_variable_models_ )
    {
      // Load GSD intervals
      lower_gsd_threshold_ = blk.get<double>( "lower_gsd_threshold" );
      upper_gsd_threshold_ = blk.get<double>( "upper_gsd_threshold" );

      // Validate GSD intervals
      if( lower_gsd_threshold_ >= upper_gsd_threshold_ )
      {
        throw config_block_parse_error( "Invalid gsd thresholds" );
      }

      // Load models
      LOAD_MODEL_FILE( eo_n_clfr_, "eo_narrow_filename" );
      LOAD_MODEL_FILE( eo_m_clfr_, "eo_medium_filename" );
      LOAD_MODEL_FILE( eo_w_clfr_, "eo_wide_filename" );
      LOAD_MODEL_FILE( ir_n_clfr_, "ir_narrow_filename" );
      LOAD_MODEL_FILE( ir_m_clfr_, "ir_medium_filename" );
      LOAD_MODEL_FILE( ir_w_clfr_, "ir_wide_filename" );

#undef LOAD_MODEL_FILE
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <typename HashType, typename OutputType>
bool
hashed_image_classifier_process<HashType,OutputType>
::initialize()
{
  this->reset_inputs();
  return true;
}

template <typename HashType, typename OutputType>
bool
hashed_image_classifier_process<HashType,OutputType>
::step()
{
  // Confirm we received an input
  if( !input_features_ )
  {
    LOG_ERROR( this->name() << ": input feature vector not set!" );
    return false;
  }

  // Reset output
  output_img_.clear();
  output_img_ = vil_image_view<OutputType>();

  // Select appropriate classifier
  classifier* to_use = &default_clfr_;

  if( use_variable_models_ )
  {
    if( is_video_modality_eo( input_modality_ ) )
    {
      if( input_gsd_ > upper_gsd_threshold_ )
      {
        to_use = &eo_w_clfr_;
      }
      else if( input_gsd_ > lower_gsd_threshold_ )
      {
        to_use = &eo_m_clfr_;
      }
      else if( input_gsd_ >= 0 )
      {
        to_use = &eo_n_clfr_;
      }
    }
    else
    {
      if( input_gsd_ > upper_gsd_threshold_ )
      {
        to_use = &ir_w_clfr_;
      }
      else if( input_gsd_ > lower_gsd_threshold_ )
      {
        to_use = &ir_m_clfr_;
      }
      else if( input_gsd_ >= 0 )
      {
        to_use = &ir_n_clfr_;
      }
    }
    if( input_gsd_ < 0 )
    {
      LOG_WARN( this->name() << ": valid GSD not specified, using default model." );
    }
  }

  // Perform classification if possible
  if( to_use->is_valid() )
  {
    to_use->classify_images( *input_features_, output_img_, 0.0 );
  }
  else
  {
    LOG_ERROR( this->name() << ": invalid classifier loaded!" );
    this->reset_inputs();
    return false;
  }

  // Reset inputs
  this->reset_inputs();
  return true;
}

// Input and output accessor functions
template <typename HashType, typename OutputType>
void
hashed_image_classifier_process<HashType,OutputType>
::reset_inputs()
{
  input_features_ = NULL;
  input_modality_ = VIDTK_INVALID;
  input_gsd_ = -1;
}


// Input and output accessor functions
template <typename HashType, typename OutputType>
void
hashed_image_classifier_process<HashType,OutputType>
::set_gsd( double gsd )
{
  input_gsd_ = gsd;
}

template <typename HashType, typename OutputType>
void
hashed_image_classifier_process<HashType,OutputType>
::set_modality( vidtk::video_modality vm )
{
  input_modality_ = vm;
}

template <typename HashType, typename OutputType>
void
hashed_image_classifier_process<HashType,OutputType>
::set_pixel_features( input_features const& features )
{
  input_features_ = &features;
}

template <typename HashType, typename OutputType>
vil_image_view<OutputType>
hashed_image_classifier_process<HashType,OutputType>
::classified_image() const
{
  return output_img_;
}


} // end namespace vidtk
