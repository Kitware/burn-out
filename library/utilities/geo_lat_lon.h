/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_GEO_LAT_LON_H_
#define _VIDTK_GEO_LAT_LON_H_

#include <ostream>

#include <utilities/fuzzy_compare.h>

namespace vidtk {
namespace geo_coord {


// ----------------------------------------------------------------
/** Geo-coordinate as latitude / longitude.
 *
 * This class represents a Latitude / longitude geolocated point.
 *
 * The values for latitude and longitude are in degrees, but there is
 * no required convention for longitude. It can be either 0..360 or
 * -180 .. 0 .. 180. It is up to the application.  If a specific
 * convention must be enforced, make a subclass.
 */
class geo_lat_lon
{
public:
  static const double INVALID;    // used to indicate uninitialized value

  geo_lat_lon();
  geo_lat_lon( double lat, double lon );

  geo_lat_lon& set_latitude( double l );
  geo_lat_lon& set_longitude( double l );
  geo_lat_lon& set_latitude_longitude(double lat, double lon);
  double get_latitude() const;
  double get_longitude() const;

  bool is_empty() const;
  bool is_valid() const;

  bool operator==( const geo_lat_lon& rhs ) const;
  bool operator!=( const geo_lat_lon& rhs ) const;
  geo_lat_lon operator=( const geo_lat_lon& l );

  bool is_equal(const geo_lat_lon& rhs,
    double epsilon = fuzzy_compare<double>::epsilon()) const;

private:
  double lat_;
  double lon_;
};


std::ostream & operator<< (std::ostream & str, const ::vidtk::geo_coord::geo_lat_lon & obj);


} // end namespace
} // end namespace

#endif /* _VIDTK_GEO_LAT_LON_H_ */
