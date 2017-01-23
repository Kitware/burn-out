/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "geo_coordinate.h"

#include <geographic/geo_coords.h>
#include <vbl/vbl_smart_ptr.hxx>

namespace vidtk {
namespace  geo_coord {

// ----------------------------------------------------------------------------
geo_coordinate
::geo_coordinate()
  : m_geo_lat_lon(),
    m_geo_MGRS(),
    m_geo_UTM()
{ }

// ----------------------------------------------------------------------------
geo_coordinate
::geo_coordinate( geo_coordinate_sptr g )
  : m_geo_lat_lon( g->m_geo_lat_lon ),
    m_geo_MGRS( g->m_geo_MGRS ),
    m_geo_UTM( g->m_geo_UTM )
{}

// ----------------------------------------------------------------------------
geo_coordinate
::geo_coordinate( geo_MGRS const& mgrs )
  : m_geo_lat_lon(),
    m_geo_MGRS( mgrs.get_coord() ),
    m_geo_UTM()
{ }

// ----------------------------------------------------------------------------
geo_coordinate
::geo_coordinate( geo_UTM const& u)
  : m_geo_lat_lon(),
    m_geo_MGRS(),
    m_geo_UTM( u.get_zone(), u.is_north(), u.get_easting(), u.get_northing() )
{ }

// ----------------------------------------------------------------------------
geo_coordinate
::geo_coordinate( geo_lat_lon const& ll)
  : m_geo_lat_lon( ll.get_latitude(), ll.get_longitude() ),
    m_geo_MGRS(),
    m_geo_UTM()
{ }

// ----------------------------------------------------------------------------
geo_coordinate
::geo_coordinate( int zone, bool is_north, double easting, double northing )
  : m_geo_lat_lon(),
    m_geo_MGRS(),
    m_geo_UTM( zone, is_north, easting, northing )
{}

// ----------------------------------------------------------------------------
geo_coordinate
::geo_coordinate( double lat, double lon )
  : m_geo_lat_lon( lat, lon ),
    m_geo_MGRS(),
    m_geo_UTM()
{}

// ----------------------------------------------------------------------------
geo_coordinate
::~geo_coordinate()
{

}

// ----------------------------------------------------------------------------
geo_UTM const&
geo_coordinate
::get_utm()
{
  if( !m_geo_UTM.is_valid() )
  {
    // Do we have latlon?
    if( m_geo_lat_lon.is_valid() )
    {
      convert_to_UTM( m_geo_lat_lon );
    }
    // We have to have MGRS since we don't allow empty construction
    else if (m_geo_MGRS.is_valid())
    {
      convert_to_UTM( m_geo_MGRS );
    }
  }

  return m_geo_UTM;
}

// ----------------------------------------------------------------------------
geo_lat_lon const&
geo_coordinate
::get_lat_lon()
{
  if( !m_geo_lat_lon.is_valid() )
  {
    // Do we have UTM?
    if( m_geo_UTM.is_valid() )
    {
      convert_to_lat_lon( m_geo_UTM );
    }
    // We have to have MGRS since we don't allow empty construction
    else if (m_geo_MGRS.is_valid())
    {
      convert_to_lat_lon( m_geo_MGRS );
    }
  }

  return m_geo_lat_lon;
}

// ----------------------------------------------------------------------------
geo_MGRS const&
geo_coordinate
::get_mgrs()
{
  if( !m_geo_MGRS.is_valid() )
  {
    // Do we have UTM?
    if( m_geo_UTM.is_valid() )
    {
      convert_to_MGRS( m_geo_UTM );
    }
    // We have to have MGRS since we don't allow empty construction
    else if (m_geo_lat_lon.is_valid())
    {
      convert_to_MGRS( m_geo_lat_lon );
    }
  }

  return m_geo_MGRS;
}

// wrapper for specific types fields
// ----------------------------------------------------------------------------
double
geo_coordinate
::get_latitude()
{
  this->get_lat_lon();
  return m_geo_lat_lon.get_latitude();
}

// ----------------------------------------------------------------------------
double
geo_coordinate
::get_longitude()
{
  this->get_lat_lon();
  return m_geo_lat_lon.get_longitude();
}

// ----------------------------------------------------------------------------
std::string const&
geo_coordinate
::get_mgrs_coord()
{
  this->get_mgrs();
  return m_geo_MGRS.get_coord();
}

void
geo_coordinate
::update_utm( geo_UTM const& utm )
{
  m_geo_UTM = utm;
  if (m_geo_lat_lon.is_valid())
  {
    convert_to_lat_lon( m_geo_UTM );
  }
  if (m_geo_MGRS.is_valid())
  {
    convert_to_MGRS(m_geo_UTM);
  }
}

// private functions
// ----------------------------------------------------------------------------
void
geo_coordinate
::convert_to_lat_lon( geo_MGRS const& coord )
{
  // convert MGRS to lat_lon
  geographic::geo_coords temp( coord.get_coord() );

  m_geo_lat_lon = geo_lat_lon( temp.latitude(),
                      temp.longitude() );

}


// ----------------------------------------------------------------------------
void
geo_coordinate
::convert_to_lat_lon( geo_UTM const& coord )
{
  // convert UTM to lat_lon
  geographic::geo_coords temp( coord.get_zone(),
                               coord.is_north(),
                               coord.get_easting(),
                               coord.get_northing() );

  m_geo_lat_lon = geo_lat_lon( temp.latitude(),
                      temp.longitude() );
}


// ----------------------------------------------------------------------------
void
geo_coordinate
::convert_to_UTM( geo_lat_lon const& coord )
{
  geographic::geo_coords temp( coord.get_latitude(), coord.get_longitude() );

  m_geo_UTM = geo_UTM( temp.zone(),
                       temp.is_north(),
                       temp.easting(),
                       temp.northing() );
}


// ----------------------------------------------------------------------------
void
geo_coordinate
::convert_to_UTM( geo_MGRS const& coord )
{
  geographic::geo_coords temp( coord.get_coord() );

  m_geo_UTM = geo_UTM( temp.zone(),
                       temp.is_north(),
                       temp.easting(),
                       temp.northing() );
}


// ----------------------------------------------------------------------------
void
geo_coordinate
::convert_to_MGRS( geo_lat_lon const& coord )
{
  geographic::geo_coords temp( coord.get_latitude(), coord.get_longitude() );
  m_geo_MGRS = geo_MGRS( temp.mgrs_representation() );
}


// ----------------------------------------------------------------------------
void
geo_coordinate
::convert_to_MGRS( geo_UTM const& coord )
{
  geographic::geo_coords temp( coord.get_zone(),
                               coord.is_north(),
                               coord.get_easting(),
                               coord.get_northing() );

  m_geo_MGRS = geo_MGRS( temp.mgrs_representation() );
}

// ----------------------------------------------------------------------------
bool
geo_coordinate
::is_valid()
{
  return (m_geo_UTM.is_valid() || m_geo_lat_lon.is_valid() || m_geo_MGRS.is_valid());
}

geo_coordinate
geo_coordinate
::operator=( const geo_coordinate& gc )
{
  if ( this != & gc )
  {
    this->m_geo_lat_lon = gc.m_geo_lat_lon;
    this->m_geo_MGRS = gc.m_geo_MGRS;
    this->m_geo_UTM = gc.m_geo_UTM;
  }

  return *this;
}

} // end namespace
} // end namespace

VBL_SMART_PTR_INSTANTIATE( vidtk::geo_coord::geo_coordinate );
