/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_cstring.h>
#include <utilities/unchecked_return_value.h>

#include "file_stream_process.h"

namespace vidtk
{

file_stream_process
::file_stream_process( const vcl_string &name, bool in, bool out )
: process(name, "file_stream_process"), 
  disabled_(false), mode_append_(false), mode_input_(in), mode_output_(out)
{
  config_.add_parameter("filename", "Name of the file to open");
  config_.add_parameter("append", "false", 
                        "Whether or not to append an existing file");
  config_.add_parameter("binary", "true", 
                        "Whether or not ito use binary I/O instead of ASCII");
  config_.add_parameter("disabled", "false", 
                        "Enable or disable the file process");
}


file_stream_process
::~file_stream_process()
{
  this->stream_.close();
}


config_block 
file_stream_process
::params() const
{
  return this->config_;
}


bool 
file_stream_process
::set_params( const config_block &blk)
{
  try
  {
    blk.get("disabled", this->disabled_);
    if(!this->disabled_)
    {
      blk.get("filename", this->filename_);
      blk.get("append", this->mode_append_);
      blk.get("binary", this->mode_binary_);
    }
  }
  catch( unchecked_return_value& )
  {
    // Reset to old values
    this->set_params( this->config_ );
    return false;
  }
  
  this->config_.update( blk );
  return true;
} 


bool
file_stream_process 
::initialize()
{
  return true;
}


bool
file_stream_process
::step(void)
{
  if( this->disabled_ )
  {
    return true;
  }
  
  vcl_ios_openmode mode;
  vcl_memset(&mode, 0, sizeof(vcl_ios_openmode));

  if( this->mode_binary_ )
  {
    mode |= vcl_ios_binary;
  }
  if( this->mode_input_ ) 
  {
    mode |= vcl_ios_in;
  }
  if( this->mode_output_ )
  {
    mode |= vcl_ios_out;
    if( this->mode_append_ ) 
    {
      mode |= vcl_ios_app;
    }
    else
    { 
      mode |= vcl_ios_trunc;
    }
  }

  this->stream_.open( this->filename_.c_str(), mode );

  // Only execute once
  this->disabled_ = true;

  return this->stream_.good();
}


vcl_iostream& 
file_stream_process
::stream(void)
{
  return this->stream_;
}

}  // namespace vidtk


