/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "video_metadata_util.h"
#include "video_metadata.h"

#include <iomanip>
#include <string>

namespace vidtk {

// ----------------------------------------------------------------
// Serialization and deserialization are really properties of the data
// file not properties of the data structure, as illustrated by the
// multiple formats that follow.
// ----------------------------------------------------------------

#define SERIALIZE_GEO(G)                        \
  << G.get_latitude() << " "                    \
  << G.get_longitude() << " "

std::ostream&
ascii_serialize(std::ostream& str, vidtk::video_metadata const& vmd)
{
  str << std::setprecision(20);

  str << vmd.timeUTC() << " "
    SERIALIZE_GEO(vmd.platform_location())
      << vmd.platform_altitude() << " "
      << vmd.platform_roll() << " "
      << vmd.platform_pitch() << " "
      << vmd.platform_yaw() << " "
      << vmd.sensor_roll() << " "
      << vmd.sensor_pitch() << " "
      << vmd.sensor_yaw() << " "
    SERIALIZE_GEO(vmd.corner_ul())
    SERIALIZE_GEO(vmd.corner_ur())
    SERIALIZE_GEO(vmd.corner_lr())
    SERIALIZE_GEO(vmd.corner_ll())
      << vmd.slant_range() << " "
      << vmd.sensor_horiz_fov() << " "
      << vmd.sensor_vert_fov() << " "
    SERIALIZE_GEO(vmd.frame_center());

  return str;
}

#undef SERIALIZE_GEO


#define DESERIALIZE_GEO(G)                      \
  {                                             \
    double lat, lon;                            \
    geo_coord::geo_lat_lon temp;                \
    str >> lat >> lon;                          \
    temp.set_latitude(lat);                     \
    temp.set_longitude(lon);                    \
    G(temp);                                    \
  }


#define DESERIALIZE_DBL(G)                      \
{                                               \
  double temp;                                  \
  str >> temp;                                  \
  G(temp);                                      \
}


std::istream&
ascii_deserialize(std::istream& str, vidtk::video_metadata& vmd)
{
  vxl_uint_64 tutc;

  str >> tutc;
  vmd.timeUTC(tutc);
  DESERIALIZE_GEO(vmd.platform_location);
  DESERIALIZE_DBL(vmd.platform_altitude);
  DESERIALIZE_DBL(vmd.platform_roll);
  DESERIALIZE_DBL(vmd.platform_pitch);
  DESERIALIZE_DBL(vmd.platform_yaw);
  DESERIALIZE_DBL(vmd.sensor_roll);
  DESERIALIZE_DBL(vmd.sensor_pitch);
  DESERIALIZE_DBL(vmd.sensor_yaw);
  DESERIALIZE_GEO(vmd.corner_ul);
  DESERIALIZE_GEO(vmd.corner_ur);
  DESERIALIZE_GEO(vmd.corner_lr);
  DESERIALIZE_GEO(vmd.corner_ll);
  DESERIALIZE_DBL(vmd.slant_range);
  DESERIALIZE_DBL(vmd.sensor_horiz_fov);
  DESERIALIZE_DBL(vmd.sensor_vert_fov);
  DESERIALIZE_GEO(vmd.frame_center);

  return str;
}


std::istream&
ascii_deserialize_red_river( std::istream& str, vidtk::video_metadata& vmd )
{
  std::string dummys;
  double dummyd;
  double temp_timestamp = 0;
  double temp_plat_lat = 0;
  double temp_plat_lon = 0;
  double temp_plat_alt = 0;
  double temp_ground_alt = 0;

  str >> dummys >> dummys // 0-1
      >> dummyd >> dummyd     // 2-3
      >> temp_timestamp;     // 4
  vmd.timeUTC(static_cast< vxl_uint_64 > ( temp_timestamp * 1000000 ) );

  str >> dummyd >> dummyd; // 5-6
  DESERIALIZE_DBL(vmd.platform_roll); // 7
  DESERIALIZE_DBL(vmd.platform_pitch); // 8
  DESERIALIZE_DBL(vmd.platform_yaw); // 9

  str >> dummyd >> dummyd >> dummyd >> dummyd >> dummyd     // 10-14
      >> dummyd >> dummyd >> dummyd >> dummyd     // 15-18
      >> temp_plat_lon     // 19
      >> temp_plat_lat;     // 20

  geo_coord::geo_lat_lon lltemp(temp_plat_lat, temp_plat_lon );
  vmd.platform_location( lltemp);

  str >> temp_plat_alt // 21
      >> dummyd >> dummyd >> dummyd >> dummyd >> dummyd     // 22-26
      >> dummyd >> dummyd >> dummyd >> dummyd >> dummyd;     // 27-31

  DESERIALIZE_DBL(vmd.sensor_yaw); // 32
  DESERIALIZE_DBL(vmd.sensor_pitch); // 33
  DESERIALIZE_DBL(vmd.sensor_roll);  // 34

  str >> dummyd >> dummyd >> dummyd >> dummyd >> dummyd     // 35-39
      >> dummyd >> dummyd >> dummyd >> dummyd >> dummyd     // 40-44
      >> dummyd >> dummyd >> dummyd;     // 45-47
  DESERIALIZE_GEO( vmd.frame_center ); // 48, 49
  str >> temp_ground_alt; // 50

  const double feet_to_meters = 0.3048;
  vmd.platform_altitude( ( temp_plat_alt - temp_ground_alt ) * feet_to_meters );

  // The following fields are not provided in the Red River metadata format:
  // (if needed, they should be set by the calling function)
  // Slant range
  // Sensor horizontal field of view
  // Sensor vertical field of view

  return str;
} // ascii_deserialize_red_river

#undef DESERIALIZE_GEO
#undef DESERIALIZE_DBL

} // end namespace
