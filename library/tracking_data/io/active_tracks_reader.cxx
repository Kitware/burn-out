/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "active_tracks_reader.h"

#include "active_tracks_generator.h"

#include <logger/logger.h>

VIDTK_LOGGER ("active_tracks_reader");

namespace vidtk
{


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
active_tracks_reader
::active_tracks_reader(track_reader * r)
  : reader_(r),
    generator_(0),
    tracks_loaded_(false)
{
}


active_tracks_reader
:: ~active_tracks_reader()
{
  //reader_ is external so is not deleted here
  delete generator_;
  generator_ = 0;
}


// ----------------------------------------------------------------
/* Read active tracks.
 *
 * This method returns the vector of active tracks for the current
 * frame.
 *
 * @param[out] trks Active tracks are returned here.
 *
 * @retval true Active tracks are returned
 * @retval false No more active tracks, stop calling.
 */
bool
active_tracks_reader
::read_active(unsigned const frame, vidtk::track_vector_t & trks)
{
  // Make sure tracks are loaded.  Returns quickly if not loaded
  if ( ! load_tracks() )
  {
    return false;
  }
  // ask the generator for our tracks
  return generator_->active_tracks_for_frame(frame, trks);
}


bool
active_tracks_reader
::read_terminated(unsigned const frame, vidtk::track_vector_t & trks)
{
  // Make sure tracks are loaded.  Returns quickly if not loaded
  if ( ! load_tracks() )
  {
    return false;
  }
  // ast the generator for our tracks
  return generator_->terminated_tracks_for_frame(frame, trks);
}


bool
active_tracks_reader
::get_timestamp( unsigned const frame, timestamp & ts )
{
  // Make sure tracks are loaded.  Returns quickly if not loaded
  if ( ! load_tracks() )
  {
    return false;
  }
  return generator_->timestamp_for_frame( frame, ts );
}

unsigned
active_tracks_reader
::first_frame()
{
  // Make sure tracks are loaded.  Returns quickly if not loaded
  if ( ! load_tracks() )
  {
    return 0;
  }
  return generator_->first_frame();
}

unsigned
active_tracks_reader
::last_frame()
{
  // Make sure tracks are loaded.  Returns quickly if not loaded
  if ( ! load_tracks() )
  {
    return 0;
  }
  return generator_->last_frame();
}

// ----------------------------------------------------------------
/** Load tracks from file and create active track views.
 *
 *
 */
bool
active_tracks_reader
::load_tracks()
{
  if ( tracks_loaded_ )
  {
    return true;
  }

  // start with a new set of active tracks.
  delete generator_;
  generator_ = new active_tracks_generator();

  /// storage for full (terminated) tracks
  vidtk::track_vector_t full_tracks;


  // some readers need more than one call to get all tracks.
  // e.g. vsl_reader
  int count(0);
  while (1)
  {
    vidtk::track_vector_t new_tracks;

    // use decorated reader to get the tracks.
    if (reader_->read_all(new_tracks) == 0)
    {
      break;
    }
    count++; // count successful calls to reader.

    full_tracks.insert (full_tracks.end(), new_tracks.begin(), new_tracks.end());
  } // end while

  // Doing the test this way, there is a possibility that there were
  // no tracks at all to read, but then why would we get an empty
  // tracks file?
  if (0 == count)
  {
    LOG_ERROR ("No tracks read from input file");
    return false;
  }

  try
  {
    // pass tracks to generator
    generator_->add_tracks (full_tracks);
    generator_->create_active_tracks();
    generator_->create_terminated_tracks();
  }
  catch (std::runtime_error const& e)
  {
    LOG_ERROR ("Exception caught while creating active tracks: " << e.what() );
    return false;
  }

  tracks_loaded_ = true;
  return true;
}

} // end namespace
