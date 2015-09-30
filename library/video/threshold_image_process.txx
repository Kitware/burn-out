/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "threshold_image_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <utilities/config_block.h>
#include <vil/algo/vil_threshold.h>

namespace vidtk
{


template <typename PixType>
threshold_image_process<PixType>
::threshold_image_process( vcl_string const& name )
  : process( name, "threshold_image_process" ),
    src_image_( NULL ),
    method_( ABOVE ),
    persist_output_( true )
{
  config_.add_parameter( "threshold", "The threshold value to use" );
  config_.add_parameter( "persist_output",
                         "false",
                         "If set, this process will return the last good mask if a source"
                         " image was not supplied" );
}


template <typename PixType>
threshold_image_process<PixType>
::~threshold_image_process()
{
}


template <typename PixType>
config_block
threshold_image_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
threshold_image_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "threshold", threshold_ );
    blk.get( "persist_output", persist_output_ );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  config_.update( blk );
  return true;
}


template <typename PixType>
bool
threshold_image_process<PixType>
::initialize()
{
  return true;
}


template <typename PixType>
bool
threshold_image_process<PixType>
::step()
{
  if( ! src_image_ )
  {
    if( persist_output_ )
    {
      return true;
    }
    else
    {
      log_error( this->name() << ": no source image, and not asked to persist!\n" );
      return false;
    }
  }
  else
  {
    switch( method_ )
    {
      case ABOVE:
        vil_threshold_above( *src_image_, output_image_, threshold_ );
        break;
    }

    // Mark the image as "used", so that we don't try to use it again
    // the next time.
    src_image_ = NULL;

    return true;
  }
}


template <typename PixType>
void
threshold_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& src )
{
  src_image_ = &src;
}


template <typename PixType>
vil_image_view<bool> const&
threshold_image_process<PixType>
::thresholded_image() const
{
  return output_image_;
}


} // end namespace vidtk


#define INSTANTIATE_THRESHOLD_IMAGE_PROCESS( PixType )                  \
  template                                                              \
  class vidtk::threshold_image_process< PixType >;
