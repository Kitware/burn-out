/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "aoi.h"

#include <vbl/vbl_smart_ptr.txx>
#include <cstdlib>
#include <utilities/video_metadata.h>

#include <logger/logger.h>
VIDTK_LOGGER("aoi_cxx");

namespace vidtk
{

aoi::aoi() :
  aoi_id_( -1 ),
  mission_id_( -1 ),
  aoi_name_( "" ),
  aoi_crop_string_( "" ),
  start_time_( 0.0 ),
  end_time_( 0.0 ),
  start_frame_( 0 ),
  end_frame_( 0 ),
  start_timestamp_( timestamp() ),
  end_timestamp_( timestamp() ),
  description_( "" )
{
}

aoi::~aoi()
{
}

vxl_int_32
aoi
::get_aoi_id() const
{
  return aoi_id_;
}

int
aoi
::get_mission_id() const
{
  return mission_id_;
}

std::string
aoi
:: get_aoi_name() const
{
  return aoi_name_;
}

std::string
aoi
::get_aoi_crop_string() const
{
  return aoi_crop_string_;
}

const vnl_vector_fixed<int, 2> &
aoi
::get_aoi_crop_wh() const
{
  return aoi_crop_wh_;
}

const vnl_vector_fixed<int, 2> &
aoi
::get_aoi_crop_xy() const
{
  return aoi_crop_xy_;
}

double
aoi
::get_latitude( aoi_point point ) const
{
  return world_corner_points_[point].latitude();
}

double
aoi
::get_longitude( aoi_point point ) const
{
  return world_corner_points_[point].longitude();
}

double
aoi
::get_start_time() const
{
  return start_time_;
}

double
aoi
::get_end_time() const
{
  return end_time_;
}

const timestamp &
aoi
::get_start_timestamp() const
{
  return start_timestamp_;
}

const timestamp &
aoi
::get_end_timestamp() const
{
  return end_timestamp_;
}

unsigned
aoi
::get_start_frame() const
{
  return start_frame_;
}

unsigned
aoi
::get_end_frame() const
{
  return end_frame_;
}

std::string
aoi
::get_description() const
{
  return description_;
}

bool
aoi
::has_geo_points() const
{
  return world_corner_points_[UPPER_LEFT].is_valid() &&
         world_corner_points_[UPPER_RIGHT].is_valid() &&
         world_corner_points_[LOWER_RIGHT].is_valid() &&
         world_corner_points_[LOWER_LEFT].is_valid();
}

bool
aoi
::point_inside( intersection_mode mode, double x, double y, int * zone, bool * is_north ) const
{
  static bool warning_been_issued = false;
  switch(mode)
  {
    case PIXEL:
      return x >=aoi_crop_xy_[0] && x <= aoi_crop_xy_[0]+aoi_crop_wh_[0] &&
             y >=aoi_crop_xy_[1] && y <= aoi_crop_xy_[1]+aoi_crop_wh_[1];

    case LAT_LONG:
      //For now we will do the simple test and assume the aoi will be rectangular relative to lat long
      return x <= world_corner_points_[UPPER_LEFT].latitude() && x >= world_corner_points_[LOWER_RIGHT].latitude() &&
             y >=  world_corner_points_[UPPER_LEFT].longitude() && y <=  world_corner_points_[LOWER_RIGHT].longitude();
    case UTM:
    {
      if((zone == NULL || is_north == NULL) && !warning_been_issued)
      {
        warning_been_issued = true;
        LOG_WARN("Performing a UTM point AOI intersection, but zone and is north was "
                 "not provided.  We are assuming the zone and is_north is the same as the AOI");
      }

      int tz = (zone == NULL)?world_corner_points_[UPPER_LEFT].zone():*zone;
      bool tin = (is_north == NULL)?world_corner_points_[UPPER_LEFT].is_north():*is_north;
      //For now we will do the simple test and assume the aoi will be rectangular relative to UTM
      return x >= world_corner_points_[UPPER_LEFT].easting() && x <= world_corner_points_[LOWER_RIGHT].easting() &&
             y <=  world_corner_points_[UPPER_LEFT].northing() && y >=  world_corner_points_[LOWER_RIGHT].northing() &&
             tz == world_corner_points_[UPPER_LEFT].zone() && tin == world_corner_points_[UPPER_LEFT].is_north();
    }
  };
  return false;
}


/*
  Setter Methods
*/

void
aoi
::set_aoi_id( vxl_int_32 i )
{
  this->aoi_id_ = i;
}

void
aoi
::set_mission_id( int m )
{
  this->mission_id_ = m;
}

void
aoi
::set_aoi_name( std::string n )
{
  this->aoi_name_ = n;
}

bool
aoi
::set_aoi_crop_string( std::string s )
{
  this->aoi_crop_string_ = s;

  int crop_i0;
  int crop_j0;
  unsigned int crop_ni;
  unsigned int crop_nj;

  //the crop string comes out of the database in the form
  //XxY+W+H.  We need to tease out the individual componenets for use.
  bool result = get_crop_region_from_string( aoi_crop_string_,
                                             crop_i0, crop_j0,
                                             crop_ni, crop_nj );

  vnl_vector_fixed<int, 2> crop_xy;
  crop_xy[0] = crop_i0;
  crop_xy[1] = crop_j0;
  set_aoi_crop_xy( crop_xy );

  vnl_vector_fixed<int, 2> crop_wh;
  crop_wh[0] = crop_ni;
  crop_wh[1] = crop_nj;
  set_aoi_crop_wh( crop_wh );
  return result;
}

void
aoi
::set_aoi_crop_xy( const vnl_vector_fixed<int, 2>& p  )
{
  this->aoi_crop_xy_ = p;
}

void
aoi
::set_aoi_crop_wh( const vnl_vector_fixed<int, 2>& p  )
{
  this->aoi_crop_wh_ = p;
}

void
aoi
::set_geo_point( aoi_point point, geographic::geo_coords geo )
{
  world_corner_points_[point] = geo;
}

void
aoi
::set_geo_point_lat_lon( aoi_point point, double lat, double lon )
{
  set_geo_point( point, geographic::geo_coords(lat,lon) );
}

void
aoi
::set_start_time( timestamp &t )
{
  this->start_time_ = t.time();
  this->start_frame_ = t.frame_number();
  this->start_timestamp_ = t;
}

void
aoi
::set_end_time( timestamp &t )
{
  this->end_time_ = t.time();
  this->end_frame_ = t.frame_number();
  this->end_timestamp_ = t;
}

void
aoi
::set_description( std::string d )
{
  this->description_ = d;
}


bool
aoi
::get_crop_region_from_string( std::string& crop_string,
                               int& crop_i0,
                               int& crop_j0,
                               unsigned int& crop_ni,
                               unsigned int& crop_nj )
{
  //the crop string comes out of the database in the form
  //XxY+W+H.  We need to tease out the individual componenets for use.

  // If the crop_string is set, initialize these.
  crop_i0 = unsigned(-1);
  crop_j0 = 0;
  crop_ni = 0;
  crop_nj = 0;

  vul_reg_exp re( "^([0-9]+)x([0-9]+)([+-][0-9]+)([+-][0-9]+)$" );

  if( ! re.find( crop_string ) )
  {
    LOG_ERROR( "Could not parse crop string " << crop_string << "");
    return false;
  }

  std::stringstream str;
  str << re.match(1) << " "
      << re.match(2) << " "
      << re.match(3) << " "
      << re.match(4);
  str >> crop_ni >> crop_nj >> crop_i0 >> crop_j0;
  LOG_INFO( "crop section ("<<crop_i0<<","<<crop_j0<<") -> ("<<crop_i0+crop_ni<<","<<crop_j0+crop_nj<<")");

  return true;
}

} // namespace vidtk

VBL_SMART_PTR_INSTANTIATE( vidtk::aoi );
