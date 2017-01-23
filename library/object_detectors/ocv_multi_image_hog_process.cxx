/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ocv_multi_image_hog_process.h"

// Other VIDTK
#include <utilities/config_block.h>
#include <utilities/video_modality.h>
#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER ("ocv_multi_image_hog_process");

// Process Definition
ocv_multi_image_hog_process
::ocv_multi_image_hog_process( std::string const& _name )
 : process( _name, "ocv_multi_image_hog_process" ),
   disabled_( false )
{
  config_.add_parameter( "disabled",
                         "false",
                         "Completely disable this process and pass no outputs." );

  config_.merge( ocv_hog_detector_settings().config() );
}


// ----------------------------------------------------------------
ocv_multi_image_hog_process
::~ocv_multi_image_hog_process()
{
}


// ----------------------------------------------------------------
config_block
ocv_multi_image_hog_process
::params() const
{
  return config_;
}


// ----------------------------------------------------------------
bool
ocv_multi_image_hog_process
::set_params( config_block const& blk )
{
  // Load internal settings
  try
  {
    LOG_INFO( this->name() << ": Loading Parameters" );

    disabled_ = blk.get<bool>( "disabled" );

    if( disabled_ )
    {
      return true;
    }

    ocv_hog_detector_settings dpm_options;
    dpm_options.read_config( blk );

    if( !detector_.configure( dpm_options ) )
    {
      throw config_block_parse_error("Unable to configure ocv dpm model classifier.");
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



// ----------------------------------------------------------------
bool
ocv_multi_image_hog_process
::initialize()
{
  return true;
}


// ----------------------------------------------------------------
bool
ocv_multi_image_hog_process
::step()
{
  image_objects_.clear();

  if( disabled_ )
  {
    return true;
  }

  for( unsigned int i = 0; i < images_.size(); ++i )
  {
    std::vector<image_object_sptr> objs = detector_.detect( images_[i] );
    for( unsigned int j = 0; j < objs.size(); ++j )
    {
      image_objects_.insert( image_objects_.end(), objs.begin(), objs.end() );
    }
  }

  return true;
}


// ----------------------------------------------------------------
// -- INPUTS --
void
ocv_multi_image_hog_process
::set_source_imagery( std::vector<vil_image_view<vxl_byte> > const& images )
{
  images_ = images;
}

void
ocv_multi_image_hog_process
::set_offset_scales( std::vector<vnl_double_3> const& offset_scales)
{
  offset_scales_ = offset_scales;
}


// -- OUTPUTS --
std::vector<image_object_sptr>
ocv_multi_image_hog_process
::image_objects() const
{
  return image_objects_;
}


} // end namespace vidtk
