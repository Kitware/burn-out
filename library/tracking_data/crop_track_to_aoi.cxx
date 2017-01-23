/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "crop_track_to_aoi.h"

#include <utilities/geo_lat_lon.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_event_writing_process_cxx__
VIDTK_LOGGER("crop_track_to_aoi");

namespace vidtk
{
track_sptr
crop_track_to_aoi
::operator()( track_sptr tin, aoi_sptr a ) const
{
  if(!a->has_geo_points())
  {
    LOG_ERROR("Currently we only support geographic AOIs");
    return NULL;
  }
  track_sptr result = tin->get_subtrack_same_uuid( a->get_start_timestamp(), a->get_end_timestamp() );
  result->set_id(tin->id());
  std::vector< track_state_sptr > inside;
  for( std::vector<track_state_sptr>::const_iterator iter = result->history().begin(); iter != result->history().end(); ++iter )
  {
    track_state_sptr ts = *iter;
    switch(mode_)
    {
      case aoi::UTM:
      {
        geo_coord::geo_UTM geo;
        ts->get_smoothed_loc_utm( geo );
        if(geo.is_valid())
        {
          int zone = geo.get_zone();
          bool is_north = geo.is_north();
          if( a->point_inside( mode_, geo.get_easting(), geo.get_northing(), &zone, &is_north ) )
          {
            inside.push_back(ts);
          }
        }
        else
        {
          LOG_ERROR("Expecting UTM location");
        }
        break;
      }
      case aoi::LAT_LONG:
      {
        geo_coord::geo_lat_lon geo;
        if(ts->latitude_longitude( geo ))
        {
          if( a->point_inside( mode_, geo.get_latitude(), geo.get_longitude() ) )
          {
            inside.push_back(ts);
          }
        }
        else
        {
          LOG_ERROR("Expecting lat lon location");
        }
        break;
      }
      case aoi::PIXEL:
      {
        double x,y;
        if(ts->get_image_loc( x, y ))
        {
          if( a->point_inside( mode_, x, y ) )
          {
            inside.push_back(ts);
          }
        }
        else
        {
          LOG_ERROR("Expecting pixel location");
        }
      }
    }
  }
  if(inside.empty())
  {
    return NULL;
  }
  result->reset_history(inside);
  return result;
}

}
