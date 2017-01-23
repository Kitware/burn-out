/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_reader_interface.h"

#include <fstream>
#include <boost/foreach.hpp>


namespace  vidtk {
namespace ns_track_reader {


track_reader_interface
::track_reader_interface()
{ }


track_reader_interface
::~track_reader_interface()
{ }


void track_reader_interface
::update_options( vidtk::ns_track_reader::track_reader_options const& opt )
{
  this->reader_options_.update_from( opt );
}


void track_reader_interface
::sort_terminated( vidtk::track::vector_t& tracks )
{
  BOOST_FOREACH (vidtk::track_sptr trk, tracks )
  {
    // skip tracks that have no history
    if ( trk->history().empty() )
    {
      continue;
    }

    // Add this track to list that terminates on tracks last frame+1.
    //
    // With the real tracker, tracks are terminated some number of
    // frames after the last state in the track. This is because of
    // track termination window. In this case, we are terminating the
    // track on the next frame after the last state to prevent a track
    // from being published by the track reader on the terminated
    // tracks port and the active tracks port on the same cycle.
    // Don't rely on the size of the track termination window!
    terminated_at_[trk->last_state()->time_.frame_number() + 1].push_back( trk );
  } // end for

  // Set iterator to first group to return.
  current_terminated_ = terminated_at_.begin();
}


// ----------------------------------------------------------------
/** Look for file type marker.
 *
 * Look for a string in the first part of a file.
 *
 * @param[in] filename Name of file to inspect
 * @param[in] nbytes Number of bytes to read from file
 * @param[in] text Marker text to look for
 */
bool track_reader_interface
::file_contains( std::string const& filename,
                 std::string const& text,
                 size_t            nbytes)
{
  // open file
  std::ifstream fstr( filename.c_str() );
  if( ! fstr )
  {
    return false;               // not valid file
  }

  // Allocate requested size and fill. Add one for string end.
  std::string data( nbytes+1, 0 );

  // Read specified number of bytes. It is possible that the read can
  // not get the requested number of bytes, but the back fill of zeros
  // should eliminate any false compares.
  fstr.read( &data[0], nbytes );

  return (data.find( text ) != std::string::npos);
}


} // end namespace
} // end namespace
