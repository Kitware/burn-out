/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_state_util.h"


namespace vidtk {

// ------------------------------------------------------------------
geo_coord::geo_coordinate_sptr
get_lat_lon( vidtk::track_state_sptr ts, bool is_utm, int zone, bool is_north )
{
  double lat = geo_coord::geo_lat_lon::INVALID;
  double lon = geo_coord::geo_lat_lon::INVALID;
  ts->latitude_longitude( lat, lon );

  vidtk::image_object_sptr objs;
  if ( ( lat == geo_coord::geo_lat_lon::INVALID || lon == geo_coord::geo_lat_lon::INVALID ) &&
       ts->image_object( objs ) )
  {
    // Invalid default values to be written out to the file to
    // to indicate missing values.
    if( is_utm )
    {
      return new geo_coord::geo_coordinate( zone, is_north, objs->get_world_loc()[0], objs->get_world_loc()[1] );
    }
    else
    {
      return new geo_coord::geo_coordinate( objs->get_world_loc()[1], objs->get_world_loc()[0] );
    }
  }

  return new geo_coord::geo_coordinate( lat, lon );
}

} // end namespace
