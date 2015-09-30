/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object.h"
#include <vgl/vgl_polygon.h>
#include <vul/vul_sprintf.h>
#include <utilities/log.h>

namespace vidtk
{
namespace vgui
{


draw_image_object
::draw_image_object()
{
  set_to_default();
}


draw_image_object
::draw_image_object( vgui_easy2D_tableau_sptr easy )
  : easy_( easy )
{
  set_to_default();
}

void
draw_image_object
::init( vgui_easy2D_tableau_sptr easy )
{
  easy_ = easy;
  set_to_default();
}

void
draw_image_object::
draw( vidtk::image_object const& obj )
{
  if( loc_color_.r >= 0 )
  {
    easy_->set_foreground( loc_color_.r,
                           loc_color_.g,
                           loc_color_.b,
                           loc_color_.a );
  }
  easy_->add_point( obj.img_loc_[0], obj.img_loc_[1] );

  // Add polygon
  if( show_boundary_ && obj.boundary_.num_sheets() == 1 )
  {
    unsigned const N = obj.boundary_[0].size();
    vcl_vector<float> x, y;
    x.reserve( N );
    y.reserve( N );
    for( unsigned i = 0; i < N; ++i )
    {
      x.push_back( obj.boundary_[0][i].x() );
      y.push_back( obj.boundary_[0][i].y() );
    }
    if( boundary_color_.r >= 0 )
    {
      easy_->set_foreground( boundary_color_.r,
                             boundary_color_.g,
                             boundary_color_.b,
                             boundary_color_.a );
    }
    easy_->add_polygon( N, &x[0], &y[0] );
  }

  // Add bounding box
  {
    float x[4] = { obj.bbox_.min_x(), obj.bbox_.max_x(),
                   obj.bbox_.max_x(), obj.bbox_.min_x() };
    float y[4] = { obj.bbox_.min_y(), obj.bbox_.min_y(),
                   obj.bbox_.max_y(), obj.bbox_.max_y() };
    if( bbox_color_.r >= 0 )
    {
      easy_->set_foreground( bbox_color_.r,
                             bbox_color_.g,
                             bbox_color_.b,
                             bbox_color_.a );
    }
    easy_->add_polygon( 4, x, y );
  }

  if( show_area_ )
  {
    if( text_ == NULL )
    {
      log_error( "vgui::draw_image_object: need a text tableau to output area\n" );
    }
    else
    {
      text_->add( obj.img_loc_[0], obj.img_loc_[1]+10,
                  vul_sprintf( "%.2f", obj.area_ ) );
    }
  }
}


void
draw_image_object
::draw_world2d( vidtk::image_object const& obj )
{
  if( loc_color_.r >= 0 )
  {
    easy_->set_foreground( loc_color_.r,
                           loc_color_.g,
                           loc_color_.b,
                           loc_color_.a );
  }
  easy_->add_point( obj.world_loc_[0], obj.world_loc_[1] );
}


void
draw_image_object
::set_to_default()
{
  loc_color_ = vil_rgba<float>( 1, 0, 0 );
  bbox_color_ = vil_rgba<float>( 1, 0, 0 );
  boundary_color_ = vil_rgba<float>( 1, 0, 0 );
  show_area_ = false;
  show_boundary_ = true;
}


} // end namespace vgui
} // end namespace vidtk
