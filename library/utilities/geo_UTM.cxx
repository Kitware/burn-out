/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "geo_UTM.h"

#include <iomanip>

namespace vidtk {
namespace  geo_coord {

geo_UTM
::geo_UTM()
  : zone_ (-100),
    is_north_ (true),
    easting_(0),
    northing_(0)
{ }

geo_UTM
::geo_UTM(int zone, bool north, double easting, double northing)
  : zone_ (zone),
    is_north_ (north),
    easting_ (easting),
    northing_ (northing)
{ }


bool
geo_UTM
::is_empty() const
{
  return (this->zone_ == -100);
}


bool
geo_UTM
::is_valid() const
{
  if (is_empty())
  {
    return false;
  }

  // Zone (1,60) (-1,-60)
  if ( ! ( ((zone_ >= -60) && (zone_ <= -1))
           || ((zone_ >= 1) && (zone_ <= 60)) ) )
  {
    return false;
  }

  //The magic numbers are from
  // The Universal Transverse Mercator (UTM) Grid System and Topographic Maps
  //    An Introductory Guide for Scientists & Engineers
  //    By: Joe S. Depner
  //    2011 Dec 15 Edition
  if ((easting_ < 166042) || (easting_ > 833958))
  {
    return false;
  }

  //The magic numbers are from
  // The Universal Transverse Mercator (UTM) Grid System and Topographic Maps
  //    An Introductory Guide for Scientists & Engineers
  //    By: Joe S. Depner
  //    2011 Dec 15 Edition
  if ((northing_ < 1094440) || (northing_ > 10000000))
  {
    return false;
  }

  return true;
}


geo_UTM&
geo_UTM
::set_zone(int z)
{
  zone_ = z;
  return *this;
}


geo_UTM&
geo_UTM
::set_is_north(bool v)
{
  is_north_ = v;
  return *this;
}


geo_UTM&
geo_UTM
::set_easting(double v)
{
  easting_ = v;
  return *this;
}


double
geo_UTM
::get_easting() const
{
  return easting_;
}


geo_UTM&
geo_UTM
::set_northing(double v)
{
  northing_ = v;
  return *this;
}


double
geo_UTM
::get_northing() const
{
  return northing_;
}

int geo_UTM
::get_zone() const
{
  return zone_;
}


bool
geo_UTM
::is_north() const
{
  return is_north_;
}


bool
geo_UTM
::operator == ( const geo_UTM &rhs ) const
{
  return ( (rhs.get_zone() == this->get_zone() )
           && (rhs.is_north() == this->is_north() )
           && (rhs.get_easting() == this->get_easting() )
           && (rhs.get_northing() == this->get_northing() ) );
}


bool
geo_UTM
::operator != ( const geo_UTM &rhs ) const
{
  return ( !( this->operator == ( rhs ) ) );
}

geo_UTM
geo_UTM
::operator=( const geo_UTM& u )
{
  if ( this != & u )
  {
    this->zone_ = u.zone_;
    this->is_north_ = u.is_north_;
    this->easting_ = u.easting_;
    this->northing_ = u.northing_;
  }

  return *this;
}

std::ostream & operator<< (std::ostream & str, const ::vidtk::geo_coord::geo_UTM & obj)
{
  str << std::setprecision(16)
      << "[UTM - Z: " << obj.get_zone()
      << (obj.is_north() ? " (North)" : " (South)")
      << " Easting: " << obj.get_easting()
      << " Northing: " << obj.get_northing()
      << " ]";

  return str;
}


} // end namespace
} // end namespace
