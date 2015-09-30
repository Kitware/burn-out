/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/unique_track_id_process.h>
#include <tracking/tracking_keys.h>

#include <utilities/unchecked_return_value.h>
#include <logger/logger.h>

#include <vul/vul_sprintf.h>

namespace vidtk
{

VIDTK_LOGGER ("unique_track_id_process");

// ----------------------------------------------------------------
/** Unique track id process
 *
 * Inputs:
 * - source timestamp - current timestamp
 * - source active tracks
 * - source terminated tracks
 *
 * Outputs:
 * - source timestamp -
 * - active tracks - number unique across tracker_subtype
 * - terminated tracks - number unique across tracker_subtype
 *
 * If multiple trackers are used in cascade upstream, the
 * input tracks will, in general, have overlapping ids.
 * This process re-assigns track numbers so that all output
 * tracks have a unique id number. The *does* require that
 * each upstream tracker assign a unique tracker_subtype
 * value in their corresponding track property set.
 *
 * @todo Add state chart/diagram.
 */


unique_track_id_process
::unique_track_id_process( vcl_string const& name )
  : process( name, "unique_track_id" ),
    disabled_( false ),
    next_unique_track_id_(1),
    map_fstr_()
{
  config_.add_parameter( "disabled", "false", "" );
  config_.add_parameter( "map_filename", "map.txt", "The output map file,"
                        " listing the track ids before and after reassignment." );
}

unique_track_id_process
::~unique_track_id_process()
{
  if ( map_fstr_.is_open() )
  {
    map_fstr_.close();
  }
}

config_block
unique_track_id_process
::params() const
{
  return config_;
}

bool
unique_track_id_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "disabled", disabled_ );
    blk.get( "map_filename", map_filename_ );
  }
  catch( unchecked_return_value& )
  {
    // Reset to old values
    set_params( config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
unique_track_id_process
::initialize()
{
  track_id_map_.clear();
  next_unique_track_id_ = 1;

  // Open stream for writing map file
  if ( ! disabled_ )
  {
    map_fstr_.open( map_filename_.c_str() );
    if( ! map_fstr_ )
    {
      LOG_ERROR( name() << ": Couldn't open "
               << map_filename_ << " for writing.\n" );
      return false;
    }
    map_fstr_ << "#Output_Track_id  Input_Tracker_Subtype Input_Track_id\n";
  }

  return true;
}

bool
unique_track_id_process
::step()
{
  if ( disabled_ )
  {
    return true;
  }

  // Process each active track, to have unique
  // id across tracker_subtypes
  track_sptr track;
  unsigned input_id;
  unsigned tracker_subtype;
  track_key_map_t::iterator find_iter;

  // Clear outputs
  active_track_keys_.clear();

  for (size_t i = 0; i < active_tracks_.size(); ++i)
  {
    track = active_tracks_[i];
    input_id = track->id();
    tracker_subtype = 0; // unknown type

    if ( track->data().has( vidtk::tracking_keys::tracker_subtype) )
    {
      track->data().get(vidtk::tracking_keys::tracker_subtype, tracker_subtype);
    }
    else
    {
      LOG_WARN(name() << ": Missing tracker_subtype property, using 0 (unknown)");
    }

    track_key_t key(tracker_subtype, input_id);

    // Add active track key to the output list
    active_track_keys_.push_back( key );

    find_iter = track_id_map_.find(key);
    if (find_iter == track_id_map_.end())
    {
      // Not in map, so assign new track id
      track->set_id( next_unique_track_id_ );
      LOG_INFO( name() << ": Assigning new track id " << track->id()
                << ", input tracker_subtype " << tracker_subtype
                << " input_id " << input_id );

      track_id_map_[key] = track->id();

      map_fstr_ << vul_sprintf("%16d %22d %14d\n",
                               track->id(),
                               tracker_subtype,
                               input_id);
      map_fstr_.flush();

      next_unique_track_id_++;
    }
    else
    {
      track->set_id( find_iter->second );
      LOG_TRACE("Found active track type/id " << key.first << ", " << key.second);
    }
  }  // end for (i)

  // Update each terminated track using the id
  // map generated from active tracks
  for (size_t i = 0; i < terminated_tracks_.size(); ++i)
  {
    track = terminated_tracks_[i];
    input_id = track->id();
    tracker_subtype = 0;

    if ( track->data().has( vidtk::tracking_keys::tracker_subtype) )
    {
      track->data().get(vidtk::tracking_keys::tracker_subtype, tracker_subtype);
    }
    else
    {
      LOG_WARN(name() << ": Missing tracker_subtype property, using 0 (unknown)");
    }

    track_key_t key(tracker_subtype, input_id);
    find_iter = track_id_map_.find(key);
    if (find_iter == track_id_map_.end())
    {
      // If not in map --> this is an error
      LOG_ERROR( name() << ": Missing entry in track id map"
                 << " for tracker_subtype " << tracker_subtype
                 << " input_id " << input_id );
      return false;
    }
    else
    {
      track->set_id( find_iter->second );
      LOG_TRACE("Found terminated track type/id " << key.first << ", " << key.second);
    }
  }

  return true;
}


// -- process inputs --
void
unique_track_id_process
::input_timestamp( timestamp const& ts )
{
  timestamp_ = ts;
}


void
unique_track_id_process
::input_active_tracks( track_vector_t const& tracks )
{
  active_tracks_.clear();

  // shallow clone track vector
  track_vector_t::const_iterator ix;
  track_vector_t::const_iterator eix = tracks.end();

  for ( ix = tracks.begin(); ix != eix; ix++)
  {
    active_tracks_.push_back ((*ix)->shallow_clone() );
  }
}


void
unique_track_id_process
::input_terminated_tracks( track_vector_t const& tracks )
{
  terminated_tracks_.clear();

  // shallow clone track vector
  track_vector_t::const_iterator ix;
  track_vector_t::const_iterator eix = tracks.end();

  for ( ix = tracks.begin(); ix != eix; ix++)
  {
    terminated_tracks_.push_back ((*ix)->shallow_clone() );
  }
}


// -- process outputs --
timestamp const&
unique_track_id_process
::output_timestamp() const
{
  return timestamp_;
}


track_vector_t const&
unique_track_id_process
::output_active_tracks() const
{
  return active_tracks_;
}


track_vector_t const&
unique_track_id_process
::output_terminated_tracks() const
{
  return terminated_tracks_;
}


unique_track_id_process::track_key_map_t const&
unique_track_id_process
::output_track_map() const
{
  return track_id_map_;
}


vcl_vector< unique_track_id_process::track_key_t > const&
unique_track_id_process
::output_active_track_keys() const
{
  return active_track_keys_;
}

}  // end namespace vidtk
