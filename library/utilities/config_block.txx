/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/config_block.h>

#include <vcl_string.h>
#include <vcl_iostream.h>
#include <vcl_sstream.h>
#include <vcl_stdexcept.h>

/** \file
    \brief
    Code for configuration blocks.

*/


namespace vidtk
{

template<class T>
checked_bool
config_block
::set( vcl_string const& name, T const& val )
{
  // Convert to a string and store the string.

  vcl_ostringstream ostr;
  ostr << val;

  // Throw if the parameter couldn't be converted to a string.
  if( ! ostr )
  {
    return checked_bool( "failed to convert to a string representation" );
  }

  return this->set( name, ostr.str() );
}


template<class T>
checked_bool
config_block
::get( vcl_string const& name, T& val ) const
{
  vcl_string valstr;
  checked_bool res = this->get( name, valstr );
  if( ! res )
  {
    return res;
  }

  // Convert from string to T
  vcl_istringstream istr( valstr );
  istr >> val;
  if( ! istr )
  {
    return checked_bool( vcl_string("failed to convert from string representation for ")+name );
  }
  else
  {
    return true;
  }
}


template<class T, unsigned N>
checked_bool
config_block
::get( vcl_string const& name, T (&val)[N] ) const
{
  vcl_string valstr;
  checked_bool res = this->get( name, valstr );
  if( ! res )
  {
    return res;
  }

  // Read into a temporary array so that we don't modify the original
  // on failure.
  T tmpval[N];

  // Convert from string to T
  vcl_istringstream istr( valstr );
  unsigned i = 0;
  while( i < N && istr.good() )
  {
    istr >> tmpval[i];
    ++i;
  }
  if( ! istr )
  {
    return checked_bool( vcl_string("failed to convert from string representation for ")+name );
  }
  else
  {
    // make sure there aren't more elements than we expect
    T tmp;
    if( istr >> tmp )
    {
      return checked_bool( vcl_string("too many elements for ")+name );
    }
    else
    {
      for( unsigned i = 0; i < N; ++i )
      {
        val[i] = tmpval[i];
      }
      return true;
    }
  }
}


template<class T>
T
config_block
::get( vcl_string const& name ) const
{
  T val;
  checked_bool res = this->get( name, val );
  if( ! res )
  {
    throw config_block_parse_error( res.message() );
  }
  return val;
}


template<class T>
config_block&
config_block
::set_value( vcl_string const& name, T const& val )
{
  this->set( name, val );
  return *this;
}


} // end namespace vitk
