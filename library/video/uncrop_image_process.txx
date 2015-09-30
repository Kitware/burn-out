
/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "uncrop_image_process.h"
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>
#include <vil/vil_crop.h>

namespace vidtk
{


template <class PixType>
uncrop_image_process<PixType>
::uncrop_image_process( vcl_string const& name )
  : process( name, "uncrop_image_process" )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Disable this process, causing the image to pass through unmodified" );

  config_.add_parameter(
    "horizontal_padding",
    "0",
    "Number of pixels to be added to the padding on each (right and left) "
    "side of the image. Symmetric padding will be produced by this process." );

  config_.add_parameter(
    "vertical_padding",
    "0",
    "Number of pixels to be added to the padding on each (top and bottom) "
    "side of the image. Symmetric padding will be produced by this process." );

  config_.add_parameter( 
    "background_value",
    "0.0",
    "Pixel value to be used for the padded pixels. This value will be directly "
    "typecasted onto the pixel data type (PixType)." );
}


template <class PixType>
uncrop_image_process<PixType>
::~uncrop_image_process()
{
}


template <class PixType>
config_block
uncrop_image_process<PixType>
::params() const
{
  return config_;
}

template <class PixType>
bool
uncrop_image_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", disabled_ );
    blk.get( "horizontal_padding", horizontal_pad_ );
    blk.get( "vertical_padding", vertical_pad_ );
    blk.get( "background_value", bkgnd_value_ );
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


template <class PixType>
bool
uncrop_image_process<PixType>
::initialize()
{
  return true;
}


template <class PixType>
bool
uncrop_image_process<PixType>
::step()
{
  if( input_image_ == NULL )
  {
    log_error( this->name() << ": Input image not available." );
    return false;
  }

  if( disabled_ || ( horizontal_pad_ == 0 && vertical_pad_ == 0 ) )
  {
    uncropped_image_ = *input_image_;
  }
  else
  {
    if( !uncropped_image_ ) 
    {
      // Assuming that source image size remains constant, we will only perform
      // following tasks only once for the first frame. 
      uncropped_image_.set_size( input_image_->ni() + 2*horizontal_pad_, 
                                 input_image_->nj() + 2*vertical_pad_, 
                                 input_image_->nplanes() );
      
      uncropped_image_.fill( bkgnd_value_ );

      cropped_view_ = vil_crop( uncropped_image_,
                               horizontal_pad_,
                               input_image_->ni(),
                               vertical_pad_,
                               input_image_->nj() );
    }

    cropped_view_.deep_copy( *input_image_ );
  }

  return true;
}


template <class PixType>
void
uncrop_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  input_image_ = &img;
}


template <class PixType>
vil_image_view<PixType> const&
uncrop_image_process<PixType>
::uncropped_image() const
{
  return uncropped_image_;
}


} // end namespace vidtk
