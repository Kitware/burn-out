/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "draw_on_image_process.h"

#include <video_transforms/draw_text.h>

#include <sstream>

#include <logger/logger.h>

VIDTK_LOGGER("draw_on_image_process_txx");

namespace vidtk
{

template< class PixType >
draw_on_image_process< PixType >
::draw_on_image_process( std::string const& _name )
: process( _name, "draw_on_image_process" ),
  disabled_( true )
{
  config_.add_parameter( "disabled",
                         "true",
                         "Whether to change the input frame or not?" );
}

template< class PixType >
draw_on_image_process< PixType >
::~draw_on_image_process()
{
}

template< class PixType >
config_block
draw_on_image_process< PixType >
::params() const
{
  return this->config_;
}

template< class PixType >
bool
draw_on_image_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    this->disabled_ = blk.get<bool>( "disabled" );
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": failed to set parameters, "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}

template< class PixType >
bool
draw_on_image_process< PixType >
::initialize()
{
  out_img_.clear();
  return true;
}

template< class PixType >
bool
draw_on_image_process< PixType >
::step()
{
  if( !in_img_ )
  {
    LOG_ERROR( this->name() << ": Input image not provided." );
    return false;
  }

  // If a deep copy is required, then an explict deep copy process
  // should be added before this image.
  out_img_ = *in_img_;

  if( disabled_ )
  {
    return true;
  }

  /// Now you can draw various pieces of information, one by one.

  // GSD
  draw_text< PixType > gsd_writer;

  std::stringstream ss;
  ss << "GSD: " << gsd_;

  // TODO: Make these config parameters?
  unsigned gsd_i = 30;
  unsigned gsd_j = 10;
  unsigned color[3] = {0,255,255};

  gsd_writer.draw_message( out_img_,
                           ss.str(),
                           gsd_i, gsd_j,
                           color );
  return true;
}

template < class PixType >
void
draw_on_image_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  in_img_ = &img;
}

template < class PixType >
void
draw_on_image_process< PixType >
::set_world_units_per_pixel( double gsd )
{
  gsd_ = gsd;
}

template < class PixType >
vil_image_view< PixType >
draw_on_image_process< PixType >
::image() const
{
  return out_img_;
}

} // end namespace vidtk
