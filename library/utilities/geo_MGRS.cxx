/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "geo_MGRS.h"

#include <iomanip>

namespace vidtk {
namespace  geo_coord {


geo_MGRS
::geo_MGRS()
{ }


geo_MGRS
::geo_MGRS(std::string const& coord)
: mgrs_coord_(coord)
{ }


geo_MGRS
::~geo_MGRS()
{ }


bool
geo_MGRS
::is_empty() const
{
  return this->mgrs_coord_.empty();
}


bool
geo_MGRS
::is_valid() const
{
  if (is_empty())
  {
    return false;
  }

  // TODO - what constututes a valid MGRS?
  return true;
}


geo_MGRS &
geo_MGRS
::set_coord( std::string const& coord)
{
  this-> mgrs_coord_ = coord;
  return *this;
}



std::string const&
geo_MGRS
::get_coord() const
{
  return this->mgrs_coord_;
}

bool
geo_MGRS
::operator == ( const geo_MGRS &rhs ) const
{
  // May want to take into precision of operands.
  return ( rhs.get_coord() == this->get_coord() );
}


bool
geo_MGRS
::operator != ( const geo_MGRS &rhs ) const
{
  return ( !( this->operator == ( rhs ) ) );
}

geo_MGRS
geo_MGRS
::operator=( const geo_MGRS& m )
{
  if ( this != & m )
  {
    this->mgrs_coord_ = m.get_coord();
  }

  return *this;
}

std::ostream & operator<< (std::ostream & str, const ::vidtk::geo_coord::geo_MGRS & obj)
{
  str << "[MGRS: " << obj.get_coord() << "]";

  return str;
}

} // end namespace
} // end namespace
