/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "deep_copy_image_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vil/vil_convert.h>
#include <vil/vil_new.h>

namespace vidtk
{


template <class PixType>
deep_copy_image_process<PixType>
::deep_copy_image_process( vcl_string const& name )
  : process( name, "deep_copy_image_process" ),
    deep_copy_( true ),
    force_component_order_( false )
{
  config_.add( "deep_copy", "true" );
  config_.add( "force_component_order", "false" );
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
    blk.get( "deep_copy", deep_copy_ );
    blk.get( "force_component_order", force_component_order_ );
  }
  catch( unchecked_return_value& )
  {
    // reset to old values
    this->set_params( this->config_ );
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
    log_info( "Input image not supplied.\n" );
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
    log_warning( this->name() << ": Configured to make a shallow image copy.\n" );
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
vil_image_view<PixType> const&
deep_copy_image_process<PixType>
::image() const
{
  return out_img_;
}


} // end namespace vidtk
