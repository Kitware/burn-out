/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "deep_copy_image_process.h"

#include <vil/vil_convert.h>
#include <vil/vil_new.h>

#include <logger/logger.h>

VIDTK_LOGGER("deep_copy_image_process_txx");

namespace vidtk
{


template <class PixType>
deep_copy_image_process<PixType>
::deep_copy_image_process( std::string const& _name )
  : process( _name, "deep_copy_image_process" ),
    deep_copy_( true ),
    force_component_order_( false )
{
  config_.add_parameter( "deep_copy", "true", "UNDOCUMENTED" );
  config_.add_parameter( "force_component_order", "false", "UNDOCUMENTED" );
}


template <class PixType>
deep_copy_image_process<PixType>
::~deep_copy_image_process()
{
}


template <class PixType>
config_block
deep_copy_image_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
deep_copy_image_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    deep_copy_ = blk.get<bool>( "deep_copy" );
    force_component_order_ = blk.get<bool>( "force_component_order" );
  }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
deep_copy_image_process<PixType>
::initialize()
{
  in_img_ = NULL;

  return true;
}


template <class PixType>
bool
deep_copy_image_process<PixType>
::step()
{
  if( in_img_ == NULL )
  {
    LOG_INFO( "Input image not supplied." );
    return false;
  }

  if( deep_copy_ )
  {
    if( force_component_order_ &&
        in_img_->planestep() != 1 )
    {
      // the force format conversion routine is only written for resources
      vil_image_resource_sptr res = vil_new_image_resource_of_view( *in_img_ );
      out_img_ = vil_image_view<PixType>(
        vil_convert_to_component_order(
          res->get_view() ) );
    }
    else
    {
      out_img_ = vil_image_view<PixType>();
      out_img_.deep_copy( *in_img_ );
    }
  }
  else // if not deep copy
  {
    LOG_WARN( this->name() << ": Configured to make a shallow image copy." );
    out_img_ = *in_img_;
  }

  // record that we've processed this input image
  in_img_ = NULL;

  return true;
}


template <class PixType>
void
deep_copy_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  in_img_ = &img;
}


template <class PixType>
vil_image_view<PixType>
deep_copy_image_process<PixType>
::image() const
{
  return out_img_;
}


} // end namespace vidtk
