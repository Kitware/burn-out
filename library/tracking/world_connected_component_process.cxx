/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "world_connected_component_process.h"
#include <utilities/log.h>

#include <vcl_cassert.h>
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


world_connected_component_process
::world_connected_component_process( vcl_string const& name )
  : process( name, "world_connected_component_process" ),
  world_units_per_pixel_( 1.0 ),
  is_fg_good_( true )
{
  config_.add_parameter( "min_size", "0", "Minimum size" );
  config_.add_parameter( "max_size", "1000000", "Maximum size" );
  config_.add_parameter( "location_type",
                         "centroid",
                         "Location of the target for tracking: bottom or centroid. "
                         "This parameter is used in conn_comp_sp:conn_comp, "
                         "tracking_sp:tracker, and tracking_sp:state_to_image");
  config_.add_parameter( "world_units_per_pixel", "1", " " );
  config_.add_parameter( "connectivity", "4", "Pixel connectivity used to label components" );
}


config_block
world_connected_component_process
::params() const
{
  return config_;
}


bool
world_connected_component_process
::set_params( config_block const& blk )
{
  blk.get( "min_size", min_size_ );
  blk.get( "max_size", max_size_ );

  vcl_string loc;
  blk.get( "location_type", loc );

  unsigned int conn_int;
  blk.get( "connectivity", conn_int );

  if( conn_int == 4 )
  {
    blob_connectivity_ = vil_blob_4_conn;
  }
  else if( conn_int == 8 )
  {
    blob_connectivity_ = vil_blob_8_conn;
  }

  else
  {
    log_error( "Invalid blob connectivity type \"" << conn_int << "\". Expect \"4\" or \"8\".\n" );
    return false;
  }

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
world_connected_component_process
::initialize()
{
  return true;
}


bool
world_connected_component_process
::step()
{
  assert( fg_img_ != NULL );

  objs_.clear();

  if( !is_fg_good_ )
    return true;

  vil_image_view<unsigned> labels_whole;
  vcl_vector<vil_blob_region> blobs;
  vcl_vector<vil_blob_region>::iterator it_blobs;

  vil_blob_labels(*fg_img_, blob_connectivity_, labels_whole);
  vil_blob_labels_to_regions(labels_whole, blobs);

  double pixel_area_to_world_area = world_units_per_pixel_ * world_units_per_pixel_;

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
    // pre-compute area in world units
    float_type area = pixel_area_to_world_area * vil_area( *it_blobs );

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

      // Compute area in world units using src2wld homography
      vidtk::homography::transform_t H = H_src2wld_.get_transform();
      vnl_matrix_fixed<double, 2, 2> jac;
      vnl_vector_fixed<double, 2> from_loc;
      from_loc(0)=obj.img_loc_[0]; from_loc(1)=obj.img_loc_[1];
      jacobian_from_homo_wrt_loc(jac, H.get_matrix(), from_loc );
      // only compute the scaling ratio on the direction paralle to the image u axis
      float scaling_factor = jac(0,0)*jac(0,0) + jac(1,0)*jac(1,0);
      area = vil_area( *it_blobs );
      area = area * scaling_factor;
      obj.area_ = area;

      objs_.push_back( obj_sptr );
    }
  }

#ifdef PRINT_DEBUG_INFO
  log_info( this->name() << ": Produced " << objs_.size() << " objects.\n" );
#endif

  return true;
}


void
world_connected_component_process
::set_fg_image( vil_image_view<bool> const& img )
{
  fg_img_ = &img;
}


vcl_vector< image_object_sptr > const&
world_connected_component_process
::objects() const
{
  return objs_;
}

void
world_connected_component_process
::set_world_units_per_pixel( double val )
{
  world_units_per_pixel_ = val;
}

void
world_connected_component_process
::set_is_fg_good( bool val )
{
  is_fg_good_ = val;
}

void
world_connected_component_process
::set_src_2_wld_homography(image_to_plane_homography const& H)
{
  H_src2wld_ = H;
}

} // end namespace vidtk
