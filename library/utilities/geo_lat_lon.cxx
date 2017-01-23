/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "geo_lat_lon.h"

#include <cmath>
#include <iomanip>

namespace vidtk {
namespace geo_coord {

const double geo_lat_lon::INVALID = 444.0;

geo_lat_lon
::geo_lat_lon()
  : lat_(INVALID),
    lon_(INVALID)
{ }

geo_lat_lon::
geo_lat_lon(double lat, double lon)
  : lat_(lat),
    lon_(lon)
{ }


geo_lat_lon&
geo_lat_lon
::set_latitude(double l)
{
  lat_ = l;
  return ( *this );
}


geo_lat_lon&
geo_lat_lon
::set_longitude(double l)
{
  lon_ = l;
  return ( *this );
}


double
geo_lat_lon
::get_latitude() const
{
  return ( lat_ );
}


double
geo_lat_lon
::get_longitude() const
{
  return ( lon_ );
}


bool
geo_lat_lon
::is_valid() const
{
  bool valid = true;
  if (!(lat_ >= -90 && lat_ <= 90))
  {
    valid = false;
  }
  else if (!(lon_ >= -180 && lon_ <= 180))
  {
    valid = false;
  }

  return valid;
}


bool
geo_lat_lon
::is_empty() const
{
  return (INVALID == get_latitude() && INVALID == get_longitude());
}


bool
geo_lat_lon
::operator == ( const geo_lat_lon &rhs ) const
{
  return this->is_equal(rhs, 0.0);
}

bool
geo_lat_lon
::operator != ( const geo_lat_lon &rhs ) const
{
  return ( !( this->operator == ( rhs ) ) );
}

bool
geo_lat_lon::is_equal(const geo_lat_lon &rhs, double epsilon) const
{
  return fuzzy_compare<double>::compare(
    this->get_latitude(), rhs.get_latitude(), epsilon)
    && fuzzy_compare<double>::compare(
      this->get_longitude(), rhs.get_longitude(), epsilon);
}

geo_lat_lon
geo_lat_lon
::operator=( const geo_lat_lon& l )
{
  if ( this != & l )
  {
    this->lat_ = l.get_latitude();
    this->lon_ = l.get_longitude();
  }

  return *this;
}

std::ostream & operator<< (std::ostream & str, const ::vidtk::geo_coord::geo_lat_lon & obj)
{
  std::streamsize old_prec = str.precision();

  str << std::setprecision(22)
      << "[ " << obj.get_latitude()
      << " / " << obj.get_longitude()
      << " ]";

  str.precision( old_prec );
  return str;
}

} // end namespace
} // end namespace
