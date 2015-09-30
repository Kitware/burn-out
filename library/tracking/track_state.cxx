/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_state.h"

#include <tracking/tracking_keys.h>
#include <vcl_utility.h>
#include <vcl_vector.h>
#include <vgl/vgl_homg_point_2d.h>
#include <geographic/geo_coords.h>
#include <utilities/log.h>
#include <utilities/homography.h>

namespace vidtk
{

void
track_state
::set_latitude_longitude( double latitude, double longitude )
{
  data_.set( tracking_keys::lat_long, vcl_pair<double, double>( latitude, longitude ) );
}


void
track_state
::set_latitude_longitude( plane_to_utm_homography const& H_wld2utm )
{
  if( !H_wld2utm.is_valid() )
  {
    log_warning( "Could not set latitude and longitude in track state because "
      "world-to-utm homography is invalid\n" );
    return;
  }

  vgl_homg_point_2d< double > wld_loc( loc_(0), loc_(1) );
  vgl_homg_point_2d< double > utm_loc = H_wld2utm.get_transform() * wld_loc;

  // UTM --> lat/long conversion
  geographic::geo_coords geo_pt( H_wld2utm.get_dest_reference().zone(),
                                 H_wld2utm.get_dest_reference().is_north(),
                                 utm_loc.x()/utm_loc.w(),
                                 utm_loc.y()/utm_loc.w() );
  this->set_latitude_longitude( geo_pt.latitude(), geo_pt.longitude() );
}


bool
track_state
::latitude_longitude( double & latitude, double & longitude ) const
{
  if( data_.has( tracking_keys::lat_long ) )
  {
    vcl_pair<double, double> lati_longi;
    data_.get( tracking_keys::lat_long, lati_longi );
    latitude = lati_longi.first;
    longitude = lati_longi.second;
    return true;
  }
  else
  {
    return false;
  }
}


void
track_state
::set_gsd( float gsd )
{
  data_.set( tracking_keys::gsd, gsd );
}


bool
track_state
::gsd( float & gsd ) const
{
  if( data_.has( tracking_keys::gsd ) )
  {
    data_.get( tracking_keys::lat_long, gsd );
    return true;
  }
  else
  {
    return false;
  }
}


void
track_state
::set_image_object( image_object_sptr & obj )
{
  data_.set( tracking_keys::img_objs, vcl_vector<image_object_sptr>( 1, obj ) );
}


bool
track_state
::image_object( image_object_sptr & image_object ) const
{
  if( data_.has( tracking_keys::img_objs ) )
  {
    vcl_vector< image_object_sptr > objs;
    data_.get( tracking_keys::img_objs, objs );
    image_object = objs[0];
    return true;
  }
  else
  {
    return false;
  }
}


bool
track_state
::set_bbox( vgl_box_2d<unsigned> const & box )
{
  image_object_sptr obj;
  if( ! image_object( obj ) )
  {
    return false;
  }
  obj->bbox_ = box;

  return true;
}


bool
track_state
::bbox( vgl_box_2d<unsigned> & box ) const
{
  image_object_sptr obj;
  if( ! image_object( obj ) )
  {
    return false;
  }
  box = obj->bbox_;
  return true;
}


void
track_state
::set_location_covariance( vnl_double_2x2 const & cov )
{
  data_.set( tracking_keys::loc_cov, cov );
}


bool
track_state
::location_covariance( vnl_double_2x2 & cov ) const
{
  if( data_.has( tracking_keys::loc_cov ) )
  {
    data_.get( tracking_keys::loc_cov, cov );
    return true;
  }
  else
  {
    return false;
  }
}


track_state_sptr
track_state
::clone()
{
  track_state_sptr copy = new track_state( *this);

  // Deep copying data member(s)
  tracking_keys::deep_copy_property_map( this->data_,
                                         copy->data_ );

  return copy;
}


// ----------------------------------------------------------------
void
track_state
::set_attr (state_attr_t attr)
{
  if (attr & _ATTR_ASSOC_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_ASSOC_MASK) | attr;
  }
  else if (attr & _ATTR_KALMAN_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_KALMAN_MASK) | attr;
  }
  else if (attr & _ATTR_INTERVAL_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_INTERVAL_MASK) | attr;
  }
  else if (attr & _ATTR_LINKING_MASK)
  {
    // handle set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_LINKING_MASK) | attr;
  }
  else
  {
    this->attributes_ |= attr;
  }
}


// ----------------------------------------------------------------
void
track_state
::clear_attr (state_attr_t attr)
{
  if (attr & _ATTR_ASSOC_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_ASSOC_MASK);
  }
  else if (attr & _ATTR_KALMAN_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_KALMAN_MASK);
  }
  else if (attr & _ATTR_INTERVAL_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_INTERVAL_MASK);
  }
  else if (attr & _ATTR_LINKING_MASK)
  {
    // clear set of bits
    this->attributes_ = (this->attributes_ & ~_ATTR_LINKING_MASK);
  }
  else
  {
    this->attributes_ &= ~attr;
  }
}


// ----------------------------------------------------------------
bool
track_state
::has_attr (state_attr_t attr)
{
  if (attr & _ATTR_ASSOC_MASK)
  {
    // handle group queries
    if (attr == ATTR_ASSOC_FG_GROUP)
    {
      return (this->attributes_ & ATTR_ASSOC_FG_GROUP) != 0;
    }
    else if (attr == ATTR_ASSOC_DA_GROUP)
    {
      return (this->attributes_ & ATTR_ASSOC_DA_GROUP) != 0;
    }
    else
    {
      // handle set of bits
      return (this->attributes_ & _ATTR_ASSOC_MASK) == attr;
    }
  }
  else if (attr & _ATTR_KALMAN_MASK)
  {
    // handle set of bits
    return (this->attributes_ & _ATTR_KALMAN_MASK) == attr;
  }
  else if (attr & _ATTR_INTERVAL_MASK)
  {
    // handle set of bits
    return (this->attributes_ & _ATTR_INTERVAL_MASK) == attr;
  }
  else if (attr & _ATTR_LINKING_MASK)
  {
    // handle set of bits
    return (this->attributes_ & _ATTR_LINKING_MASK) == attr;
  }
  else
  {
    return (this->attributes_ & attr) == attr;
  }
}


bool
track_state
::add_img_vel(vgl_h_matrix_2d<double> const& wld2img_H)
{
  vnl_matrix_fixed<double,3,3> H = wld2img_H.get_matrix();
  vnl_vector_fixed<double,2> wld_vel (vel_(0),vel_(1));
  vnl_vector_fixed<double,2> wld_loc;
  wld_loc(0) = this->loc_(0);
  wld_loc(1) = this->loc_(1);

  vnl_matrix_fixed<double, 2, 2> jacobian;
  // compute jacobian based on the 3x3 homography and the partricular location
  bool ret = jacobian_from_homo_wrt_loc(jacobian, H, wld_loc );
  if(!ret)
  {
    log_error("Fail to compute jacobian in track_state::add_img_vel()\n");
    return false;
  }
  // compute the image velocity based on jacobian
  this->img_vel_ = jacobian * wld_vel;

  return true;

}

} // namespace vidtk
