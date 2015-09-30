/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "greyscale_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vil/vil_convert.h>

namespace vidtk
{


template <class PixType>
greyscale_process<PixType>
::greyscale_process( vcl_string const& name )
  : process( name, "greyscale_process" )
{
}


template <class PixType>
greyscale_process<PixType>
::~greyscale_process()
{
}


template <class PixType>
config_block
greyscale_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
greyscale_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
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
greyscale_process<PixType>
::initialize()
{
  return true;
}


template <class PixType>
bool
greyscale_process<PixType>
::step()
{
  log_assert( in_img_ != NULL, "Input image not set" );

  if( in_img_->nplanes() == 1 )
  {
    out_img_ = *in_img_;
  }
  else if( in_img_->nplanes() == 3 )
  {
    vil_convert_planes_to_grey( *in_img_, out_img_ );
  }
  else
  {
    log_fatal_error( "Not implemented for " << in_img_->nplanes()
                     << " planes\n" );
  }

  return true;
}


template <class PixType>
void
greyscale_process<PixType>
::set_image( vil_image_view<PixType> const& img )
{
  in_img_ = &img;
}


template <class PixType>
vil_image_view<PixType> const&
greyscale_process<PixType>
::image() const
{
  return out_img_;
}


template <class PixType>
vil_image_view<PixType> const&
greyscale_process<PixType>
::copied_image() const
{
  // Reset output image
  copied_out_img_ = vil_image_view< PixType >();
  copied_out_img_.deep_copy( out_img_ );

  return copied_out_img_;
}


} // end namespace vidtk
