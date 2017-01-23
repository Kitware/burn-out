/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_demultiplexer.h"

#include <tracking_data/tracking_keys.h>
#include <logger/logger.h>

#include <boost/foreach.hpp>

namespace vidtk {

VIDTK_LOGGER( "track_demultiplexer" );

track_demultiplexer::
track_demultiplexer()
{ }


track_demultiplexer::
~track_demultiplexer()
{ }


// ----------------------------------------------------------------
/** Add active tracks for the current frame.
 *
 * The list of active tracks is processed against the internal list of
 * previously active tracks. These tracks are processed as follows.
 *
 * If track is not in previous active track list, it is added to the
 * new-track list and added to the current active track list.
 *
 * If track's last state does not have current timestamp, it is added
 * to not-updated list.
 *
 * Otherwise, it is added to the updated list.
 *
 * Terminated tracks are those tracks that are in the local (last) active
 * track list that are not in the current active track list.
 *
 * @param[in] tracks List of active tracks.
 * @param[in] ts Current timestamp used to determine if track is updated.
 */
void track_demultiplexer
::add_tracks_for_frame( vidtk::track::vector_t const&  tracks,
                        vidtk::timestamp const&        ts )
{
  // reset outputs
  reset_outputs();

  this->active_tracks_ = tracks; // all tracks are active

  track_map_t local_map( this->active_track_map_ );

  // make list of new tracks
  BOOST_FOREACH( vidtk::track_sptr track, tracks )
  {
    track_map_key_t key( track );

    // look for new tracks
    if ( this->active_track_map_.count( key ) == 0 )
    {
      new_tracks_.push_back( track );

      map_value_t val( key, track );
      this->active_track_map_.insert( val );
      continue;
    }
    else
    {
      // remove existing tracks from local map
      // This will result in list of tracks that are not active
      // (termianted tracks)
      local_map.erase( key );
    }

    // always update local track map with newest track.
    this->active_track_map_[key] = track;

    // check for not updated
    if( ts.is_valid() )
    {
      if ( track->last_state()->time_ != ts )
      {
        // missing state
        this->not_updated_tracks_.push_back( track );
      }
      // then it must be updated
      else
      {
        this->updated_tracks_.push_back( track );
      }
    }
  } // end foreach

  // Scan map that contains terminated tracks.
  // Add track to terminated output list.
  // Remove track from active map.
  BOOST_FOREACH( map_value_t const & elem, local_map )
  {
    this->terminated_tracks_.push_back( elem.second );
    this->active_track_map_.erase( elem.first );
  }
} // add_tracks_for_frame


// ----------------------------------------------------------------
/** Add active tracks.
 *
 * The list of active tracks is processed against the internal list of
 * previously active tracks. These tracks are processed as follows.
 *
 * If track is not in previous active track list, it is added to the
 * new-track list and added to the current active track list.
 *
 * Terminated tracks are those tracks that are in the local (last) active
 * track list that are not in the current active track list.
 *
 * @param[in] tracks List of active tracks.
 */
void track_demultiplexer
::add_tracks( vidtk::track::vector_t const& tracks )
{
  this->add_tracks_for_frame( tracks, vidtk::timestamp() );
}


// ----------------------------------------------------------------
/** Flush stored tracks to the terminated port.
 *
 *
 */
bool track_demultiplexer::
flush_tracks()
{
  reset_outputs();
  if( this->active_track_map_.empty() )
  {
    // This is not considered a *failure* but more of a *warning*
    // for the caller to notify that there wasn't anything to flush.
    return false;
  }

  BOOST_FOREACH (map_value_t const& elem, this->active_track_map_)
  {
    this->terminated_tracks_.push_back(elem.second);
  }

  this->active_track_map_.clear();

  return true;
}


// ----------------------------------------------------------------
/** Reset outputs
 *
 *
 */
void track_demultiplexer::
reset_outputs()
{
  new_tracks_.clear();
  terminated_tracks_.clear();
  updated_tracks_.clear();
  not_updated_tracks_.clear();
  active_tracks_.clear();
}


// ----------------------------------------------------------------
/**
 *
 *
 */
vidtk::track::vector_t
track_demultiplexer::
get_new_tracks() const
{
  if ( IS_DEBUG_ENABLED() )
  {
    std::stringstream stuff;
    stuff << "-- New Tracks --\n";
    BOOST_FOREACH( vidtk::track_sptr trk, this->new_tracks_ )
    {
      stuff << *trk << std::endl;
    }

    LOG_DEBUG( stuff.str() );
  }

  return this->new_tracks_;
}


vidtk::track::vector_t
track_demultiplexer::
get_updated_tracks() const
{
  if ( IS_DEBUG_ENABLED() )
  {
    std::stringstream stuff;
    stuff << "-- Updated Tracks -- size (" << this->updated_tracks_.size() << ")\n";
    BOOST_FOREACH( vidtk::track_sptr trk, this->updated_tracks_ )
    {
      stuff << *trk << std::endl;
    }

    LOG_DEBUG( stuff.str() );
  }

  return this->updated_tracks_;
}


vidtk::track::vector_t
track_demultiplexer::
get_not_updated_tracks() const
{
  if ( IS_DEBUG_ENABLED() )
  {
    std::stringstream stuff;
    stuff << "-- Not Updated Tracks -- size (" << this->not_updated_tracks_.size() << ")\n";
    BOOST_FOREACH( vidtk::track_sptr trk, this->not_updated_tracks_ )
    {
      stuff << *trk << std::endl;
    }

    LOG_DEBUG( stuff.str() );
  }

  return this->not_updated_tracks_;
}


vidtk::track::vector_t
track_demultiplexer::
get_terminated_tracks() const
{
  if ( IS_DEBUG_ENABLED() )
  {
    std::stringstream stuff;
    stuff << "-- Terminated Tracks -- size (" << this->terminated_tracks_.size() << ")\n";
    BOOST_FOREACH( vidtk::track_sptr trk, this->terminated_tracks_ )
    {
      stuff << *trk << std::endl;
    }

    LOG_DEBUG( stuff.str() );
  }

  return this->terminated_tracks_;
}


vidtk::track::vector_t
track_demultiplexer::
get_active_tracks() const
{
  if ( IS_DEBUG_ENABLED() )
  {
    std::stringstream stuff;
    stuff << "-- Active Tracks -- size (" << this->active_tracks_.size() << ")\n";
    BOOST_FOREACH( vidtk::track_sptr trk, this->active_tracks_ )
    {
      stuff << *trk << std::endl;
    }

    LOG_DEBUG( stuff.str() );
  }

  return this->active_tracks_;
}

} // end namespace
