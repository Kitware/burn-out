/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "extract_image_resolution_process.h"

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_extract_image_resolution_process_txx__
VIDTK_LOGGER("extract_image_resolution_process_txx");

namespace vidtk
{


template <class PixType>
extract_image_resolution_process<PixType>
::extract_image_resolution_process( std::string const& _name )
  : process( _name, "extract_image_resolution_process" )
{
}


template <class PixType>
extract_image_resolution_process<PixType>
::~extract_image_resolution_process()
{
}


template <class PixType>
config_block
extract_image_resolution_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
extract_image_resolution_process<PixType>
::set_params( config_block const& blk )
{
  config_.update( blk );
  return true;
}


template <class PixType>
bool
extract_image_resolution_process<PixType>
::initialize()
{
  input_image_ = vil_image_view<PixType>();

  return true;
}


template <class PixType>
bool
extract_image_resolution_process<PixType>
::step()
{
  if ( !input_image_ )
  {
    LOG_ERROR( this->name() << ": Input image not set" );
    return false;
  }

  resolution_ = resolution_t(input_image_.ni(), input_image_.nj());

  input_image_ = vil_image_view<PixType>();

  return true;
}


template <class PixType>
void
extract_image_resolution_process<PixType>
::set_image( vil_image_view<PixType> const& img )
{
  input_image_ = img;
}


template <class PixType>
typename extract_image_resolution_process<PixType>::resolution_t
extract_image_resolution_process<PixType>
::resolution() const
{
  return resolution_;
}


} // end namespace vidtk
