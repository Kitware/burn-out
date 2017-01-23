/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "salient_region_classifier_process.h"

#include <object_detectors/maritime_salient_region_classifier.h>
#include <object_detectors/groundcam_salient_region_classifier.h>

#include <utilities/config_block.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "salient_region_classifier_process" );

// Process Definition
template <typename PixType, typename FeatureType>
salient_region_classifier_process<PixType, FeatureType>
::salient_region_classifier_process( std::string const& _name )
  : process( _name, "salient_region_classifier_process" ),
    disabled_( false )
{
  // Operating Mode Parameters
  config_.add_parameter( "disabled",
                         "false",
                         "Is this process disabled?" );
  config_.add_parameter( "mode",
                         "groundcam",
                         "The type of the salient object detector to use. Current options: "
                         "maritime, groundcam" );
  config_.add_parameter( "classifier_filename",
                         "",
                         "If set, will use a secondary pixel level classifier on top of "
                         "every pixel in the inputted saliency mask. Used for maritime mode "
                         "only." );
}


template <typename PixType, typename FeatureType>
salient_region_classifier_process<PixType, FeatureType>
::~salient_region_classifier_process()
{
}


template <typename PixType, typename FeatureType>
config_block
salient_region_classifier_process<PixType, FeatureType>
::params() const
{
  return config_;
}


template <typename PixType, typename FeatureType>
bool
salient_region_classifier_process<PixType, FeatureType>
::set_params( config_block const& blk )
{
  std::string mode, classifier;

  try
  {
    disabled_ = blk.get<bool>("disabled");

    if( !disabled_ )
    {
      mode = blk.get<std::string>("mode");
      classifier = blk.get<std::string>("classifier_filename");

      if( mode == "maritime" )
      {
        maritime_classifier_settings settings;
        settings.use_ooi_pixel_classifier_ = classifier.size() > 0;
        settings.pixel_classifier_filename_ = classifier;
        classifier_.reset( new maritime_salient_region_classifier<PixType,FeatureType>( settings ) );
      }
      else if( mode == "groundcam" )
      {
        groundcam_classifier_settings settings;
        settings.use_pixel_classifier_ = classifier.size() > 0;
        settings.pixel_classifier_filename_ = classifier;
        classifier_.reset( new groundcam_salient_region_classifier<PixType,FeatureType>( settings ) );
      }
      else
      {
        throw config_block_parse_error( "Invalid mode specified: " + mode );
      }
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  // Set internal config block
  config_.update( blk );

  return true;
}


template <typename PixType, typename FeatureType>
bool
salient_region_classifier_process<PixType, FeatureType>
::initialize()
{
  return true;
}


template <typename PixType, typename FeatureType>
bool
salient_region_classifier_process<PixType, FeatureType>
::step()
{
  if( disabled_ )
  {
    output_fg_.fill( false );
    output_weights_.fill( 0.0f );
    return true;
  }

  LOG_ASSERT( classifier_ != NULL, "No valid detector set" );

  if( !input_image_ )
  {
    LOG_ERROR( "Received invalid input image" );
    return false;
  }

  output_fg_ = vil_image_view<bool>( input_image_.ni(), input_image_.nj() );
  output_weights_ = vil_image_view<float>( input_image_.ni(), input_image_.nj() );

  // Note: note all of these inputs need be checked, as only ones which are
  // required will be used, and the others are option.
  classifier_->process_frame( input_image_,
                              saliency_map_,
                              saliency_mask_,
                              features_,
                              output_weights_,
                              output_fg_,
                              border_,
                              obstruction_mask_ );

  return true;
}

// ------------------ Pipeline Accessor Functions ----------------------

template <typename PixType, typename FeatureType>
void salient_region_classifier_process<PixType, FeatureType>
::set_input_image( vil_image_view<PixType> const& src )
{
  input_image_ = src;
}

template <typename PixType, typename FeatureType>
void salient_region_classifier_process<PixType, FeatureType>
::set_saliency_map( vil_image_view<double> const& src )
{
  saliency_map_ = src;
}

template <typename PixType, typename FeatureType>
void salient_region_classifier_process<PixType, FeatureType>
::set_saliency_mask( vil_image_view<bool> const& src )
{
  saliency_mask_ = src;
}

template <typename PixType, typename FeatureType>
void salient_region_classifier_process<PixType, FeatureType>
::set_obstruction_mask( vil_image_view<bool> const& src )
{
  obstruction_mask_ = src;
}

template <typename PixType, typename FeatureType>
void salient_region_classifier_process<PixType, FeatureType>
::set_input_features( feature_array const& array )
{
  features_ = array;
}

template <typename PixType, typename FeatureType>
void salient_region_classifier_process<PixType, FeatureType>
::set_border( vgl_box_2d<int> const& rect )
{
  border_ = rect;
}

template <typename PixType, typename FeatureType>
vil_image_view<float>
salient_region_classifier_process<PixType, FeatureType>
::pixel_saliency() const
{
  return output_weights_;
}

template <typename PixType, typename FeatureType>
vil_image_view<bool>
salient_region_classifier_process<PixType, FeatureType>
::foreground_image() const
{
  return output_fg_;
}


} // end namespace vidtk
