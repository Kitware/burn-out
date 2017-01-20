/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "active_tracks_generator.h"

#include <boost/foreach.hpp>
#include <algorithm>

#include <logger/logger.h>

VIDTK_LOGGER ("active_tracks_generator");

#define LOCAL_DEBUG 0


#if LOCAL_DEBUG

#define DEBUG_PRINT(v,A) do { if ((v) <= LOCAL_DEBUG) std::cout << A; } while(0)
#define ERROR_PRINT(v,A) do { if ((v) <= LOCAL_DEBUG) std::cerr << A; } while(0)

#else

#define DEBUG_PRINT(v,A) do { } while (0)
#define ERROR_PRINT(v,A) do { } while (0)

#endif


namespace vidtk
{

// -- local types --
typedef std::map < unsigned, vidtk::track_view::vector_t >::iterator map_iterator_t;
typedef std::vector< track_state_sptr >::const_iterator track_state_iterator_t;

typedef std::map < unsigned, vidtk::timestamp > ts_map_t;
typedef ts_map_t::value_type ts_map_value_t;
typedef ts_map_t::iterator ts_map_it_t;

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
active_tracks_generator
::active_tracks_generator()
  : lowest_frame_(~0),
    highest_frame_(0)
{ }


active_tracks_generator
:: ~active_tracks_generator()
{ }


// ----------------------------------------------------------------
/** Add tracks to working set.
 *
 *
 */
void
active_tracks_generator
::add_tracks (vidtk::track_vector_t const& tracks)
{
  this->full_tracks_.insert (full_tracks_.end(), tracks.begin(), tracks.end());
}


// ----------------------------------------------------------------
/** Create active tracks.
 *
 * This method processes the vector of tracks and creates a map of
 * active tracks.
 *
 * Note: This implementation is faster, but does \b not include a
 * track in the active tracks vector for a frame if it is missing a
 * state at that frame.
 */

void
active_tracks_generator
::create_active_tracks_alt()
{
  // build a map of frames to timestamps
  create_timestamp_map();
  BOOST_FOREACH (vidtk::track_sptr track, this->full_tracks_)
  {
    if (track->history().empty()) // skip empty tracks
    {
      continue;
    }

    track_state_iterator_t ix = track->history().begin();
    track_state_iterator_t eix = track->history().end();

    // loop over all states
    for (/* empty */; ix != eix; ix++)
    {
      // Add track view to our set
      vidtk::track_view::sptr tvsp= vidtk::track_view::create(track);
      vidtk::track_state_sptr state = *ix;
      tvsp->set_view_bounds (track->first_state()->get_timestamp(), state->get_timestamp());

      DEBUG_PRINT( 3, "Added view to active set for track id: " << track->id()
                   << " - frame " << track->first_state()->get_timestamp().frame_number()
                   << " to " << state->get_timestamp().frame_number()
                   << std::endl);

      // Add active track view to list
      active_tracks_[state->get_timestamp().frame_number()].push_back( tvsp );
    } // end for states

  } // end foreach track
}


// ----------------------------------------------------------------
/** Create active tracks.
 *
 * This method processes the vector of tracks and creates a map of
 * active tracks.
 *
 * Note: This implementation \b does include a track in the active tracks
 * vector for a frame if it is missing a state at that frame.
 *
 * This implementation involves a lot of looping, even excessive, some
 * would say.
 */
void
active_tracks_generator
::create_active_tracks()
{
  // build a map of frames to timestamps
  create_timestamp_map();

  for (unsigned frame = this->lowest_frame_; frame <= this->highest_frame_; frame++)
  {
    vidtk::timestamp ts;
    timestamp_for_frame (frame, ts);

    // loop over all tracks
    BOOST_FOREACH (vidtk::track_sptr ptr, this->full_tracks_)
    {
      if (ptr->history().empty()) // skip empty tracks
      {
        continue;
      }

      // If this track contains current timestamp, then add to input list
      if ( (ptr->first_state()->get_timestamp().frame_number() <= frame )
           && (frame <= ptr->last_state()->get_timestamp().frame_number()) )
      {
        // Add track view to our set
        vidtk::track_view::sptr tvsp= vidtk::track_view::create(ptr);
        tvsp->set_view_bounds (ptr->first_state()->get_timestamp(), ts);

        DEBUG_PRINT (3, "Track " << ptr->id() << " from " << ptr->first_state()->get_timestamp().frame_number()
                     << " to " << ts.frame_number()
                     << " added to frame " << frame << "\n");

        // Add active track view to list
        active_tracks_[frame].push_back( tvsp );
      }
    } // end foreach
  } // end for
}


// ----------------------------------------------------------------
void
active_tracks_generator
::create_terminated_tracks()
{
  BOOST_FOREACH (vidtk::track_sptr trk, this->full_tracks_)
  {
    // If there are no states, then we can not be expected to use the
    // TS from the last one as an index, now can we?
    if ( ! trk->history().empty() )
    {
      terminated_tracks_[ trk->last_state()->get_timestamp().frame_number() + 1 ].push_back( trk );
    }
  }
}


// ----------------------------------------------------------------
// Find timestamp in a track for a frame number
vidtk::timestamp
active_tracks_generator
::find_timestamp (unsigned frame, const vidtk::track_sptr ptr)
{
  std::vector< vidtk::track_state_sptr > const& hist (ptr->history());
  BOOST_FOREACH (const vidtk::track_state_sptr state, hist)
  {
    if (state->get_timestamp().frame_number() == frame)
    {
      return state->get_timestamp();
    }
  }

  return vidtk::timestamp();
}


// ----------------------------------------------------------------
/** Create timestamp map.
 *
 * Create a timestamp map to relate frame numbers to timestamps.
 */
void
active_tracks_generator
::create_timestamp_map()
{
  // find lowest frame number and highest frame number.
  BOOST_FOREACH (const vidtk::track_sptr ptr, this->full_tracks_)
  {
    if (ptr->history().empty()) // skip empty tracks
    {
      continue;
    }

    this->lowest_frame_  = std::min (this->lowest_frame_, ptr->first_state()->get_timestamp().frame_number() );
    this->highest_frame_ = std::max (this->highest_frame_, ptr->last_state()->get_timestamp().frame_number() );
  }

  // build a map of frames to timestamps
  for (unsigned frame = this->lowest_frame_; frame <= this->highest_frame_; frame++)
  {
    // loop over all tracks
    BOOST_FOREACH (vidtk::track_sptr ptr, this->full_tracks_)
    {
      if (ptr->history().empty()) // skip empty tracks
      {
        continue;
      }

      // We need a real timestamp from the input set, not just a
      // frame number, so if we haven't found one before, look
      // through the current track for the current frame and build
      // a timestamp from that.
      vidtk::timestamp ts = find_timestamp( frame, ptr);
      if (ts.is_valid())
      {
        this->ts_map_.insert ( ts_map_value_t(frame, ts) );
        break;
      }
    } // end foreach
  } // end for


  double f_sum(0), f_mean(0);   // x
  double t_sum(0), t_mean(0);   // y
  double count(0);
  BOOST_FOREACH (ts_map_value_t & v, this->ts_map_)
  {
    f_sum += v.second.frame_number();
    t_sum += v.second.time();
    count += 1.0;
  }

  f_mean = f_sum / count;
  t_mean = t_sum / count;

  double f_dev(0);
  // double t_dev(0);
  BOOST_FOREACH (ts_map_value_t & v, this->ts_map_)
  {
    double t;
    t = v.second.frame_number() - f_mean;
    f_dev += t * t;

    // t = v.second.time() - t_mean;
    // t_dev += t * t;
  }

  double sft(0);
  BOOST_FOREACH (ts_map_value_t & v, this->ts_map_)
  {
    sft += (v.second.frame_number() - f_mean) * (v.second.time() - t_mean);
  }

  this->seconds_per_frame_ = sft / f_dev;
  this->base_timestamp_ = t_mean - (seconds_per_frame_ * f_mean);
}


// ----------------------------------------------------------------
/* Find active tracks for frame.
 */
bool
active_tracks_generator
::active_tracks_for_frame(unsigned frame, vidtk::track_vector_t & tracks)
{
  // assure no previous tracks
  tracks.clear();

  // look for active tracks for specified frame
  map_iterator_t ix = this->active_tracks_.find(frame);
  if ( ix == this->active_tracks_.end())
  {
    // No active tracks for this frame, but still o.k.
    return true;
  }

  // convert from vector of track_views to vector of tracks
  BOOST_FOREACH (vidtk::track_view::sptr tvsp, ix->second)
  {
    vidtk::track_sptr a_trk = *tvsp;
    tracks.push_back(a_trk);
  }

  return true;
}

// ----------------------------------------------------------------
/* Find terminated tracks for frame.
 */
bool
active_tracks_generator
::terminated_tracks_for_frame(unsigned frame, vidtk::track_vector_t & tracks)
{
  // assure no previous tracks
  tracks.clear();

  // look for terminated tracks for specified frame
  std::map < unsigned, std::vector< vidtk::track_sptr > >::iterator ix
    = this->terminated_tracks_.find(frame);

  if ( ix == this->terminated_tracks_.end())
  {
    // No terminated tracks for this frame, but still o.k.
    return true;
  }

  tracks = ix->second;
  return true;
}


// ----------------------------------------------------------------
/* Get timestamp for frame.
 */
bool
active_tracks_generator
::timestamp_for_frame( unsigned frame, vidtk::timestamp & ts, bool est)
{
  std::map < unsigned, vidtk::timestamp >::const_iterator ix, ixl, ixg;

  if (est)
  {
    // calculate estimated timestamp using linear approximation
    ts = vidtk::timestamp(this->seconds_per_frame_ * frame + this->base_timestamp_, frame);
    return false;
  }

  ix = this->ts_map_.find(frame);
  if (ix == this->ts_map_.end())
  {
    // fill in missing timestamp by using local interpolation
    // assume frame rate in uniform within gap
    ixg = ts_map_.upper_bound(frame);
    ixl = ixg;

    // If not at the beginning, we can back up one
    if (ixl != this->ts_map_.begin())
    {
      --ixl;
    }

    // Test for invalid iterator positions
    // - ixg at the end, no entry there to use
    // - ixg at begin, can not get ixl to be the previous entry
    if ( (ixg != this->ts_map_.end()) && (ixg != this->ts_map_.begin()) )
    {
      const double df ( ixg->first - ixl->first );
      const double dt ( ixg->second.time() - ixl->second.time() );
      const double fo ( frame - ixl->first );
      const double t ( ixl->second.time() + ( (dt / df) * fo) );
      ts = vidtk::timestamp(t, frame);
    }
    else
    {
      // calculate estimated timestamp if either bound can not be determined
      ts = vidtk::timestamp(this->seconds_per_frame_ * frame + this->base_timestamp_, frame);
    }
    return false;
  }

  ts = ix->second;
  return true;
}


// ----------------------------------------------------------------
/**
 *
 *
 */
unsigned
active_tracks_generator
::first_frame() const
{
  return this->lowest_frame_;
}


unsigned
active_tracks_generator
::last_frame() const
{
  return this->highest_frame_;
}


} // end namespace
