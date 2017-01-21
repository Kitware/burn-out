/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_object_reader_process.h"

#include "image_object_reader.h"

#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.hxx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.hxx>
#include <utilities/vsl/timestamp_io.h>
#include <tracking_data/vsl/image_object_io.h>

#include <logger/logger.h>
VIDTK_LOGGER("image_object_reader_process_cxx");


namespace vidtk
{

image_object_reader_process
::image_object_reader_process( std::string const& _name )
  : process( _name, "image_object_reader_process" ),
    disabled_( false ),
    eof_seen_( false ),
    the_reader_ ( 0 )
{
  config_.add_parameter( "disabled", "false", "Whether or not the process is disabled" );
  config_.add_parameter( "filename", "Input file name with extension." );
}


image_object_reader_process
::~image_object_reader_process()
{
  delete the_reader_;
}


config_block
image_object_reader_process
::params() const
{
  return config_;
}


// ----------------------------------------------------------------
bool
image_object_reader_process
::set_params( config_block const& blk )
{

  try
  {
    this->disabled_ = blk.get < bool >( "disabled" );
    this->filename_ = blk.get < std::string >( "filename" );
  }
  catch( config_block_parse_error& e)
  {
    LOG_ERROR (this->name() << ": Unable to set parameters: " << e.what());
    return false;
  }

  config_.update( blk );
  return true;
}


// ----------------------------------------------------------------
bool
image_object_reader_process
::initialize()
{
  if( disabled_ )
  {
    return true;
  }

  delete the_reader_;
  the_reader_ = new image_object_reader(filename_);
  if (! the_reader_->open())
  {
    LOG_ERROR( name() << ": unknown format for file: " << this->filename_);
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
bool
image_object_reader_process
::step()
{
  // reset outputs
  this->output_objs_.clear();

  if( this->disabled_ )
  {
    this->output_ts_ = this->input_ts_;
    return true;
  }

  // If we are at the end of file.
  if ( this->eof_seen_ )
  {
    this->output_ts_ = this->input_ts_;
    return true;
  }

  ns_image_object_reader::ts_object_vector_t buffer;

  //
  // read inputs for next timestamp
  //
  if ( ! this->file_timestamp_.is_valid() )
  {
    if ( ! the_reader_->read_next_timestamp( buffer ) )
    {
      this->eof_seen_ = true;
      this->output_ts_ = this->input_ts_;

      LOG_INFO( this->name() <<  ": End of file on detections input" );
      if ( this->input_ts_.is_valid() )
      {
        return true; // file is now empty
      }
      else
      {
        // This is the case where the input timestamp is not valid.
        // The process is used just as a source, do indicate EOF
        return false;
      }
    }

    // Save detection read from the file
    this->file_timestamp_ = buffer.first;
    this->file_objs_      = buffer.second;
  }

  // There are use cases where this reader process is only used to
  // source detections, rather than to meter them out matching a data
  // stream. In this case, where the input TS is not valid, we will
  // skip the checks that match our output to the input TS.
  if ( this->input_ts_.is_valid() )
  {
    // Check to see if our input TS is the same as the one we have from
    // the file. We check for input TS < detection TS, and if so, just
    // pass back the input TS.
    if ( this->input_ts_  <  this->file_timestamp_ )
    {
      // There are no detections for this TS
      this->output_ts_ = this->input_ts_;
      LOG_TRACE(this->name() << ": Sourcing image objects for " << this->output_ts_
                 << " - no detections returned from reader." );
      return true;
    }

    // At this point, the input-TS >= detection-TS. If it is equal, then
    // conditions are as expected.  if greater, then the current data
    // flow is not the same as that used to create the external
    // detections. A Warning is in order.
    if ( this->input_ts_ != this->file_timestamp_ )
    {
      LOG_WARN( name() <<  ": Timestamp mismatch between data flow and input file. Input: "
                << this->input_ts_ << "  From file: " << this->file_timestamp_  );
    }
  }

  // Return list of objects and timestamp
  this->output_ts_      = this->file_timestamp_;
  this->output_objs_    = this->file_objs_;
  this->file_timestamp_ = vidtk::timestamp();

  // reset inputs
  this->input_ts_ = vidtk::timestamp();
  LOG_TRACE( this->name() << ":Sourcing image objects for " << this->output_ts_ << " - "
             << this->output_objs_.size() << " detections" );

  return true;
}


// ----------------------------------------------------------------
// -- inputs --
void
image_object_reader_process
::set_timestamp( vidtk::timestamp const& ts )
{
  input_ts_ = ts;
}


// ----------------------------------------------------------------
// -- outputs --
vidtk::timestamp
image_object_reader_process
::timestamp() const
{
  return output_ts_;
}


std::vector< image_object_sptr >
image_object_reader_process
::objects() const
{
  return output_objs_;
}


bool
image_object_reader_process
::is_disabled() const
{
  return disabled_;
}

} // end namespace vidtk
