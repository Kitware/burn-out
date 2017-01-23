/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "list_writer_process.h"

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_list_writer_process_txx__
VIDTK_LOGGER("list_writer_process_txx");


namespace vidtk
{

template<typename T>
list_writer_process<T>
::list_writer_process( const std::string &name )
: process(name, "list_writer_process"),
  disabled_(false), stream_(NULL), data_(NULL)
{
  config_.add_parameter("disabled", "false",
    "Whether or not the writer process is disabled");
}


template<typename T>
list_writer_process<T>
::~list_writer_process()
{
  if( this->stream_ )
  {
    this->stream_->flush();
  }
}


template<typename T>
config_block
list_writer_process<T>
::params() const
{
  return this->config_;
}


template<typename T>
bool
list_writer_process<T>
::set_params( const config_block &blk)
{
  try
  {
    this->disabled_ = blk.get<bool>("disabled");
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


template<typename T>
bool
list_writer_process<T>
::initialize()
{
  return true;
}


template<typename T>
bool
list_writer_process<T>
::step()
{
  if( this->disabled_ )
  {
    return true;
  }
  if( !this->stream_ || !this->data_ )
  {
    return false;
  }

  for( typename std::vector<T>::const_iterator i = this->data_->begin();
       i != this->data_->end();
       ++i )
  {
    (*(this->stream_)) << *i << "\n";
  }
  this->stream_->flush();

  return true;
}


template<typename T>
void
list_writer_process<T>
::set_stream( std::ostream &stream )
{
  this->stream_ = &stream;
}


template<typename T>
void
list_writer_process<T>
::set_data( const std::vector<T>& d )
{
  this->data_ = &d;
}

}
