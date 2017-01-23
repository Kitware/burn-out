/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "blob_pixel_feature_extraction_process.h"

#include <utilities/config_block.h>

#include <logger/logger.h>

#include <boost/lexical_cast.hpp>

namespace vidtk
{


VIDTK_LOGGER("blob_pixel_feature_extraction_process");


template <typename FeatureType>
blob_pixel_feature_extraction_process<FeatureType>
::blob_pixel_feature_extraction_process( std::string const& _name )
  : process( _name, "blob_pixel_feature_extraction_process" ),
    src_image_( NULL )
{
  config_.merge( settings_.config() );
}


template <typename FeatureType>
blob_pixel_feature_extraction_process<FeatureType>
::~blob_pixel_feature_extraction_process()
{
}


template <typename FeatureType>
config_block
blob_pixel_feature_extraction_process<FeatureType>
::params() const
{
  return config_;
}


template <typename FeatureType>
bool
blob_pixel_feature_extraction_process<FeatureType>
::set_params( config_block const& blk )
{
  try
  {
     settings_.read_config( blk );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <typename FeatureType>
bool
blob_pixel_feature_extraction_process<FeatureType>
::initialize()
{
  return true;
}

template <typename FeatureType>
bool
blob_pixel_feature_extraction_process<FeatureType>
::step()
{
  // Validate inputs
  if( !src_image_ )
  {
    LOG_ERROR( this->name() << ": Invalid input!" );
    return false;
  }

  // Reset output image
  output_image_ = vil_image_view<FeatureType>();

  extract_blob_pixel_features( *src_image_,
                               settings_,
                               output_image_ );

  // Reset inputs
  src_image_ = NULL;
  return true;
}


template <typename FeatureType>
void
blob_pixel_feature_extraction_process<FeatureType>
::set_source_image( vil_image_view<bool> const& src )
{
  src_image_ = &src;
}


template <typename FeatureType>
vil_image_view<FeatureType>
blob_pixel_feature_extraction_process<FeatureType>
::feature_image() const
{
  return output_image_;
}


} // end namespace vidtk
