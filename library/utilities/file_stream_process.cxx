/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <cstring>
#include <logger/logger.h>

#include <iostream>

#include "file_stream_process.h"

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_file_stream_process_cxx__
VIDTK_LOGGER("file_stream_process_cxx");

namespace vidtk
{

file_stream_process
::file_stream_process( const std::string &_name, bool in, bool out )
: process(_name, "file_stream_process"),
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
    disabled_ = blk.get<bool>("disabled");
    if(!this->disabled_)
    {
      this->filename_ = blk.get<std::string>("filename");
      this->mode_append_ = blk.get<bool>("append");
      this->mode_binary_ = blk.get<bool>("binary");
    }
  }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
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

  std::ios::openmode mode;
  std::memset(&mode, 0, sizeof(std::ios::openmode));

  if( this->mode_binary_ )
  {
    mode |= std::ios::binary;
  }
  if( this->mode_input_ )
  {
    mode |= std::ios::in;
  }
  if( this->mode_output_ )
  {
    mode |= std::ios::out;
    if( this->mode_append_ )
    {
      mode |= std::ios::app;
    }
    else
    {
      mode |= std::ios::trunc;
    }
  }

  this->stream_.open( this->filename_.c_str(), mode );

  // Only execute once
  this->disabled_ = true;

  return this->stream_.good();
}


std::iostream&
file_stream_process
::stream(void)
{
  return this->stream_;
}

}  // namespace vidtk
