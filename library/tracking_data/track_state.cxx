/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_state.h"

#include <tracking_data/tracking_keys.h>
#include <tracking_data/vsl/track_io.h>

#include <utility>
#include <vector>
#include <vgl/vgl_homg_point_2d.h>
#include <geographic/geo_coords.h>
#include <logger/logger.h>
#include <utilities/geo_lat_lon.h>
#include <utilities/homography_util.h>

#include <logger/logger.h>

namespace vidtk
{
VIDTK_LOGGER ("track_state");


void
track_state
::set_latitude_longitude( double latitude, double longitude )
{
  this->geo_coord_ = geo_coord::geo_coordinate(latitude, longitude);
}


void
track_state
::set_latitude_longitude( vidtk::geo_coord::geo_coordinate const& ll )
{
  this->geo_coord_= geo_coord::geo_coordinate( ll );
}


bool
track_state
::latitude_longitude( double & latitude, double & longitude ) const
{
  latitude = geo_coord_.get_latitude();
  longitude = geo_coord_.get_longitude();

  return geo_coord_.is_valid();
}


bool
track_state
::latitude_longitude( geo_coord::geo_lat_lon& ll ) const
{
  ll = this->geo_coord_.get_lat_lon();
  return ll.is_valid();
}

const geo_coord::geo_lat_lon &
track_state
::latitude_longitude() const
{
  return this->geo_coord_.get_lat_lon();
}


bool
track_state
::get_location( location_type type, vnl_vector_fixed<double,3> & loc ) const
{
  switch(type)
  {
    case stabilized_plane:
      loc = this->loc_;
      return true;

    case unstabilized_world:
    {
      double lat, lon;
      if ( data_.has( tracking_keys::calculated_utm ) )
      {
        data_.get( tracking_keys::calculated_utm, loc );
        return true;
      }
      else if( this->latitude_longitude(lat, lon) )
      {
        // use lat lon to transform to UTM/World
        geographic::geo_coords geo( lat, lon );
        loc[0] = geo.easting();
        loc[1] = geo.northing();
        loc[2] = 1.0;

        //now, stuff it in the track_state so we don't have to do that again!
        data_.set( tracking_keys::calculated_utm, loc );
        return true;
      }
      return false;
    }

    case stabilized_world:
    {
      geo_coord::geo_UTM geo;
      get_smoothed_loc_utm( geo );
      if(geo.is_valid())
      {
        loc[0] = geo.get_easting();
        loc[1] = geo.get_northing();
        loc[2] = 1.0;
        return true;
      }
      return false;
    }

    default:
      LOG_WARN( "Unknown coordinate type specified: " << type );
      return false;
  }
  return false;
}


geo_coord::geo_coordinate const&
track_state
::get_geo_location() const
{
  return geo_coord_;
}


void
track_state
::set_gsd( float _gsd )
{
  data_.set( tracking_keys::gsd, _gsd );
}


bool
track_state
::gsd( float & _gsd ) const
{
  if( data_.has( tracking_keys::gsd ) )
  {
    data_.get( tracking_keys::gsd, _gsd );
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
  data_.set( tracking_keys::img_objs, std::vector<image_object_sptr>( 1, obj ) );
}


bool
track_state
::image_object( image_object_sptr & _image_object ) const
{
  if( data_.has( tracking_keys::img_objs ) )
  {
    std::vector< image_object_sptr > objs;
    data_.get( tracking_keys::img_objs, objs );
    _image_object = objs[0];
    return true;
  }
  else
  {
    return false;
  }
}


bool
track_state
::get_image_loc( double& x, double& y) const
{
  image_object_sptr io_ptr;
  if (! this->image_object(io_ptr) )
  {
    // image object unavailable
    return false;
  }

  vidtk_pixel_coord_type const& img_loc = io_ptr->get_image_loc();
  x = img_loc[0];
  y = img_loc[1];

  return true;
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
  obj->set_bbox( box );

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
  box = obj->get_bbox();

  return true;
}


void
track_state
::set_location_covariance( vnl_double_2x2 const & cov )
{
  cov_matrix_ = cov;
}


vnl_double_2x2 const&
track_state
::location_covariance() const
{
  return cov_matrix_;
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
    LOG_ERROR( "Fail to compute jacobian in track_state::add_img_vel()" );
    return false;
  }
  // compute the image velocity based on jacobian
  this->img_vel_ = jacobian * wld_vel;

  return true;

}


void
track_state
::get_smoothed_loc_utm( geo_coord::geo_UTM & geo ) const
{
  geo = utm_smoothed_loc_;
}

const geo_coord::geo_UTM &
track_state
::get_smoothed_loc_utm() const
{
  return utm_smoothed_loc_;
}

void
track_state
::get_raw_loc_utm( geo_coord::geo_UTM & geo ) const
{
  geo = utm_raw_loc_;
}


bool
track_state
::get_utm_vel( vnl_vector_fixed<double, 2> & vel ) const
{
  vel = utm_vel_;
  return utm_smoothed_loc_.is_valid();
}


void
track_state
::set_smoothed_loc_utm( geo_coord::geo_UTM const& geo )
{
  utm_smoothed_loc_ = geo;
}


void
track_state
::set_raw_loc_utm( geo_coord::geo_UTM const& geo )
{
  utm_raw_loc_ = geo;
}


void track_state
::set_utm_vel( vnl_vector_fixed<double, 2> const& vel )
{
  utm_vel_ = vel;
}


void track_state
::fill_utm_based_fields( plane_to_utm_homography const& h )
{
  image_object_sptr iobj;

  utm_smoothed_loc_ = geo_coord::geo_UTM();
  utm_raw_loc_ = geo_coord::geo_UTM();
  utm_vel_ = vnl_vector_fixed< double, 2 > ();
  if ( h.is_valid() && image_object( iobj ) )
  {
    plane_to_utm_homography::transform_t const& trans = h.get_transform();
    vgl_homg_point_2d< double > l( loc_[0], loc_[1] );
    vgl_homg_point_2d< double > pt = trans * l;
    utm_smoothed_loc_ = geo_coord::geo_UTM( h.get_dest_reference().zone(),
                                            h.get_dest_reference().is_north(),
                                            pt.x() / pt.w(), pt.y() / pt.w() );

    tracker_world_coord_type const& wloc = iobj->get_world_loc();
    l = vgl_homg_point_2d< double > ( wloc[0], wloc[1] );
    pt = trans * l;
    utm_raw_loc_ = geo_coord::geo_UTM( h.get_dest_reference().zone(),
                                       h.get_dest_reference().is_north(),
                                       pt.x() / pt.w(), pt.y() / pt.w() );
    l = vgl_homg_point_2d< double > ( vel_[0], vel_[1], 0 );
    pt = trans * l;
    utm_vel_[0] = pt.x();
    utm_vel_[1] = pt.y();
  }
}


bool track_state
::get_track_confidence(double & conf) const
{
  if (this->track_confidence_)
  {
    conf = *(this->track_confidence_);
    return true;
  }
  else
  {
    // Do NOT reset val when track_confidence is not available.
    return false;
  }
}

} // namespace vidtk
