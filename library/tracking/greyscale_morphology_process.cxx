/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/greyscale_morphology_process.h>
#include <utilities/log.h>

#include <vcl_cassert.h>
#include <vil/algo/vil_greyscale_erode.h>
#include <vil/algo/vil_greyscale_dilate.h>
#include <vil/algo/vil_greyscale_erode.hxx>
#include <vil/algo/vil_greyscale_dilate.hxx>

namespace vidtk
{


greyscale_morphology_process
::greyscale_morphology_process( vcl_string const& name )
  : process( name, "greyscale_morphology_process" ),
    src_img_( NULL ),
    opening_radius_( 0 ),
    closing_radius_( 0 )
{
  config_.add( "opening_radius", "0" );
  config_.add( "closing_radius", "0" );
}


config_block
greyscale_morphology_process
::params() const
{
  return config_;
}


bool
greyscale_morphology_process
::set_params( config_block const& blk )
{
  double o_rad = blk.get<double>( "opening_radius" );
  double c_rad = blk.get<double>( "closing_radius" );

  if( o_rad > 0 && c_rad > 0 )
  {
    log_error( name() << ": can only open or close, not both.\n" );
    return false;
  }

  opening_radius_ = o_rad;
  closing_radius_ = c_rad;
  config_.update( blk );

  return true;
}


bool
greyscale_morphology_process
::initialize()
{
  if( opening_radius_ > 0 )
  {
    opening_el_.set_to_disk( opening_radius_ );
  }

  if( closing_radius_ > 0 )
  {
    closing_el_.set_to_disk( closing_radius_ );
  }

  return true;
}


bool
greyscale_morphology_process
::step()
{
  assert( src_img_ != NULL );

  // We call erode and dilate directly instead of calling open and
  // close because we want to use our own intermediate buffer to avoid
  // reallocating it at every frame.

  if( opening_radius_ > 0 )
  {
    vil_greyscale_erode( *src_img_, buffer_, opening_el_ );
    vil_greyscale_dilate( buffer_, out_img_, opening_el_ );
  }
  else if( closing_radius_ > 0 )
  {
    vil_greyscale_dilate( *src_img_, buffer_, closing_el_ );
    vil_greyscale_erode( buffer_, out_img_, closing_el_ );
  }
  else
  {
    out_img_ = *src_img_;
  }

  return true;
}


void
greyscale_morphology_process
::set_source_image( vil_image_view<vxl_byte> const& img )
{
  src_img_ = &img;
}


vil_image_view<vxl_byte> const&
greyscale_morphology_process
::image() const
{
  return out_img_;
}


} // end namespace vidtk
