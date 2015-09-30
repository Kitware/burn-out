/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_limits.h>
#include <GeographicLib/GeoCoords.hpp>
#include "geo_coords.h"

namespace vidtk
{
namespace geographic
{

struct geo_coords_impl
{
  geo_coords_impl(void)
  : is_valid(true)
  { }

  bool is_valid;
  GeographicLib::GeoCoords c;
};


geo_coords
::geo_coords(void)
: impl_(0)
{
  impl_ = new geo_coords_impl;
}


geo_coords
::geo_coords(const vcl_string &s)
: impl_(0)
{
  impl_ = new geo_coords_impl;

  this->reset(s);
}


geo_coords
::geo_coords(double latitude, double longitude)
: impl_(0)
{
  impl_ = new geo_coords_impl;

  this->reset(latitude, longitude);
}


geo_coords
::geo_coords(int zone, bool is_north, double easting, double northing)
: impl_(0)
{
  impl_ = new geo_coords_impl;

  this->reset(zone, is_north, easting, northing);
}


// Copy constructor
geo_coords::
geo_coords (geo_coords const & obj)
{
  // do a deep copy
  this->impl_ = new geo_coords_impl();
  *this->impl_ = *obj.impl_;
}


// assignment operator
geo_coords & geo_coords::
operator= (geo_coords const & obj)
{
  if (this != &obj)
  {
    *this->impl_ = *obj.impl_;
  }

  return ( *this );
}


geo_coords::
~geo_coords()
{
  delete impl_;
  impl_ = 0;
}


bool
geo_coords
::reset(const std::string& s)
{
  try
  {
    this->impl_->c.Reset(s);
  }
  catch(const GeographicLib::GeographicErr &ex)
  {
    return this->impl_->is_valid = false;
  }
  return this->impl_->is_valid = true;
}


bool
geo_coords
::reset(double latitude, double longitude)
{
  try
  {
    this->impl_->c.Reset(latitude, longitude);
  }
  catch(const GeographicLib::GeographicErr &ex)
  {
    return this->impl_->is_valid = false;
  }
  return this->impl_->is_valid = true;
}


bool
geo_coords
::reset(int zone, bool is_north, double easting, double northing)
{
  try
  {
    this->impl_->c.Reset(zone, is_north, easting, northing);
  }
  catch(const GeographicLib::GeographicErr &ex)
  {
    return this->impl_->is_valid = false;
  }
  return this->impl_->is_valid = true;
}


double
geo_coords
::latitude(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Latitude() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::longitude(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Longitude() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::easting(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Easting() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::northing(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Northing() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::convergence(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Convergence() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::scale(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Scale() :
                                vcl_numeric_limits<double>::max();
}


bool
geo_coords
::is_north(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Northp() : false;
}


char
geo_coords
::hemisphere(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Hemisphere() : '\0';
}


int
geo_coords
::zone(void) const
{
  return this->impl_->is_valid ? this->impl_->c.Zone() :
                                vcl_numeric_limits<int>::max();
}


bool
geo_coords
::set_alt_zone(int zone)
{
  try
  {
    this->impl_->c.SetAltZone(zone);
  }
  catch(const GeographicLib::GeographicErr &ex)
  {
    return false;
  }
  return true;
}


int
geo_coords
::alt_zone(void) const
{
  return this->impl_->is_valid ? this->impl_->c.AltZone() :
                                vcl_numeric_limits<int>::max();
}


double
geo_coords
::alt_easting() const
{
  return this->impl_->is_valid ? this->impl_->c.AltEasting() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::alt_northing() const
{
  return this->impl_->is_valid ? this->impl_->c.AltNorthing() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::alt_convergence() const
{
  return this->impl_->is_valid ? this->impl_->c.AltConvergence() :
                                vcl_numeric_limits<double>::max();
}


double
geo_coords
::alt_scale() const
{
  return this->impl_->is_valid ? this->impl_->c.AltScale() :
                                vcl_numeric_limits<double>::max();
}


vcl_string
geo_coords
::geo_representation(int prec) const
{
  return this->impl_->is_valid ? this->impl_->c.GeoRepresentation(prec) : "";
}


vcl_string
geo_coords
::dms_representation(int prec) const
{
  return this->impl_->is_valid ? this->impl_->c.DMSRepresentation(prec) : "";
}


vcl_string
geo_coords
::mgrs_representation(int prec) const
{
  return this->impl_->is_valid ? this->impl_->c.MGRSRepresentation(prec) : "";
}

vcl_string
geo_coords
::utm_ups_representation(int prec) const
{
  return this->impl_->is_valid ? this->impl_->c.UTMUPSRepresentation(prec) : "";
}


vcl_string
geo_coords
::alt_mgrs_representation(int prec) const
{
  return this->impl_->is_valid ? this->impl_->c.AltMGRSRepresentation(prec) : "";
}


vcl_string
geo_coords
::alt_utm_ups_representation(int prec) const
{
  return this->impl_->is_valid ? this->impl_->c.AltUTMUPSRepresentation(prec):"";
}


double
geo_coords
::major_radius() const
{
  return this->impl_->c.MajorRadius();
}


double
geo_coords
::inverse_flattening() const
{
  return this->impl_->c.InverseFlattening();
}


bool
geo_coords
::is_valid(void) const
{
  return this->impl_->is_valid;
}

}
}

