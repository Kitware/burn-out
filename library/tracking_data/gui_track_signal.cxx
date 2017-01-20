/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "gui_track_signal.h"

namespace vidtk
{

bool gui_track_signal::operator==( const gui_track_signal& other ) const
{
  return ( this->message_type_ == other.message_type_ &&
           this->object_type_ == other.object_type_ &&
           this->ts_ == other.ts_ &&
           this->location_ == other.location_ &&
           this->qid_ == other.qid_ );
}

bool gui_track_signal::operator<( const gui_track_signal& other ) const
{
  return ( this->ts_ < other.ts_ );
}

bool gui_track_signal::operator>( const gui_track_signal& other ) const
{
  return ( this->ts_ > other.ts_ );
}

std::ostream& operator<<( std::ostream& str, const vidtk::gui_track_signal& obj )
{
  str << obj.ts_ << " " << obj.message_type_ << " ";

  if( obj.message_type_ == gui_track_signal::USER_CLICK )
  {
    str << obj.location_ << " " << obj.object_type_;
  }

  return str;
}

}
