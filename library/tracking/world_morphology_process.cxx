/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/world_morphology_process.h>
#include <utilities/log.h>

#include <vcl_cassert.h>
#include <vil/algo/vil_binary_erode.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vnl/vnl_math.h>

#include <vcl_algorithm.h>

namespace vidtk
{


world_morphology_process
::world_morphology_process( vcl_string const& name )
  : process( name, "world_morphology_process" ),
    src_img_( NULL ),
    is_fg_good_( true ),
    opening_radius_( 0 ),
    closing_radius_( 0 ),
    min_image_opening_radius_( 0 ),
    min_image_closing_radius_( 0 ),
    max_image_opening_radius_( 0 ),
    max_image_closing_radius_( 0 ),
    last_image_closing_radius_( 0 ),
    last_image_opening_radius_( 0 ),
    last_step_world_units_per_pixel_( 1 ),
    world_units_per_pixel_( 1 )
{
  config_.add( "opening_radius", "0" );
  config_.add( "min_image_opening_radius", "0" );
  config_.add( "max_image_opening_radius", "4" );
  config_.add( "closing_radius", "0" );
  config_.add( "min_image_closing_radius", "0" );
  config_.add( "max_image_closing_radius", "4" );
  config_.add( "world_units_per_pixel", "1" );
}


config_block
world_morphology_process
::params() const
{
  return config_;
}


bool
world_morphology_process
::set_params( config_block const& blk )
{
  double o_rad = blk.get<double>( "opening_radius" );
  min_image_opening_radius_ = blk.get<double>( "min_image_opening_radius" );
  max_image_opening_radius_ = blk.get<double>( "max_image_opening_radius" );
  double c_rad = blk.get<double>( "closing_radius" );
  min_image_closing_radius_ = blk.get<double>( "min_image_closing_radius" );
  max_image_closing_radius_ = blk.get<double>( "max_image_closing_radius" );
  double world_units = blk.get<double>( "world_units_per_pixel" );

  if( o_rad > 0 && c_rad > 0 )
  {
    log_error( name() << ": can only open or close, not both.\n" );
    return false;
  }

  if ( fabs( world_units ) < 1e-6 )
  {
    log_error( name() << ": world_units are extremely small!\n");
  }

  opening_radius_ = o_rad;
  closing_radius_ = c_rad;
  world_units_per_pixel_ = world_units;
  config_.update( blk );

  return true;
}


bool
world_morphology_process
::initialize()
{
  // Now handled in step.
  /*
  if( opening_radius_ > 0 )
  {
    opening_el_.set_to_disk( opening_radius_ );
  }

  if( closing_radius_ > 0 )
  {
    closing_el_.set_to_disk( closing_radius_ );
  }
  */
  return true;
}


bool
world_morphology_process
::step()
{
  assert( src_img_ != NULL );
  
  if( !is_fg_good_ ) 
    return true;

  // We need to update the structuring element if the set_world_units_per_pixel
  // changed since last run.
  
  // Structuring element will take a double for the radius. 
  double image_opening_radius = opening_radius_ / world_units_per_pixel_;
  double image_closing_radius = closing_radius_ / world_units_per_pixel_;

  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS computed image_opening_radius : " << image_opening_radius << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS computed image_closing_radius : " << image_closing_radius << vcl_endl;

  if ( last_image_opening_radius_ != image_opening_radius && 
        opening_radius_ > 0 )
  {
    double rad = vcl_max( min_image_opening_radius_, image_opening_radius );
    rad = vcl_min( max_image_opening_radius_, rad );
#ifdef PRINT_DEBUG_INFO
    log_debug( this->name() << ": set opening radius: " << rad << "\n" );
#endif
    opening_el_.set_to_disk( rad );
    last_image_opening_radius_ = image_opening_radius;
  }
  if ( last_image_closing_radius_ != image_closing_radius && 
        closing_radius_ > 0 )
  {
    double rad = vcl_max( min_image_closing_radius_, image_closing_radius );
    rad = vcl_min( max_image_closing_radius_, rad );
#ifdef PRINT_DEBUG_INFO
    log_debug( this->name() << ": set closing radius: " << rad << "\n" );
#endif
    closing_el_.set_to_disk( rad );
    last_image_closing_radius_ = image_closing_radius;
  }

  // Debug 
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS GSD : " << world_units_per_pixel_ << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS OPENING max i : " << opening_el_.max_i() << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS OPENING min i : " << opening_el_.min_i() << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS OPENING max j : " << opening_el_.max_j() << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS OPENING min j : " << opening_el_.min_j() << vcl_endl;

  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS CLOSING max i : " << closing_el_.max_i() << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS CLOSING min i : " << closing_el_.min_i() << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS CLOSING max j : " << closing_el_.max_j() << vcl_endl;
  //vcl_cerr << "WORLD_MORPHOLOGY_PROCESS CLOSING min j : " << closing_el_.min_j() << vcl_endl;

  // We call erode and dilate directly instead of calling open and
  // close because we want to use our own intermediate buffer to avoid
  // reallocating it at every frame.

  if( world_units_per_pixel_ * opening_radius_ > 0 )
  {
    vil_binary_erode( *src_img_, buffer_, opening_el_ );
    vil_binary_dilate( buffer_, out_img_, opening_el_ );
  }
  else if( world_units_per_pixel_ * closing_radius_ > 0 )
  {
    vil_binary_dilate( *src_img_, buffer_, closing_el_ );
    vil_binary_erode( buffer_, out_img_, closing_el_ );
  }
  else
  {
    out_img_ = *src_img_;
  }

  return true;
}


void
world_morphology_process
::set_source_image( vil_image_view<bool> const& img )
{
  src_img_ = &img;
}


vil_image_view<bool> const&
world_morphology_process
::image() const
{
  return out_img_;
}

void 
world_morphology_process
::set_world_units_per_pixel( double units_per_pix )
{
  world_units_per_pixel_ = units_per_pix;
}

void
world_morphology_process
::set_is_fg_good( bool val )
{
  is_fg_good_ = val;
}


} // end namespace vidtk
