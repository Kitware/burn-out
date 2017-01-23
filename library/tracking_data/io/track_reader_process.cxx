/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_reader_process.h"
#include "active_tracks_reader.h"

#include <utilities/kw18_timestamp_parser.h>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("track_reader_process");


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
track_reader_process
::track_reader_process( std::string const& _name )
  : process( _name, "track_reader_process" ),
    disabled_( false ),
    ignore_occlusions_( true ),
    ignore_partial_occlusions_( true ),
    frequency_( 0.5 ),
    reader_( 0 ),
    batch_mode_( false ),
    batch_not_read_(true),
    active_tracks_disable_( true ),
    timestamps_from_tracks_( false ),
    current_frame_( 0 ),
    active_tracks_reader_( 0 ),
    terminated_flushed_( false ),
    read_lat_lon_for_world_( false )
{
  config_.add_parameter( "disabled", "false",
                         "If disabled, process produces no output. If enabled, process produces "
                         "vectors of tracks.");
  config_.add_parameter( "format", "",
                         "not used - for compatibility only" );
  config_.add_parameter( "filename", "",
                         "Name of input file to read tracks from.\n"
                         "When opening a database, it can either be the path to the database file (sqlite) "
                         "or the name of the database (postegresql).");
  config_.add_parameter( "ignore_occlusions", "true",
                         "Ignore occlusions. This option is specific to the VIPER file format." );
  config_.add_parameter( "ignore_partial_occlusions", "true",
                         "Ignore partial occlusions. This option is specific to the VIPER file format." );
  config_.add_parameter( "frequency", ".5",
                         "Seconds per frame. MIT Reader Only." );
  config_.add_parameter( "path_to_images", "",
                         "Path to files that contain images. One image per frame. "
                         "Pixels from images used to make chips in image objects. "
                         "Option for KW18 format only." );
  config_.add_parameter( "active_tracks_disable",
                         "true",
                         "Setting this to false will enable active tracks output." );
  config_.add_parameter( "timestamps_from_tracks",
                         "false",
                         "Setting this to true will enable using the track file as the source of timestamps."
                         "Warning: most track file formats do not store timestamps for frames with no tracks."
                         "Times on missing frames are interpolated and missing frames at the start and end are"
                         "skipped.  Only use this mode when processing tracks without loading video.");
  config_.add_parameter( "read_lat_lon_for_world",
                         "true",
                         "Read lat/lon values from the world coordinate fields. Only applicable to"
                         "the KW18 format." );

  //Database parameters
  config_.add_parameter("db_type",
                        "",
                        "The database connection driver being used. Supported types are:\n"
                        " - (empty string): No database connection (default)\n"
                        " - postgresql: Postgresql database\n"
                        " - sqlite3: SQLite database\n");
  config_.add_parameter("db_name",
                        "perseas",
                        "The name of the database.\n");
  config_.add_parameter("db_user",
                        "perseas",
                        "The username for database connectivity. Unused if the db_type is SQLite\n");
  config_.add_parameter("db_pass",
                        "",
                        "The password for database connectivity. Unused if the db_type is SQLite\n");
  config_.add_parameter("db_host",
                        "localhost",
                        "The host database server. Unused if the db_type is SQLite\n");
  config_.add_parameter("db_port",
                        "12345",
                        "The connection port for the database server. Unused if the db_type is SQLite\n");
  config_.add_parameter("connection_args",
                        "",
                        "Arguments that are passed to the database connection.\n"
                        "The argument list takes the form k1=v1;k2=v2;(...).\n");
  config_.add_parameter("append_tracks",
                        "true",
                        "Instructs the reader whether to append current frame's track_states\n"
                        "onto the existing track ( true ), or to pass just the states for this frame.");
  config_.add_parameter("db_session_id",
                        "-1",
                        "The id of the session used to write the tracks. ");
}


track_reader_process
::~track_reader_process()
{
  delete reader_;
  delete active_tracks_reader_;
}


config_block
track_reader_process
::params() const
{
  return config_;
}


// ----------------------------------------------------------------
/** Configure this process from supplied parameters.
 *
 *
 */
bool
track_reader_process
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get< bool > ( "disabled" );
    if ( ! disabled_ )
    {
      filename_ = blk.get< std::string > ( "filename" );
      ignore_occlusions_ = blk.get< bool > ( "ignore_occlusions" );
      ignore_partial_occlusions_ = blk.get< bool > ( "ignore_partial_occlusions" );
      frequency_ = blk.get< double > ( "frequency" );
      path_to_images_ = blk.get< std::string > ( "path_to_images" );
      active_tracks_disable_ = blk.get< bool > ( "active_tracks_disable" );
      timestamps_from_tracks_ = blk.get< bool > ( "timestamps_from_tracks" );
      read_lat_lon_for_world_ = blk.get< bool > ( "read_lat_lon_for_world" );

      this->reader_opt_.set_ignore_occlusions( ignore_occlusions_ )
        .set_ignore_partial_occlusions( ignore_partial_occlusions_ )
        .set_sec_per_frame( frequency_ )
        .set_path_to_images( path_to_images_ )
        .set_read_lat_lon_for_world( read_lat_lon_for_world_ );

      this->reader_opt_.set_db_type(blk.get<std::string>("db_type"));
      this->reader_opt_.set_db_user(blk.get<std::string>("db_user"));
      this->reader_opt_.set_db_pass(blk.get<std::string>("db_pass"));
      this->reader_opt_.set_db_host(blk.get<std::string>("db_host"));
      this->reader_opt_.set_db_port(blk.get<std::string>("db_port"));
      this->reader_opt_.set_connection_args(blk.get<std::string>("connection_args"));
      this->reader_opt_.set_append_tracks(blk.get<bool>("append_tracks"));
      this->reader_opt_.set_db_session_id(blk.get<int>("db_session_id"));
    }
  }
  catch ( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
} // set_params


// ----------------------------------------------------------------
/** Initialize process.
 *
 *
 */
bool
track_reader_process
::initialize()
{
  if (disabled_)
  {
    return true;
  }

  // delete any previous readers.  In some cases the process is reused
  // so we can't count on this being a newly created object.
  delete reader_;
  reader_ = 0;

  batch_not_read_ = true;

  reader_ = new vidtk::track_reader( filename_ );
  reader_->update_options( reader_opt_ );

  delete active_tracks_reader_;
  active_tracks_reader_ = 0;

  if ( ! this->reader_->open() )
  {
    LOG_ERROR (this->name() << ": Could not open track file \"" << filename_ << "\"");
    return false;
  }

  // Instantiate active track reader if enabled
  if ( !active_tracks_disable_ )
  {
    active_tracks_reader_ = new active_tracks_reader( reader_ );

    if ( timestamps_from_tracks_ )
    {
      current_frame_ = active_tracks_reader_->first_frame();
    }
  }
  return true;
}


// ----------------------------------------------------------------
/** Main process loop.
 *
 *
 */
bool
track_reader_process
::step()
{
  if(disabled_)
  {
    return true;
  }

  // batch mode and active tracks do not make sense together.
  if (batch_mode_ && ! active_tracks_disable_)
  {
    LOG_WARN(this->name() << ": Active tracks output enabled in batch mode. Active tracks output disabled.");
    active_tracks_disable_ = true;
  }

  if( !active_tracks_disable_ && !ts_.has_frame_number() && terminated_flushed_ )
  {
    LOG_WARN( this->name() << ": A valid input frame_number is required when "
      "producing active tracks." );
    return false;
  }

  if( batch_mode_ )
  {
    bool r = batch_not_read_;
    batch_not_read_ = false;
    tracks_.clear();
    reader_->read_all( tracks_ );
    return r;
  }

  // return terminated tracks
  bool rc_read(false);

  if ( active_tracks_disable_ )
  {
    unsigned frame;

    // Not doing active tracks, just read in terminated order
    rc_read = reader_->read_next_terminated(tracks_, frame);

    // Note that we DON'T set tracker_ts_ from this frame number--
    // see track_reader_interface.h for details.  If active_tracks_disable_
    // is true, we don't set tracker_ts_ at all.

  }
  else
  {
    if( timestamps_from_tracks_ )
    {
      if( current_frame_ > active_tracks_reader_->last_frame() )
      {
        ts_ = timestamp();
      }
      else
      {
        ts_.set_frame_number(current_frame_++);
      }
    }
    // Active tracks is enabled - use active tracks interface
    if( ts_.has_frame_number() )
    {
      this->last_ts_ = ts_;

      // Return false when both terminated tracks and active tracks readers fail
      rc_read = active_tracks_reader_->read_terminated( ts_.frame_number(), tracks_ );
      rc_read = rc_read && active_tracks_reader_->read_active( ts_.frame_number(), active_tracks_);

      LOG_DEBUG( this->name() << ": Publishing ("<< active_tracks_.size() <<" active,"
        << tracks_.size()<< " terminated) tracks at " << ts_ );

      active_tracks_reader_->get_timestamp(ts_.frame_number(),tracker_ts_);
    }
    else
    {
      // This is to handle the last frame of the video where the
      // terminated tracks are published *after* the last state in the
      // track. Consumers of the active mode need to be aware of this
      // extra step when the input timestamp is invlaid.
      active_tracks_.clear();
      rc_read = active_tracks_reader_->read_terminated( last_ts_.frame_number() + 1, tracks_ );
      active_tracks_reader_->get_timestamp(last_ts_.frame_number() + 1,tracker_ts_);
      terminated_flushed_ = true;
      LOG_DEBUG( this->name() << ": Publishing (" << active_tracks_.size() << " active,"
                              << tracks_.size() << " terminated) final set at " << tracker_ts_ );

    }
  }

  ts_ = timestamp(); // setting to invalid default

  return rc_read;
}


// -- process inputs(s) --
void
track_reader_process
::set_timestamp(timestamp const & ts)
{
  ts_ = ts;
}


// -- process output(s) --
std::vector<track_sptr>
track_reader_process
::tracks() const
{
  return tracks_;
}


std::vector<track_sptr>
track_reader_process
::active_tracks() const
{
  return active_tracks_;
}


timestamp track_reader_process
::get_current_timestamp() const
{
  return tracker_ts_;
}


// -- misc utility methods --
bool
track_reader_process
::is_disabled() const
{
  return disabled_;
}


bool
track_reader_process
::is_batch_mode() const
{
  return batch_mode_;
}


void
track_reader_process
::set_batch_mode( bool batch_mode )
{
  batch_mode_ = batch_mode;
}

}
