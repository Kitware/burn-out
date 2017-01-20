/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_writer_interface.h"
#include <vbl/vbl_smart_ptr.txx>

namespace vidtk
{

track_writer_interface::
track_writer_interface( )
{}


track_writer_interface::
~track_writer_interface()
{}


// ----------------------------------------------------------------
bool
track_writer_interface::
write( track_sptr const track )
{
  if ( ! track)
  {
    return false;
  }

  return write(*track);
}


// ----------------------------------------------------------------
bool
track_writer_interface::
write( image_object_sptr const io, timestamp const& ts )
{
  if ( ! io )
  {
    return false;
  }

  return write(*io, ts);
}


// ----------------------------------------------------------------
bool
track_writer_interface::
write( image_object const& /*io*/, timestamp const& /*ts*/)
{
  // Format does not write image objects.  Returns true because that is expected.
  // Used in with write( std::vector<vidtk::track_sptr> const& tracks,
  //                    std::vector<vidtk::image_object_sptr> const* ios,
  //                    timestamp const& ts )
  // For some formats.
  return true;
}


// ----------------------------------------------------------------
bool
track_writer_interface
::write( std::vector<vidtk::track_sptr> const& tracks )
{
  for(std::vector<vidtk::track_sptr>::const_iterator iter = tracks.begin(); iter != tracks.end(); ++iter)
  {
    if(!this->write(*iter))
    {
      return false;
    }
  }
  return true;
}


// ----------------------------------------------------------------
bool
track_writer_interface
::write( std::vector<vidtk::image_object_sptr> const& ios, timestamp const& ts )
{
  for(std::vector<vidtk::image_object_sptr>::const_iterator iter = ios.begin(); iter != ios.end(); ++iter)
  {
    if(!this->write(*iter, ts))
    {
      return false;
    }
  }
  return true;
}


// ----------------------------------------------------------------
bool
track_writer_interface
::write( std::vector<vidtk::track_sptr> const& tracks, std::vector<vidtk::image_object_sptr> const* ios, timestamp const& ts )
{
  if(!this->write(tracks))
  {
    return false;
  }
  if(ios != NULL && !this->write(*ios,ts))
  {
    return false;
  }
  return true;
}

}

VBL_SMART_PTR_INSTANTIATE( vidtk::track_writer_interface );
