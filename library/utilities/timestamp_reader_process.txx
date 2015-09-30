/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "timestamp_reader_process.h"
#include <vcl_limits.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{


template <class PixType>
timestamp_reader_process<PixType>
::timestamp_reader_process( vcl_string const& name )
  : process( name, "timestamp_reader_process" ),
    input_md_( NULL ),
    frame_counter_( 0 ),
    last_ts_()
{
  config_.add( "override_existing_time", "false" );
  config_.add( "filename", "" );
  config_.add( "method", "null" );
  config_.add( "manual_frame_rate", "-1" );
  config_.add_parameter( "base_time",
    "0.0",
    "An absolute timestamp (UTC or other) in seconds for the first frame of "
    " the input image sequence." );
  config_.add_parameter( "skip_frames" , "0",
    "Number of frames to drop after a frame used from the source, e.g. 2 "
    " convert a 30Hz source into 10Hz source video." );
  config_.add_parameter( "start_frame", "0",
    "Index that is considered as the *start of input*." );
  vcl_ostringstream oss;
  oss << vcl_numeric_limits<unsigned>::max();
  config_.add_parameter( "end_frame", oss.str(),
    "Index that is considered as the *end of input*." );
}


template <class PixType>
timestamp_reader_process<PixType>
::~timestamp_reader_process()
{
}


template <class PixType>
config_block
timestamp_reader_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
timestamp_reader_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "override_existing_time", override_time_ );
    blk.get( "filename", filename_ );
    vcl_string method;
    blk.get( "method", method );
    if( method == "null" )
    {
      method_ = null;
    }
    else if( method == "ocean_city" )
    {
      method_ = ocean_city;
    }
    else if( method == "start_at_frame0" )
    {
      method_ = start_at_frame0;
    }
    else
    {
      log_error( name() << ": unknown method \"" << method << "\"\n" );
      throw unchecked_return_value( "unknown timestamp reading method" );
    }

    blk.get( "manual_frame_rate", manual_frame_rate_ );
    blk.get( "base_time", base_time_ );
    base_time_ *= 1e6; // secs to usecs (used in vidtk::timestamp)

    blk.get( "skip_frames", skip_frames_ );
    blk.get( "start_frame", start_frame_ );
    blk.get( "end_frame", end_frame_ );
  }
  catch( unchecked_return_value& )
  {
    // reset to the old values.
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
timestamp_reader_process<PixType>
::initialize()
{
  frame_counter_ = 0;

  switch( method_ )
  {
  case null:        return true;
  case ocean_city:  return initialize_ocean_city();
  case start_at_frame0:
    frame_num_0based_ = 0;
    return true;
  }

  // should never reach here.
  return false;
}


template <class PixType>
bool
timestamp_reader_process<PixType>
::step()
{
  // If the source already has a timestamp, we may have nothing to do.
  if( input_ts_.has_time() && ! override_time_ )
  {
    ts_ = input_ts_;
    input_ts_ = vidtk::timestamp();
    return true;
  }

  ts_ = input_ts_;

  bool okay = false;
  switch( method_ )
  {
  case null:       okay = true;  break;
  case ocean_city: okay = parse_ocean_city();     break;
  case start_at_frame0:
    ts_.set_frame_number( frame_num_0based_++ );
    okay = true;
    break;
  };

  if( manual_frame_rate_ > 0 )
  {
    okay = get_full_timestamp();
  }

  // base_time should be the last modification to the usecs time
  if( base_time_ > 0.0 )
  {
    ts_.set_time( ts_.time() + base_time_ );
    okay = true;
  }

  // invalidate input for next time around.
  input_ts_ = vidtk::timestamp();

  return okay;
}


template <class PixType>
process::step_status
timestamp_reader_process<PixType>
::step2()
{
  step_status p_status = SUCCESS;


  // If the TS is not valid, that means that we have not received any
  // input since the last step.
  if (! input_ts_.is_valid() )
  {
    return (FAILURE);
  }

  working_ts_vect_.push_back( input_ts_ );
  log_info( this->name() << ": adding ts to vector  " << input_ts_
            << "  (size: " << working_ts_vect_.size() << ")\n");

  if( input_md_ )
  {
    working_md_vect_.push_back( *input_md_ );
  }

  // Should this loop FAIL if we go past the end frame?
  if( frame_counter_ >= start_frame_ &&
      frame_counter_ <= end_frame_ &&
      (frame_counter_ % (skip_frames_+1) == 0) )
  {
    if( ! this->step() )
    {
      return FAILURE;
    }

    // Sanity check: Time should only increase.
    if( ts_.has_time() && last_ts_.time() >= ts_.time() )
    {
      log_error( this->name() << "Time should only increase from "
                              << last_ts_ << " to " << ts_ << vcl_endl );
      return FAILURE;
    }
    last_ts_ = ts_;

    output_ts_vect_ = working_ts_vect_;
    working_ts_vect_.clear();

    output_md_vect_ = working_md_vect_;
    working_md_vect_.clear();
  }
  else
  {
    p_status = SKIP;

    if( frame_counter_ > end_frame_ )
    {
      log_info( this->name() << ": Stopping the pipeline at end_frame \""
                             << end_frame_ <<"\"\n" );
      return FAILURE;
    }
    else if( frame_counter_ < start_frame_ )
    {
      log_info( this->name() << ": Skipping the pipeline untill start_frame \""
                             << start_frame_ <<"\"\n" );
    }
    else
    {
      log_info( this->name() << ": Skipping frame " << frame_counter_+1 << "\n" );
    }
  }

  frame_counter_++;

  // invalidate input data.
  input_md_ = NULL;

  return p_status;
}


template <class PixType>
bool
timestamp_reader_process<PixType>
::initialize_ocean_city()
{
  return oc_timestamp_parser_.load_file( filename_ );
}


template <class PixType>
bool
timestamp_reader_process<PixType>
::parse_ocean_city()
{
  return oc_timestamp_parser_.set_timestamp( ts_ );
}


// Use the manual_frame_rate_ to provide timestamp in secs.
// NOTE: produces timestamp in MICRO seconds, this is what
// timestamp class expects.

template <class PixType>
bool
timestamp_reader_process<PixType>
::get_full_timestamp()
{
  if( ts_.has_frame_number() )
  {
    double t = double (ts_.frame_number()) / manual_frame_rate_;
    //convert from seconds to micro seconds, because
    //timestamp.time_in_secs() assumes that the internal time_ field
    //will be in microseconds.
    ts_.set_time( t * 1e6 );
  }
  else
  {
    log_error( name() << ": cannot use manual_frame_rate when the frame "
                         "numbers are not provided by source.\n" );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
// -- Inputs --

template < class PixType >
void
timestamp_reader_process< PixType >
::set_source_timestamp( vidtk::timestamp const& ts )
{
  input_ts_ = ts;
}


template < class PixType >
void
timestamp_reader_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  img_ = img;
}


template< class PixType >
void timestamp_reader_process< PixType >
::set_source_metadata(video_metadata const& md)
{
  input_md_ = &md;
}


// ----------------------------------------------------------------
// -- Outputs --

template < class PixType >
vidtk::timestamp const&
timestamp_reader_process< PixType >
::timestamp() const
{
  return ts_;
}


template < class PixType >
vil_image_view< PixType > const&
timestamp_reader_process< PixType >
::image() const
{
  return img_;
}


template< class PixType >
vcl_vector< video_metadata > const& timestamp_reader_process< PixType >
::metadata() const
{
  return ( output_md_vect_ );
}


template< class PixType >
vidtk::timestamp::vector_t const&
timestamp_reader_process< PixType >::
timestamp_vector() const
{

  return ( output_ts_vect_ );
}


} // end namespace vidtk
