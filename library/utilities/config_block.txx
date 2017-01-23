/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/config_block.h>

#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace vidtk
{

template<class T>
checked_bool
config_block
::set( std::string const& name, T const& val )
{
  // Convert to a string and store the string.
  std::ostringstream ostr;
  ostr << val;

  // Throw if the parameter couldn't be converted to a string.
  if( !ostr )
  {
    return checked_bool( "failed to convert to a string representation" );
  }

  return this->set( name, ostr.str() );
}


// Forward declare specializations
template<>
unsigned char
config_block::get<unsigned char>( std::string const& name ) const;
template<>
signed char
config_block::get<signed char>( std::string const& name ) const;
template<>
bool
config_block::get<bool>( std::string const& name ) const;
template<>
std::string
config_block::get<std::string>( std::string const& name ) const;
//

template<class T, unsigned N>
checked_bool
config_block
::get( std::string const& name, T (&val)[N] ) const
{
  return get<T>( name, val, N );
}

template<class T>
checked_bool
config_block
::get( std::string const& name, T* ptr, unsigned N ) const
{
  std::string valstr;

  checked_bool res = this->get_raw_value( name, valstr );

  if( ! res )
  {
    return res;
  }

  // Read into a temporary array so that we don't modify the original
  // on failure.
  std::vector<T> tmpval( N );

  // Convert from string to T
  std::istringstream istr( valstr );
  unsigned i = 0;
  while( i < N && !istr.fail() )
  {
    istr >> tmpval[i];
    ++i;
  }
  if( istr.fail() )
  {
    return checked_bool( std::string("failed to convert from string representation \""+valstr+"\" for ")+name );
  }
  else
  {
    // make sure there aren't more elements than we expect
    T tmp;
    if( istr >> tmp )
    {
      return checked_bool( std::string("too many elements for ")+name );
    }
    else
    {
      std::copy(tmpval.begin(), tmpval.end(), ptr);
      return true;
    }
  }
}

template<class T>
T
config_block
::get( std::string const& name ) const
{
  std::string valstr = this->get<std::string>( name );

  // Convert from string to T
  T val;
  std::istringstream istr( valstr );
  istr >> val;
  if( istr.fail() )
  {
    throw config_block_parse_error( "failed to convert from string representation \'" + valstr + "\' for " + name );
  }

  return val;
}


template <class T>
T
config_block
::get( config_enum_option const& table, std::string const& name ) const
{
  return static_cast< T >(get_enum_int( table, name) );
}


template<class T>
config_block&
config_block
::set_value( std::string const& name, T const& val )
{
  this->set( name, val );
  return *this;
}


} // end namespace vitk
