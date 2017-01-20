/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klv_to_metadata.h"

#include <klv/klv_parse.h>
#include <klv/klv_key.h>
#include <klv/klv_0601.h>
#include <klv/klv_0601_traits.h>
#include <klv/klv_0104.h>

#include <logger/logger.h>

#include <boost/optional/optional.hpp>

#include <utilities/video_metadata.h>

namespace vidtk
{
VIDTK_LOGGER("klv_to_metadata");

/// Returns true upon reading a valid 0601 klv packet
bool
klv_0601_to_metadata( const klv_data &klv, video_metadata &metadata )
{
  if ( !klv_0601_checksum( klv ) )
  {
    metadata.is_valid(false);
    return false;
  }

  klv_lds_vector_t lds = parse_klv_lds( klv );

  boost::optional<double> ul_lat_offset, ul_lon_offset, ur_lat_offset, ur_lon_offset,
                          lr_lat_offset, lr_lon_offset, ll_lat_offset, ll_lon_offset,
                          frame_center_lat, frame_center_lon, platform_loc_lat, platform_loc_lon;

  for ( klv_lds_vector_t::const_iterator itr = lds.begin(); itr != lds.end(); ++itr )
  {
    if ( ( itr->first <= KLV_0601_UNKNOWN ) || ( itr->first >= KLV_0601_ENUM_END ) )
    {
      continue;
    }

    const klv_0601_tag tag (static_cast< klv_0601_tag > ( vxl_byte( itr->first ) ));
    boost::any data = klv_0601_value( tag, &itr->second[0], itr->second.size() );

    switch (itr->first)
    {
    case KLV_0601_UNIX_TIMESTAMP:
      metadata.timeUTC(boost::any_cast<klv_0601_traits<KLV_0601_UNIX_TIMESTAMP>::type>(data));
      break;
    case KLV_0601_SENSOR_LATITUDE:
      platform_loc_lat = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_SENSOR_LONGITUDE:
      platform_loc_lon = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_SENSOR_TRUE_ALTITUDE:
      metadata.platform_altitude(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_PLATFORM_PITCH_ANGLE:
      metadata.platform_pitch(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_PLATFORM_ROLL_ANGLE:
      metadata.platform_roll(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_PLATFORM_HEADING_ANGLE:
      metadata.platform_yaw(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_SENSOR_REL_AZ_ANGLE:
      metadata.sensor_yaw(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_SENSOR_REL_EL_ANGLE:
      metadata.sensor_pitch(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_SENSOR_REL_ROLL_ANGLE:
      metadata.sensor_roll(klv_0601_value_double(tag, data));
      break;
    //Correpondences between PT # and corner are from 0601 spec
    case KLV_0601_OFFSET_CORNER_LAT_PT_1:
      ul_lat_offset = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_OFFSET_CORNER_LONG_PT_1:
      ul_lon_offset = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_OFFSET_CORNER_LAT_PT_2:
      ur_lat_offset = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_OFFSET_CORNER_LONG_PT_2:
      ur_lon_offset = klv_0601_value_double(tag, data);
      break;
    case  KLV_0601_OFFSET_CORNER_LAT_PT_3:
      lr_lat_offset = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_OFFSET_CORNER_LONG_PT_3:
      lr_lon_offset = klv_0601_value_double(tag, data);
      break;
    case  KLV_0601_OFFSET_CORNER_LAT_PT_4:
      ll_lat_offset = klv_0601_value_double(tag, data);
      break;
    case  KLV_0601_OFFSET_CORNER_LONG_PT_4:
      ll_lon_offset = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_SLANT_RANGE:
      metadata.slant_range(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_SENSOR_HORIZONTAL_FOV:
      metadata.sensor_horiz_fov(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_SENSOR_VERTICAL_FOV:
      metadata.sensor_vert_fov(klv_0601_value_double(tag, data));
      break;
    case KLV_0601_FRAME_CENTER_LAT:
      frame_center_lat = klv_0601_value_double(tag, data);
      break;
    case KLV_0601_FRAME_CENTER_LONG:
      frame_center_lon = klv_0601_value_double(tag, data);
      break;
    default:
      break;
    }
  }

  if (platform_loc_lat.is_initialized() && platform_loc_lon.is_initialized())
  {
    metadata.platform_location(vidtk::geo_coord::geo_lat_lon(platform_loc_lat.get(), platform_loc_lon.get()));
  }

  if (frame_center_lat.is_initialized() && frame_center_lon.is_initialized())
  {
    const double c_lat = frame_center_lat.get();
    const double c_lon = frame_center_lon.get();
    metadata.frame_center(vidtk::geo_coord::geo_lat_lon(c_lat, c_lon));

    if (ul_lat_offset.is_initialized() && ul_lon_offset.is_initialized())
    {
      metadata.corner_ul(geo_coord::geo_lat_lon(c_lat + ul_lat_offset.get(), c_lon + ul_lon_offset.get()));
    }
    if (ur_lat_offset.is_initialized() && ur_lon_offset.is_initialized())
    {
      metadata.corner_ur(geo_coord::geo_lat_lon(c_lat + ur_lat_offset.get(), c_lon + ur_lon_offset.get()));
    }
    if (lr_lat_offset.is_initialized() && lr_lon_offset.is_initialized())
    {
      metadata.corner_lr(geo_coord::geo_lat_lon(c_lat + lr_lat_offset.get(), c_lon + lr_lon_offset.get()));
    }
    if (ll_lat_offset.is_initialized() && ll_lon_offset.is_initialized())
    {
      metadata.corner_ll(geo_coord::geo_lat_lon(c_lat + ll_lat_offset.get(), c_lon + ll_lon_offset.get()));
    }
  }

  //If we got a valid packet then we set this to true, because we do not know
  //which fields are needed for each algorithm/data set here.
  metadata.is_valid(true);

  return true;
}


/// Returns true upon reading the desired klv packet
bool
klv_0104_to_metadata( const klv_data &klv, video_metadata &metadata )
{
  klv_uds_vector_t uds = parse_klv_uds( klv );

  boost::optional<double> ul_lat, ul_lon, ur_lat, ur_lon, lr_lat, lr_lon, ll_lat, ll_lon,
                          frame_center_lat, frame_center_lon, platform_loc_lat, platform_loc_lon;
  try
  {
    for ( klv_uds_vector_t::const_iterator itr = uds.begin(); itr != uds.end(); ++itr )
    {
      const klv_0104::tag tag = klv_0104::inst()->get_tag(itr->first);
      if ( tag >= klv_0104::UNKNOWN )
      {
        continue;
      }

      boost::any data = klv_0104::inst()->get_value(tag, &itr->second[0], itr->second.size());

      switch (tag)
      {
      case klv_0104::UNIX_TIMESTAMP:
        metadata.timeUTC(klv_0104::inst()->get_value<vxl_uint_64>(tag, data));
        break;
      case klv_0104::SENSOR_LATITUDE:
        platform_loc_lat = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::SENSOR_LONGITUDE:
        platform_loc_lon = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::SENSOR_ALTITUDE:
        metadata.platform_altitude(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::PLATFORM_PITCH_ANGLE:
        metadata.platform_pitch(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::PLATFORM_ROLL_ANGLE:
        metadata.platform_roll(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::PLATFORM_HEADING:
        metadata.platform_yaw(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::SENSOR_ROLL_ANGLE:
        metadata.sensor_roll(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::UPPER_LEFT_CORNER_LAT:
        ul_lat = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::UPPER_LEFT_CORNER_LON:
        ul_lon = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::UPPER_RIGHT_CORNER_LAT:
        ur_lat = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::UPPER_RIGHT_CORNER_LON:
        ur_lon = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case  klv_0104::LOWER_RIGHT_CORNER_LAT:
        lr_lat = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::LOWER_RIGHT_CORNER_LON:
        lr_lon = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case  klv_0104::LOWER_LEFT_CORNER_LAT:
        ll_lat = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case  klv_0104::LOWER_LEFT_CORNER_LON:
        ll_lon = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::SLANT_RANGE:
        metadata.slant_range(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::SENSOR_HORIZONTAL_FOV:
        metadata.sensor_horiz_fov(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::SENSOR_VERTICAL_FOV:
        metadata.sensor_vert_fov(klv_0104::inst()->get_value<double>(tag, data));
        break;
      case klv_0104::FRAME_CENTER_LATITUDE:
        frame_center_lat = klv_0104::inst()->get_value<double>(tag, data);
        break;
      case klv_0104::FRAME_CENTER_LONGITUDE:
        frame_center_lon = klv_0104::inst()->get_value<double>(tag, data);
        break;
      default:
        break;
      }
    }

    //Set lat/lon pairs only if we got both
    if (platform_loc_lat.is_initialized() && platform_loc_lon.is_initialized())
    {
      metadata.platform_location(vidtk::geo_coord::geo_lat_lon(platform_loc_lat.get(), platform_loc_lon.get()));
    }
    if (frame_center_lat.is_initialized() && frame_center_lon.is_initialized())
    {
      metadata.frame_center(vidtk::geo_coord::geo_lat_lon(frame_center_lat.get(), frame_center_lon.get()));
    }
    if (ul_lat.is_initialized() && ul_lon.is_initialized())
    {
      metadata.corner_ul(geo_coord::geo_lat_lon(ul_lat.get(), ul_lon.get()));
    }
    if (ur_lat.is_initialized() && ur_lon.is_initialized())
    {
      metadata.corner_ur(geo_coord::geo_lat_lon(ur_lat.get(), ur_lon.get()));
    }
    if (lr_lat.is_initialized() && lr_lon.is_initialized())
    {
      metadata.corner_lr(geo_coord::geo_lat_lon(lr_lat.get(), lr_lon.get()));
    }
    if (ll_lat.is_initialized() && ll_lon.is_initialized())
    {
      metadata.corner_ll(geo_coord::geo_lat_lon(ll_lat.get(), ll_lon.get()));
    }

    //If we got a valid packet then we set this to true, because we do not know
    //which fields are needed for each algorithm/data set here.
    metadata.is_valid(true);
  }
  catch (klv_0104::klv_exception &e)
  {
    LOG_ERROR(e.what());
    metadata.is_valid(false);
    return false;
  }

  return true;
}

} //end namespace vidtk
