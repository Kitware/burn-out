/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "greyscale_process.h"

#include <vil/vil_convert.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_greyscale_process_txx__
VIDTK_LOGGER("greyscale_process_txx");


namespace vidtk
{


template <class PixType>
greyscale_process<PixType>
::greyscale_process( std::string const& _name )
  : process( _name, "greyscale_process" ),
    disabled_( false )
{
  config_.add_parameter( "disabled",
                         "false",
                         "Do not perform any operation on input and pass it along unchanged." );
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
    disabled_ = blk.get<bool>( "disabled" );
    config_.update( blk );
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }

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
  LOG_ASSERT( in_img_ != NULL, "Input image not set" );

  if( disabled_ || in_img_->nplanes() == 1 )
  {
    out_img_ = *in_img_;
  }
  else if( in_img_->nplanes() == 3 )
  {
    vil_convert_planes_to_grey( *in_img_, out_img_ );
  }
  else
  {
    LOG_AND_DIE( "Not implemented for " << in_img_->nplanes()
                 << " planes" );
  }

  // Perform deep copy to new output image for async mode
  copied_out_img_ = vil_image_view< PixType >();
  copied_out_img_.deep_copy( out_img_ );

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
vil_image_view<PixType>
greyscale_process<PixType>
::image() const
{
  return out_img_;
}


template <class PixType>
vil_image_view<PixType>
greyscale_process<PixType>
::copied_image() const
{
  return copied_out_img_;
}


} // end namespace vidtk
