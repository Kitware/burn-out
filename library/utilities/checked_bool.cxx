/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/checked_bool.h>

#include <vcl_iostream.h>
#include <utilities/unchecked_return_value.h>

namespace vidtk
{

checked_bool
::checked_bool( bool v )
  : value_( v ),
    checked_( false ),
    msg_("Unchecked return value")
{
}


checked_bool
::checked_bool( vcl_string const& failure_msg )
  : value_( false ),
    checked_( false ),
    msg_( failure_msg )
{
}


checked_bool
::checked_bool( char const* failure_msg )
  : value_( false ),
    checked_( false ),
    msg_( failure_msg )
{
}


checked_bool
::checked_bool( checked_bool const& other )
{
  *this = other;
}


checked_bool&
checked_bool
::operator=( checked_bool const& other )
{
  value_ = other.value_;
  msg_ = other.msg_;
  checked_ = false;

  // Absolve other from the checking responsibility.
  other.checked_ = true;

  return *this;
}


checked_bool
::~checked_bool()
{
  if( value_ == false && ! checked_ )
  {
    throw unchecked_return_value( message() );
  }
}


checked_bool::
operator bool() const
{
  checked_ = true;
  return value_;
}


vcl_string const&
checked_bool
::message() const
{
  return msg_;
}


} // end namespace vidtk
