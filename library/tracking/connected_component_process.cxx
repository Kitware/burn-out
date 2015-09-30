/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "connected_component_process.h"
#include <utilities/log.h>

#include <vcl_cassert.h>
#include <vil/algo/vil_blob.h>  //_finder.h>
#include <vgl/vgl_convex.h>
#include <vgl/vgl_area.h>
#include <vil/vil_crop.h>


namespace {

using namespace vidtk;

// Compute the centroid of the polygon.
image_object::float2d_type
polygon_centroid( vgl_polygon<image_object::float_type> const& poly )
{
  typedef image_object::float_type float_type;
  typedef image_object::float2d_type float2d_type;
  typedef image_object::float3d_type float3d_type;

  assert( poly.num_sheets() == 1 );
  vgl_polygon<float_type>::sheet_t const& sh = poly[0];

  unsigned const n = sh.size();
  assert( n > 0 );

  float2d_type cent( 0, 0 );
  float_type A2 = 0.0;

  for( unsigned i = 0, j = n-1; i < n; j = i++ )
  {
    float_type d = sh[j].x() * sh[i].y() - sh[i].x() * sh[j].y();
    A2 += d;
    cent[0] += ( sh[j].x() + sh[i].x() ) * d;
    cent[1] += ( sh[j].y() + sh[i].y() ) * d;
  }
  cent /= (3 * A2);

  return cent;
}


// Compute the bottom-most, left-most corner of the polygon, assuming
// the polygon is in image coordinates. (I.e. find the point with the
// largest y, smallest x).
image_object::float2d_type
polygon_bottom_left( vgl_polygon<image_object::float_type> const& poly )
{
  typedef image_object::float_type float_type;
  typedef image_object::float2d_type float2d_type;

  assert( poly.num_sheets() == 1 );
  vgl_polygon<float_type>::sheet_t const& sh = poly[0];

  unsigned const n = sh.size();
  assert( n > 0 );

  float2d_type pt( sh[0].x(), sh[0].y() );

  for( unsigned i = 1; i < n; ++i )
  {
    if( sh[i].y() > pt[1] ||
        ( sh[i].y() == pt[1] && sh[i].x() < pt[0] ) )
    {
      pt = float2d_type( sh[i].x(), sh[i].y() );
    }
  }

  return pt;
}


} // end anonymous namespace


namespace vidtk
{


connected_component_process
::connected_component_process( vcl_string const& name )
  : process( name, "connected_component_process" ),
    loc_type_(centroid)
{
  config_.add( "min_size", "1" );
  config_.add( "max_size", "1000000" );
  config_.add_parameter( "location_type",
                         "centroid",
                         "Location of the target for tracking: bottom or centroid. "
                         "This parameter is used in conn_comp_sp:conn_comp, tracking_sp:tracker, "
                         "and tracking_sp:state_to_image");
}


config_block
connected_component_process
::params() const
{
  return config_;
}


bool
connected_component_process
::set_params( config_block const& blk )
{
  min_size_ = blk.get<float_type>( "min_size" );
  max_size_ = blk.get<float_type>( "max_size" );

  vcl_string loc = blk.get<vcl_string>( "location_type" );
  if( loc == "centroid" )
  {
    loc_type_ = centroid;
  }
  else if( loc == "bottom" )
  {
    loc_type_ = bottom;
  }
  else
  {
    log_error( "Invalid location type \"" << loc << "\". Expect \"centroid\" or \"bottom\".\n" );
    return false;
  }

  config_.update( blk );

  return true;
}


bool
connected_component_process
::initialize()
{
  return true;
}


bool
connected_component_process
::step()
{
  assert( fg_img_ != NULL );

  objs_.clear();

  //perform 4 connectivity connected components
  vil_blob_connectivity conn = vil_blob_4_conn;

  vil_image_view<unsigned> labels_whole, labels_edges;
  vcl_vector<vil_blob_region> blobs;
  vcl_vector<vil_blob_region>::iterator it_blobs;

  vil_blob_labels(*fg_img_, conn, labels_whole);
  vil_blob_labels_to_edge_labels(labels_whole, conn, labels_edges);
  vil_blob_labels_to_regions(labels_edges, blobs);

  for(it_blobs=blobs.begin(); it_blobs < blobs.end(); it_blobs++)
  {
    image_object_sptr obj_sptr = new image_object;
    image_object& obj = *obj_sptr; // to avoid the costly dereference
                                   // every time.

    vcl_vector< vgl_point_2d<float_type> > pts;
    vil_blob_region::iterator it_chords;

    unsigned int i1, i2, j1, j2;

    for(it_chords=it_blobs->begin(); it_chords < it_blobs->end(); it_chords++)
    {
      unsigned int j = it_chords->j;
      obj.bbox_.add( image_object::image_point_type(it_chords->ilo, j));
      obj.bbox_.add( image_object::image_point_type(it_chords->ihi, j));

      for(unsigned int i=it_chords->ilo; i <= it_chords->ihi; i++)
      {
        pts.push_back(vgl_point_2d<float_type>( i, j ));
      }
    }

    // Move the max point, so that min_point() is inclusive and
    // max_point() is exclusive.
    obj.bbox_.set_max_x( obj.bbox_.max_x() + 1);
    obj.bbox_.set_max_y( obj.bbox_.max_y() + 1);

    obj.boundary_ = vgl_convex_hull( pts );
    float_type area = vgl_area( obj.boundary_ );
    obj.area_ = area;

    obj.mask_i0_ = obj.bbox_.min_x();
    obj.mask_j0_ = obj.bbox_.min_y();

    vil_image_view<bool> mask_chip = vil_crop( *fg_img_, obj.bbox_.min_x(),
               obj.bbox_.width(), obj.bbox_.min_y(), obj.bbox_.height() );
    obj.mask_.deep_copy( mask_chip );

    if( min_size_ <= area && area <= max_size_ )
    {
      switch( loc_type_ )
      {
      case centroid:
        obj.img_loc_ = polygon_centroid( obj.boundary_ );
        break;
      case bottom:
        float_type x = (obj.bbox_.min_x() + obj.bbox_.max_x()) / float_type(2);
        float_type y = float_type( obj.bbox_.max_y() );
        obj.img_loc_ = float2d_type( x, y );
        break;
      }
      obj.world_loc_ = float3d_type( obj.img_loc_[0], obj.img_loc_[1], 0 );

      objs_.push_back( obj_sptr );
    }
  }

  return true;
}


void
connected_component_process
::set_fg_image( vil_image_view<bool> const& img )
{
  fg_img_ = &img;
}


vcl_vector< image_object_sptr > const&
connected_component_process
::objects() const
{
  return objs_;
}


} // end namespace vidtk
