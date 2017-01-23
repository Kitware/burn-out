/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_reader_process.h"

#include <vil/vil_load.h>
#include <vil/vil_convert.h>

#include <logger/logger.h>

namespace vidtk {

VIDTK_LOGGER( "mask_reader_process" );

// ----------------------------------------------------------------
/**
 * @brief Constructor.
 *
 *
 */
mask_reader_process::
mask_reader_process( std::string const& n )
  : process( n, "mask_reader_process" )
{
  this->config_.add_parameter( "disabled",
                               "true",
                               "If enabled, will reset on video modality change. "
                               "If disabled, will pass all inputs to outputs." );
  this->config_.add_parameter( "file", "",
                               "Name of the mask file to read" );
}


mask_reader_process::
~mask_reader_process()
{

}


// ----------------------------------------------------------------
/**
 * @brief Collect parameters from this process.
 *
 * This method returns a config block that contains all the
 * parameters needed by this process.
 *
 * @return Config block with all parameters.
 */
config_block
mask_reader_process::
params() const
{
  return this->config_;
}


// ----------------------------------------------------------------
/**
 * @brief Set values for parameters.
 *
 * This method is called with an updated config block that
 * contains the most current configuration values. We will
 * query our parameters from this block.
 *
 * @param[in] blk - updated config block
 */
bool
mask_reader_process::
set_params( config_block const& blk )
{
  try
  {
    this->disabled_ =  blk.get< bool > ( "disabled" );
  }
  catch ( config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );

    return false;
  }

  this->config_ = blk; // save the config block
  return true;
}


// ----------------------------------------------------------------
/**
 * @brief Initialize this process.
 *
 * This initialize method is called once after the parameters
 * have been supplied. It is our duty to initialize this process
 * and make it ready to operate.
 */
bool
mask_reader_process::
initialize()
{
  std::string filename = this->config_.get< std::string > ( "file" );

  vil_image_resource_sptr local_image = vil_load_image_resource( filename.c_str() );
  if ( ! local_image )
  {
    LOG_ERROR( name() << ": couldn't load mask image from file \""
               << filename << "\""  );
    return false;
  }

  // This is a simple cast to bool. If a more complicated conversion
  // is needed, this is the place to do it.
  this->mask_image_ = vil_convert_cast( bool(), local_image->get_view() );

  return this->mask_image_; // return if valid image
}


// ----------------------------------------------------------------
/**
 * @brief Main processing method.
 *
 * This method is called once per iteration of the pipeline.
 */
bool
mask_reader_process::
step()
{
  return this->mask_image_; // return if image is valid
}


// ================================================================
// Output methods
vil_image_view< bool >
mask_reader_process::
mask_image() const
{
  return this->mask_image_;
}

} // end namespace
