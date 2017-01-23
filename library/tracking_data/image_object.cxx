/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/image_object.h>
#include <tracking_data/tracking_keys.h>

#include <vbl/vbl_smart_ptr.hxx>
#include <vgl/vgl_vector_2d.h>

#include <boost/make_shared.hpp>

namespace vidtk
{

// ----------------------------------------------------------------------------
image_object
::image_object()
  : source_type_( UNKNOWN ),
    area_( -1 ),
    img_area_( -1 ),
    confidence_(0.0)
{
  img_loc_[0] = -1;
  img_loc_[1] = -1;

  world_loc_[0] = -1;
  world_loc_[1] = -1;
  world_loc_[2] = -1;
}

// ----------------------------------------------------------------------------
vgl_polygon< float_type > const&
image_object
::get_boundary() const
{
  return boundary_;
}

// ----------------------------------------------------------------------------
void
image_object
::set_boundary( vgl_polygon< float_type > const& b )
{
  boundary_ = b;
}

// ----------------------------------------------------------------------------
vgl_box_2d< unsigned > const&
image_object
::get_bbox() const
{
  return bbox_;
}

// ----------------------------------------------------------------------------
void
image_object
::set_bbox( vgl_box_2d< unsigned > const& b )
{
  bbox_ = b;
}

// ----------------------------------------------------------------------------
void
image_object
::set_bbox( unsigned min_x, unsigned max_x, unsigned min_y, unsigned max_y )
{
  bbox_.set_min_x( min_x );
  bbox_.set_max_x( max_x );
  bbox_.set_min_y( min_y );
  bbox_.set_max_y( max_y );
}

// ----------------------------------------------------------------------------
float_type
image_object
::get_area() const
{
  return area_;
}

// ----------------------------------------------------------------------------
void
image_object
::set_area( float_type a )

{
  area_ = a;
}

// ----------------------------------------------------------------------------
float_type
image_object
::get_image_area() const
{
  return img_area_;
}

// ----------------------------------------------------------------------------
void
image_object
::set_image_area( float_type a )
{
  img_area_ = a;
}

// ----------------------------------------------------------------------------
unsigned
image_object
::get_mask_i0() const
{
  return mask_i0_;
}

// ----------------------------------------------------------------------------
unsigned int
image_object
::get_mask_j0() const
{
  return mask_j0_;
}

// ----------------------------------------------------------------------------
bool
image_object
::is_used_in_track() const
{
  return data_.is_set(tracking_keys::used_in_track);
}

// ----------------------------------------------------------------------------
void
image_object
::set_used_in_track( bool u )
{
  data_.set(tracking_keys::used_in_track, u);
}

// ----------------------------------------------------------------------------
bool
image_object
::get_histogram(image_histogram& hist) const
{
  return data_.get( tracking_keys::histogram, hist );
}

// ----------------------------------------------------------------------------
void
image_object
::set_histogram( const image_histogram& hist )
{
  data_.set( tracking_keys::histogram, hist );
}

// ----------------------------------------------------------------------------
bool
image_object
::get_intensity_distribution(intensity_distribution_type& dist) const
{
  return data_.get( tracking_keys::intensity_distribution, dist );
}

// ----------------------------------------------------------------------------
void
image_object
::set_intensity_distribution( const intensity_distribution_type& dist )
{
  data_.set( tracking_keys::intensity_distribution, dist );
}


// ----------------------------------------------------------------------------
geo_coord::geo_coordinate_sptr const
image_object
::get_geo_loc() const
{
  return geo_loc_;
}


// ----------------------------------------------------------------------------
void
image_object
::set_geo_loc(geo_coord::geo_coordinate_sptr const geo)
{
  geo_loc_ = geo;
}


// ----------------------------------------------------------------------------
vidtk_pixel_coord_type const&
image_object
::get_image_loc() const
{
  return img_loc_;
}

// ----------------------------------------------------------------------------
void
image_object
::set_image_loc(vidtk_pixel_coord_type const& loc)
{
  img_loc_ = loc;
}

// ----------------------------------------------------------------------------
void
image_object
::set_image_loc(float_type x, float_type y)
{
  img_loc_[0] = x;
  img_loc_[1] = y;
}

// ----------------------------------------------------------------------------
tracker_world_coord_type const&
image_object
::get_world_loc() const
{
  return world_loc_;
}

// ----------------------------------------------------------------------------
void
image_object
::set_world_loc(tracker_world_coord_type const& loc)
{
  world_loc_ = loc;
}

// ----------------------------------------------------------------------------
void
image_object
::set_world_loc(float_type x, float_type y, float_type z)
{
  world_loc_[0] = x;
  world_loc_[1] = y;
  world_loc_[2] = z;
}

// ----------------------------------------------------------------------------
void image_object
::set_source( source_code s, std::string const& inst )
{
  this->source_type_ = s;
  this->source_instance_name_ = inst;
}

// ----------------------------------------------------------------------------
image_object::source_code
image_object
::get_source_type() const
{
  return this->source_type_;
}

// ----------------------------------------------------------------------------
std::string
image_object
::get_source_name() const
{
  return this->source_instance_name_;
}

// ----------------------------------------------------------------------------
double
image_object
::get_confidence() const
{
  return this->confidence_;
}

// ----------------------------------------------------------------------------
void
image_object
::set_confidence( double val )
{
  this->confidence_ = val;
}

// ----------------------------------------------------------------------------
void
image_object
::image_shift( int di, int dj )
{
  vgl_vector_2d< float_type > delta_float( di, dj );

  for( unsigned s = 0; s < boundary_.num_sheets(); ++s )
  {
    vgl_polygon<float_type>::sheet_t& sh = boundary_[s];
    for( unsigned i = 0; i < sh.size(); ++i )
    {
      sh[i] += delta_float;
    }
  }

  // this is okay because unsigned int are modulo N arithmetric
  vgl_vector_2d<unsigned> delta_unsigned( di, dj );
  bbox_.set_centroid( bbox_.centroid() + delta_unsigned );

  img_loc_[0] += di;
  img_loc_[1] += dj;

  // world_loc_ is unchanged

  // area_ is unchanged

  mask_i0_ += di;
  mask_j0_ += dj;

  // mask_ is unchanged
}

// ----------------------------------------------------------------------------
image_object_sptr
image_object
::clone()
{
  //shallow copy
  image_object_sptr copy = new image_object( *this );

  //deep copy
  copy->mask_ = vil_image_view< bool >();
  copy->mask_.deep_copy( this->mask_ );

  //copy heat map
  if (this->heat_map_)
  {
    copy->heat_map_ = boost::make_shared< heat_map_type >();
    copy->heat_map_->deep_copy( *this->heat_map_ );
  }

  tracking_keys::deep_copy_property_map( this->data_, copy->data_);
  return copy;
}

// ----------------------------------------------------------------------------
bool
image_object
::get_object_mask( vil_image_view< bool >& mask, image_point_type& origin ) const
{
  // If the mask empty, bail before setting anything.

  if (this->mask_.size_bytes() == 0)
  {
    return false;
  }

  mask = this->mask_;
  origin.x() =  mask_i0_;
  origin.y() =  mask_j0_;
  return true;
}

// ----------------------------------------------------------------------------
void
image_object
::set_object_mask( vil_image_view< bool > const& mask, image_point_type const& origin )
{
  // This could be a deep copy operation
  this->mask_ = mask;
  this->mask_i0_ = origin.x();
  this->mask_j0_ = origin.y();
}

// ----------------------------------------------------------------------------
bool
image_object
::get_image_chip( vil_image_resource_sptr& image, unsigned int& border ) const
{
  border = INVALID_IMG_CHIP_BORDER;
  if ( ! this->data_.has( vidtk::tracking_keys::pixel_data ) )
  {
    return false;
  }

  this->data_.get< vil_image_resource_sptr > ( vidtk::tracking_keys::pixel_data, image );

  const unsigned int* chip_offset =
    this->data_.get_if_avail< unsigned > ( tracking_keys::pixel_data_buffer );
  if ( chip_offset != NULL )
  {
    border = *chip_offset;
  }

  return true;
}

// ----------------------------------------------------------------------------
void
image_object
::set_image_chip( vil_image_resource_sptr image, unsigned int border )
{
  this->data_.set( vidtk::tracking_keys::pixel_data, image );

  // Don't set invalid borders.
  if ( border != INVALID_IMG_CHIP_BORDER )
  {
    this->data_.set( vidtk::tracking_keys::pixel_data_buffer, border );
  }
}

// ----------------------------------------------------------------------------
bool
image_object
::get_heat_map( heat_map_sptr& map, image_point_type& origin ) const
{
  // If no heat map present, return false
  if (! heat_map_ )
  {
    return false;
  }

  map = this->heat_map_;
  origin = this->heat_map_origin_;

  return true;
}

// ----------------------------------------------------------------------------
void
image_object
::set_heat_map( heat_map_sptr map, image_point_type const& origin )
{
  this->heat_map_ = map;
  this->heat_map_origin_ = origin;
}

} // end namespace vidtk

// Instantiate the smart pointer
VBL_SMART_PTR_INSTANTIATE( vidtk::image_object );
