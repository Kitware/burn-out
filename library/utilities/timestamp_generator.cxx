/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "timestamp_generator.h"

#include <utilities/oc_timestamp_hack.h>
#include <logger/logger.h>
#include <limits>

#include <ctime>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace vidtk
{

VIDTK_LOGGER( "timestamp_generator" );


timestamp_generator
::timestamp_generator()
  : frame_number_(0)
{
  config_.add_parameter( "filename", "",
    "Path to the file containing the timestamps corresponding to appropriate"
    "'method'." );
  config_.add_parameter( "method", "disabled",
    "Choose one of the available methods to produce the timestamp: disabled "
    "[default] (NOTE: disabled can change and/or filter timestamps if other config parameters are set)"
    ", ocean_city, update_frame_number, or frame_timestamp_pair_file." );

  // Parameters to affect frame number
  config_.add_parameter( "start_frame", "0",
    "Frame number assigned to the first frame in the sequence." );
  std::ostringstream oss;
  oss << std::numeric_limits<unsigned>::max();
  config_.add_parameter( "end_frame", oss.str(),
    "Frame number assigned to the last frame in the sequence." );
  config_.add_parameter( "skip_frames" , "0",
    "Number of frames to drop after a frame used from the source, e.g. 2 "
    " converts a 30Hz source into 10Hz source video." );

  // Parameters to affect time
  config_.add_parameter( "base_time", "0.0",
    "An absolute timestamp (UTC or other) in seconds for the first frame of "
    " the input image sequence. This will over-write the pre-existing time." );
  config_.add_parameter( "manual_frame_rate", "-1",
    "Provide a frames/second rate at which timestamps should be created."
    " This will over-write the pre-existing time." );
  config_.add_parameter( "random_base_time", "false",
    "Should the base time be a random timestamp?." );
  config_.add_parameter( "random_min_time", "1",
    "Minimum allowed random base time, if enabled." );
  config_.add_parameter( "random_max_time", "500000000",
    "Maximum allowed random base time, if enabled." );
}


timestamp_generator
::~timestamp_generator()
{
  in_stream_.close();
}


config_block
timestamp_generator
::params() const
{
  return config_;
}


checked_bool
timestamp_generator
::set_params( config_block const& blk )
{
  filename_ = blk.get< std::string >( "filename" );
  std::string method;
  method = blk.get< std::string >( "method" );
  if( method == "disabled" )
  {
    method_ = disabled;
  }
  else if( method == "update_frame_number" )
  {
    method_ = update_frame_number;
  }
  else if( method == "ocean_city" )
  {
    method_ = ocean_city;
  }
  else if( method == "frame_timestamp_pair_file" )
  {
    method_ = fr_ts_pair_file;
  }
  else
  {
    throw config_block_parse_error( "unknown timestamp reading method.  Options are: "
                                    "disabled, ocean_city, update_frame_number, or "
                                    "frame_timestamp_pair_file." );
  }

  skip_frames_ = blk.get< unsigned >( "skip_frames" );
  start_frame_ = blk.get< unsigned >( "start_frame" );
  end_frame_ = blk.get< unsigned >( "end_frame" );

  manual_frame_rate_ = blk.get< double >( "manual_frame_rate" );

  if( !blk.get< bool >( "random_base_time" ) )
  {
    base_time_ = blk.get< double >( "base_time" );
    base_time_ *= 1e6; // secs to usecs (used in vidtk::timestamp)
  }
  else
  {
    const unsigned min_time = blk.get< unsigned >( "random_min_time" );
    const unsigned max_time = blk.get< unsigned >( "random_max_time" );

    boost::random::mt19937 gen( std::time( 0 ) );
    boost::random::uniform_int_distribution<> dist( min_time, max_time );
    base_time_ = static_cast< double >( dist( gen ) );
  }

  config_.update( blk );
  return true;
}


bool
timestamp_generator
::initialize()
{
  frame_number_ = start_frame_;

  switch( method_ )
  {
  case ocean_city:
    return oc_timestamp_parser_.load_file( filename_ );
  case fr_ts_pair_file:
    return init_pair_file();
  default:
    return true;
  }
}


process::step_status
timestamp_generator
::provide_timestamp( timestamp & ts )
{
  process::step_status p_status = process::SUCCESS;
  bool okay = false;

  // Update the frame number
  switch( method_ )
  {
  case disabled:
    okay = true; // Keep the frame number in the input ts
    break;

  case update_frame_number:
    ts.set_frame_number( frame_number_ );
    frame_number_++;
    okay = true;
    break;

  case ocean_city:
    okay = oc_timestamp_parser_.set_timestamp( ts );
    break;

  case fr_ts_pair_file:
    okay = parse_pair_file( ts );
    break;
  }

  if( !okay )
  {
    return process::FAILURE;
  }

  if( ts.frame_number() >= start_frame_ &&
      ts.frame_number() <= end_frame_ &&
      (ts.frame_number() % (skip_frames_+1) == 0) )
  {
    // Update time if desired
    if( manual_frame_rate_ > 0 )
    {
      if( !apply_manual_framerate( ts ) )
      {
        return process::FAILURE;
      }
    }

    // base_time should be the last modification to the usecs time
    if( base_time_ > 0.0 )
    {
      ts.set_time( ts.time() + base_time_ );
    }
    p_status = process::SUCCESS;

  }
  else
  {
    p_status = process::SKIP;

    if( ts.frame_number() > end_frame_ )
    {
      LOG_INFO( "timestamp_generator: Stopping the pipeline at end_frame \""
                << end_frame_ <<"\"" );
      return process::FAILURE;
    }
    else if( ts.frame_number() < start_frame_ )
    {
      LOG_INFO( "timestamp_generator: Skipping the pipeline until start_frame \""
                << start_frame_ <<"\"" );
    }
    else
    {
      LOG_INFO( "timestamp_generator: Skipping frame " << ts.frame_number() );
    }
  }

  return p_status;
}


// Use the manual_frame_rate_ to provide timestamp in secs.
// NOTE: produces timestamp in MICRO seconds, this is what
// timestamp class expects.

bool
timestamp_generator
::apply_manual_framerate( timestamp & ts )
{
  if( ts.has_frame_number() )
  {
    double t = double (ts.frame_number()) / manual_frame_rate_;
    //convert from seconds to micro seconds, because
    //timestamp.time_in_secs() assumes that the internal time_ field
    //will be in microseconds.
    ts.set_time( t * 1e6 );
  }
  else
  {
    LOG_ERROR( "timestamp_generator: cannot use manual_frame_rate when the frame "
               "numbers are not provided by source." );
    return false;
  }

  return true;
}


bool
timestamp_generator
::init_pair_file()
{
  if( filename_.empty() )
  {
    LOG_ERROR( "timestamp_generator: frame_timestamp_pair_file mode requires "
               "a valid 'filename' value." );
    return false;
  }
  in_stream_.clear();
  in_stream_.open( filename_.c_str() );
  if (!in_stream_)
  {
    LOG_ERROR( "timestamp_generator: Couldn't open frame-timestamp pair file "
               << filename_ );
    return false;
  }

  return true;
}


bool
timestamp_generator
::parse_pair_file( timestamp & ts )
{
  if(!in_stream_.eof())
  {
    unsigned frame_number;
    double time;

    if( !(in_stream_ >> frame_number) )
    {
      return false;
    }
    ts.set_frame_number( frame_number );

    if( !(in_stream_ >> time) )
    {
      return false;
    }

    ts.set_time( time * 1e6 );
    return true;
  }
  else
  {
    in_stream_.close();
    return false;
  }
}

} // end namespace vidtk
