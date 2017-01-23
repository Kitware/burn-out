/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tot_writer.h"

#include <logger/logger.h>
#include <boost/concept_check.hpp>

namespace vidtk
{


VIDTK_LOGGER( "tot_writer" );


tot_writer
::tot_writer()
{
}


tot_writer
::~tot_writer()
{
  if( this->is_open() )
  {
    this->close();
  }
}


bool tot_writer
::write( std::vector<vidtk::track_sptr> const& tracks )
{
  if( !this->is_open() )
  {
    return false;
  }

  bool ret_val = true;

  for(std::vector<vidtk::track_sptr>::const_iterator iter = tracks.begin();
      iter != tracks.end(); ++iter)
  {
    if( ! *iter )
    {
      LOG_WARN( "NULL track pointer when writing TOT probabilities" );
      ret_val = false;
    }
    else if( !this->write(**iter) )
    {
      LOG_ERROR( "Failed to write TOT value to output" );
      return false;
    }
  }
  return ret_val;
}


/// Write the Target Object Type (TOT) probabilities associated with a track
bool tot_writer
::write( vidtk::track const& track )
{
  if( !this->is_open() )
  {
    return false;
  }

  vidtk::pvo_probability pvo;

  if( track.get_pvo( pvo ) )
  {
    fstr_ << track.id() << ' '
          << pvo.get_probability_person() << ' '
          << pvo.get_probability_vehicle() << ' '
          << pvo.get_probability_other() << std::endl;
  }
  else
  {
    LOG_WARN( "No TOT probabilities for track " << track.id() );
  }

  return true;
}


bool
tot_writer
::open( std::string const& fname )
{
  fstr_.open( fname.c_str() );

  if( !this->is_good() )
  {
    LOG_ERROR( "Could not open " << fname );
    return false;
  }

  return true;
}


bool
tot_writer
::is_open() const
{
 return this->is_good();
}


void
tot_writer
::close()
{
  fstr_.close();
}


bool
tot_writer
::is_good() const
{
  return fstr_.good();
}


}
