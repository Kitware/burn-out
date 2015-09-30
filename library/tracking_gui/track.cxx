/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track.h"
#include <tracking_gui/image_object.h>
#include <tracking/tracking_keys.h>
#include <vgl/vgl_polygon.h>
#include <vcl_iostream.h>
#include <vcl_cmath.h>
#include <vnl/algo/vnl_symmetric_eigensystem.h>
#include <vnl/vnl_matrix_fixed.h>
#include <vnl/vnl_math.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>
#include <vul/vul_sprintf.h>

namespace vidtk
{
namespace vgui
{


draw_track
::draw_track()
{
  set_to_default();
}


draw_track
::draw_track( vgui_easy2D_tableau_sptr easy )
  : easy_( easy )
{
  set_to_default();
}

void
draw_track
::init( vgui_easy2D_tableau_sptr easy )
{
  easy_ = easy;
  set_to_default();
}


int 
draw_track
::draw( vidtk::track const& trk )
{
  return draw( trk, 0 );
}

int
draw_track
::draw_upto( vidtk::track const& trk, unsigned int end_frame )
{
  return draw( trk, 0, end_frame );
}

int
draw_track
::draw( vidtk::track const& trk,
        vnl_matrix_fixed<double,3,3> const* world_to_image,
        unsigned int end_frame,
        unsigned int start_frame )
{
  vcl_vector< vidtk::track_state_sptr > const& hist = trk.history();

  assert( ! hist.empty() );
  unsigned idx = hist.size();
  unsigned stop_at = static_cast<unsigned>(hist.back()->time_.frame_number() - cutoff_);
  if( cutoff_ < 0 )
    stop_at = hist[0]->time_.frame_number();
  if( stop_at < start_frame )
    stop_at = start_frame;
  if( end_frame == 0xFFFFFFFF )
    end_frame = hist.back()->time_.frame_number();

  draw_image_object obj_drawer( easy_ );
  obj_drawer.loc_color_.r = -1;
  obj_drawer.bbox_color_.r = -1;
  obj_drawer.boundary_color_.r = -1;
  obj_drawer.show_boundary_ = draw_obj_boundary_;
  obj_drawer.show_area_ = false;

  vcl_vector<float> ximg, yimg;

  while( idx > 0 && (cutoff_ < 0 || hist[idx-1]->time_.frame_number() > stop_at ) )
  {
    --idx;
    //if( hist[idx]->time_.frame_number() > end_frame )
    //{
    //  continue;
    //}

    if( hist[idx]->time_.frame_number() < end_frame )
    {
      easy_->set_foreground( past_color_.r,
                             past_color_.g,
                             past_color_.b,
                             past_color_.a );
    }
    else
    {
      easy_->set_foreground( current_color_.r,
                             current_color_.g,
                             current_color_.b,
                             current_color_.a );
    }

    vcl_vector<image_object_sptr> objs;
    if( hist[idx]->data_.get( tracking_keys::img_objs, objs ) )
    {
      for( unsigned i = 0; i < objs.size(); ++i )
      {
        if( draw_obj_ && hist[idx]->time_.frame_number() == end_frame )
        {
          obj_drawer.draw( *objs[i] );
        }
        if( world_to_image )
        {
          vnl_vector_fixed<double,3> p = objs[i]->world_loc_;
          p[2] = 1;
          p = (*world_to_image) * p;
          ximg.push_back( p[0] / p[2] );
          yimg.push_back( p[1] / p[2] );
        }
        else
        {
          ximg.push_back( objs[i]->img_loc_[0] );
          yimg.push_back( objs[i]->img_loc_[1] );
        }
      }

    }
  }

  if( !ximg.empty())
  {
    if( draw_trail_ )
    {
      easy_->set_foreground( trail_color_.r,
                             trail_color_.g,
                             trail_color_.b,
                             trail_color_.a );

      easy_->add_linestrip( ximg.size(), &ximg[0], &yimg[0] );
    }

    if( draw_point_at_end_ )
    {
      easy_->add_point( ximg.back(), yimg.back() );
    }
    if( text_ )
    {
      text_->add( ximg[0], yimg[0], vul_sprintf("%d", trk.id() ) );
    }

    return 0;
  }
  else
  {
    return 1;
  }
}


int
draw_track
::draw_world2d( vidtk::track const& trk )
{
  vcl_vector< vidtk::track_state_sptr > const& hist = trk.history();

  assert( ! hist.empty() );
  unsigned idx = hist.size();
  double stop_at = hist[idx-1]->time_.frame_number() - cutoff_;

  draw_image_object obj_drawer( easy_ );
  obj_drawer.loc_color_.r = -1;
  obj_drawer.bbox_color_.r = -1;
  obj_drawer.boundary_color_.r = -1;

  vcl_vector<float> x, y;
  while( idx > 0 && (cutoff_ < 0 || hist[idx-1]->time_.frame_number() > stop_at ) )
  {
    --idx;

    x.push_back( hist[idx]->loc_[0] );
    y.push_back( hist[idx]->loc_[1] );

    if( idx+1 < hist.size() )
    {
      easy_->set_foreground( past_color_.r,
                             past_color_.g,
                             past_color_.b,
                             past_color_.a );
    }
    else
    {
      easy_->set_foreground( current_color_.r,
                             current_color_.g,
                             current_color_.b,
                             current_color_.a );
    }

    vcl_vector<image_object_sptr> objs;
    if( hist[idx]->data_.get( "MODs", objs ) )
    {
      for( unsigned i = 0; i < objs.size(); ++i )
      {
        obj_drawer.draw_world2d( *objs[i] );
      }
    }
  }

  draw_point_at_end_ = false;

  if( !x.empty() )
  {
    if( draw_trail_ )
    {
      easy_->set_foreground( trail_color_.r,
                             trail_color_.g,
                             trail_color_.b,
                             trail_color_.a );
      easy_->add_linestrip( x.size(), &x[0], &y[0] );
    }
    if( draw_point_at_end_ )
    {
      easy_->add_point( x.back(), y.back() );
    }
    return 0;
  }
  else
  {
    return 1;
  }
}


void
draw_track
::draw_gate( vnl_vector_fixed<double,2> const& loc,
             vnl_matrix_fixed<double,2,2> const& cov,
             double sigma,
             vnl_matrix_fixed<double,3,3>* backproj )
{
  vnl_matrix_fixed<double,2,2> V;
  vnl_vector_fixed<double,2> lambda;
  vnl_symmetric_eigensystem_compute( cov.as_ref(), V.as_ref().non_const(), lambda.as_ref().non_const() );

  double sig_0 = vcl_sqrt( lambda(0) );
  double sig_1 = vcl_sqrt( lambda(1) );
  vnl_double_2 ax1( V(0,0)*sig_0, V(1,0)*sig_0 );
  vnl_double_2 ax2( V(0,1)*sig_1, V(1,1)*sig_1 );
  ax1 *= sigma;
  ax2 *= sigma;

  unsigned const N = 20;
  float xs[N], ys[N];
  for( unsigned i = 0; i < N; ++i )
  {
    double t = vnl_math::pi * 2 * i / N;
    double ct = vcl_cos( t );
    double st = vcl_sin( t );
    vnl_double_2 pt = loc + ct * ax1 + st * ax2;
    if( backproj )
    {
      vnl_double_3 x = (*backproj) * vnl_double_3( pt(0), pt(1), 1 );
      xs[i]= x(0) / x(2);
      ys[i]= x(1) / x(2);
    }
    else
    {
      xs[i] = pt(0);
      ys[i] = pt(1);
    }
  }

  if( draw_trail_ && trail_color_.r >= 0 )
  {
    easy_->set_foreground( trail_color_.r,
                           trail_color_.g,
                           trail_color_.b,
                           trail_color_.a );
  }
  easy_->add_polygon( N, &xs[0], &ys[0] );
}


void
draw_track
::set_to_default()
{
  cutoff_ = -1;
  draw_point_at_end_ = false;
  draw_obj_ = true;
  draw_trail_ = true;
  trail_color_ = vil_rgba<float>( 0, 1, 0 );
  current_color_ = vil_rgba<float>( 0, 1, 0 );
  past_color_ = vil_rgba<float>( 0.7f, 1, 0.7f );
}


} // end namespace vgui
} // end namespace vidtk
