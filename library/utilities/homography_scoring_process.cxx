/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_scoring_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vgl/vgl_area.h>
#include <vgl/vgl_distance.h>
#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_polygon.h>

#include <cmath>

namespace vidtk
{

template<typename T>
static vgl_polygon<T> transform_polygon( const vgl_polygon<T>& polygon, const vgl_h_matrix_2d<T>& homog );

homography_scoring_process
  ::homography_scoring_process( vcl_string const& name )
  : process( name, "homography_scoring_process" ),
    disabled_( true )
{
  config_.add_parameter( "disabled",
    "true",
    "Do not do anything." );

  config_.add_parameter( "max_dist_offset",
    "5",
    "Maximum distance between corresponding points after transformation" );

  config_.add_parameter( "area_percent_factor",
    ".2",
    "Maximum factor the area of the resulting figure is off from the good homography" );

  config_.add_parameter( "quadrant",
    "1",
    "Quadrant to place the original rectangle in (0 is centered on origin)" );

  config_.add_parameter( "height",
    "480",
    "Height of the original rectangle to use" );

  config_.add_parameter( "width",
    "720",
    "Height of the original rectangle to use" );
}


homography_scoring_process
::~homography_scoring_process()
{
}


config_block
homography_scoring_process
::params() const
{
  return config_;
}


bool
homography_scoring_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get("disabled",this->disabled_);
    if( !this->disabled_ )
    {
      blk.get("max_dist_offset",this->max_dist_offset_);
      blk.get("area_percent_factor",this->area_percent_factor_);
      blk.get("quadrant",this->quadrant_);
      blk.get("height",this->height_);
      blk.get("width",this->width_);
    }
  }
  catch( unchecked_return_value& e)
  {
    log_error( name() << ": couldn't set parameters: "<< e.what() <<"\n" );

    return false;
  }

  config_.update( blk );
  return true;
}


bool
homography_scoring_process
::initialize()
{
  is_good_homog_ = true;

  return true;
}


bool
homography_scoring_process
::step()
{
  if( this->disabled_ )
  {
    return false;
  }

  is_good_homog_ = true;

  vgl_polygon<double> orig_rect;
  orig_rect.add_contour( NULL, NULL, 0 );

  switch( quadrant_ )
  {
    case 0:
      orig_rect.push_back(  0.5 * width_,  0.5 * height_ );
      orig_rect.push_back( -0.5 * width_,  0.5 * height_ );
      orig_rect.push_back( -0.5 * width_, -0.5 * height_ );
      orig_rect.push_back(  0.5 * width_, -0.5 * height_ );
      break;
    case 1:
      orig_rect.push_back( width_,  height_ );
      orig_rect.push_back( 0,       height_ );
      orig_rect.push_back( 0,       0 );
      orig_rect.push_back( width_,  0 );
      break;
    case 2:
      orig_rect.push_back( 0,       height_ );
      orig_rect.push_back( -width_, height_ );
      orig_rect.push_back( -width_, 0 );
      orig_rect.push_back( 0,       0 );
      break;
    case 3:
      orig_rect.push_back( 0,       0 );
      orig_rect.push_back( -width_, 0 );
      orig_rect.push_back( -width_, -height_ );
      orig_rect.push_back( 0,       -height_ );
      break;
    case 4:
      orig_rect.push_back( width_,  0 );
      orig_rect.push_back( 0,       0 );
      orig_rect.push_back( 0,       -height_ );
      orig_rect.push_back( width_,  -height_ );
      break;
    default:
      log_error( name() << ": invalid quadrant: " << quadrant_ << "\n" );
      return false;
  }

  const vgl_polygon<double> good_rect = transform_polygon( orig_rect, good_homography_ );
  const vgl_polygon<double> test_rect = transform_polygon( orig_rect, test_homography_ );

  const vgl_polygon<double>::sheet_t& orig_sheet = orig_rect[0];
  const vgl_polygon<double>::sheet_t& good_sheet = good_rect[0];
  const vgl_polygon<double>::sheet_t& test_sheet = test_rect[0];

  double max_dist = 0;

  for( size_t i = 0; i < orig_sheet.size(); ++i )
  {
    const double dist = vgl_distance( good_sheet[i], test_sheet[i] );

    log_debug( name() << ": Moved point " << orig_sheet[i] << " to: " << vcl_endl
                      << "  good: " << good_sheet[i] << vcl_endl
                      << "  test: " << test_sheet[i] << vcl_endl
                      << "  dist: " << dist << vcl_endl );

    if( dist > max_dist )
    {
      max_dist = dist;
    }
  }

  if( max_dist > max_dist_offset_ )
  {
    log_debug( name() << ": Distance " << max_dist
                      << " > " << max_dist_offset_ << vcl_endl );
    is_good_homog_ = false;
  }

  const double orig_area = vgl_area( orig_rect );
  const double good_area = vgl_area( good_rect );
  const double test_area = vgl_area( test_rect );
  const double area_diff = std::abs( good_area - test_area );
  const double area_factor = area_diff / good_area;

  log_debug( name() << ": Original area: " << orig_area << vcl_endl
                    << "  good: " << good_area << vcl_endl
                    << "  test: " << test_area << vcl_endl
                    << "  offset: " << area_factor << vcl_endl );

  if( area_factor > area_percent_factor_ )
  {
    log_debug( name() << ": Total area " << area_factor
                      << " > " << area_percent_factor_ << vcl_endl );
    is_good_homog_ = false;
  }

  return true;
}

void homography_scoring_process::set_good_homography( vgl_h_matrix_2d<double> const& homog )
{
  this->good_homography_ = homog;
}

void homography_scoring_process::set_test_homography( vgl_h_matrix_2d<double> const& homog )
{
  this->test_homography_ = homog;
}

bool homography_scoring_process::is_good_homography() const
{
  return this->is_good_homog_;
}

template<typename T>
vgl_polygon<T> transform_polygon( const vgl_polygon<T>& polygon, const vgl_h_matrix_2d<T>& homog )
{
  vgl_polygon<T> new_polygon;

  for( size_t i = 0; i < polygon.num_sheets(); ++i )
  {
    const typename vgl_polygon<T>::sheet_t& sheet = polygon[i];

    new_polygon.add_contour( NULL, NULL, 0 );

    for( size_t j = 0; j < sheet.size(); ++j )
    {
      const vgl_homg_point_2d<T> hpt( sheet[j].x(), sheet[j].y() );
      const vgl_homg_point_2d<T> t_hpt = homog * hpt;

      const typename vgl_polygon<T>::point_t pt( t_hpt.x() / t_hpt.w(),
                                                 t_hpt.y() / t_hpt.w() );

      new_polygon.push_back( pt );
    }
  }

  return new_polygon;
}

} // end namespace vidtk
