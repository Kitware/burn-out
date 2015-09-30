/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//clamp_image_pixel_values_process.txx
#include "clamp_image_pixel_values_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vil/vil_convert.h>
#include <vil/vil_new.h>

namespace vidtk
{

template <class PixType>
clamp_image_pixel_values_process<PixType>
::clamp_image_pixel_values_process( vcl_string const& name )
  : process( name, "clamp_image_pixel_values_process" ),
    disable_( false ),
    min_( 1 ),
    max_( 254 )
{
  config_.add( "disable", "false" );
  config_.add( "min", "1" );
  config_.add( "max", "254" );
}


template <class PixType>
clamp_image_pixel_values_process<PixType>
::~clamp_image_pixel_values_process()
{
}


template <class PixType>
config_block
clamp_image_pixel_values_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
clamp_image_pixel_values_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disable", disable_ );
    blk.get( "min", min_ );
    blk.get( "max", max_ );
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
clamp_image_pixel_values_process<PixType>
::initialize()
{
  in_img_ = NULL;

  return true;
}


template <class PixType>
bool
clamp_image_pixel_values_process<PixType>
::step()
{
  log_assert( in_img_ != NULL, "Input image not set" );
  out_img_ = *in_img_;//shallow copy

  if( !disable_ )
  {
    for( unsigned int i = 0; i < out_img_.ni(); ++i)
    {
      for( unsigned int j = 0; j < out_img_.nj(); ++j )
      {
        for( unsigned int p = 0; p < out_img_.nplanes(); ++p )
        {
          PixType & pix = out_img_(i,j,p);
          if(pix<min_) pix = min_;
          else if(pix > max_) pix = max_;
        }
      }
    }
  }

  // record that we've processed this input image
  in_img_ = NULL;

  return true;
}


template <class PixType>
void
clamp_image_pixel_values_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  in_img_ = &img;
}


template <class PixType>
vil_image_view<PixType> const&
clamp_image_pixel_values_process<PixType>
::image() const
{
  return out_img_;
}

}
