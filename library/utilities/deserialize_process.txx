/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vsl/vsl_binary_io.h>

#include <utilities/unchecked_return_value.h>

#include "deserialize_process.h"

#include <logger/logger.h>
VIDTK_LOGGER("deserialize_process_txx");


namespace vidtk
{

template<typename T>
deserialize_process<T>
::deserialize_process( const std::string &name )
: process(name, "deserialize_process"),
  disabled_(false),
  stream_(NULL), bstream_(NULL), data_(NULL)
{
  config_.add_parameter("disabled", "false",
    "Whether or not the deserialization process is disabled");
}


template<typename T>
deserialize_process<T>
::~deserialize_process()
{
  if( this->bstream_ )
  {
    delete this->bstream_;
  }
}


template<typename T>
config_block
deserialize_process<T>
::params() const
{
  return this->config_;
}


template<typename T>
bool
deserialize_process<T>
::set_params( const config_block &blk)
{
  try
  {
    blk.get("disabled", this->disabled_);
  }
  catch( unchecked_return_value& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  return true;
}


template<typename T>
bool
deserialize_process<T>
::initialize()
{
  return true;
}


template<typename T>
bool
deserialize_process<T>
::step()
{
  if( this->disabled_ )
  {
    return true;
  }
  if( !this->stream_ )
  {
    return false;
  }
  this->stream_->peek();
  if( !this->stream_->good() )
  {
    return false;
  }
  if( !this->bstream_ )
  {
    this->bstream_ = new vsl_b_istream( this->stream_ );
  }

  this->bstream_->is().peek();
  if( !(this->bstream_->is().good()) ) { return false; }


  if( this->data_ )
  {
    delete this->data_;
  }
  this->data_ = new T;
  this->bstream_->clear_serialisation_records();
  vsl_b_read( *this->bstream_, *this->data_ );

  return this->bstream_->is().good();
}


template<typename T>
void
deserialize_process<T>
::set_stream( std::istream &stream )
{
  this->stream_ = &stream;
}


template<typename T>
T&
deserialize_process<T>
::data(void)
{
  return *this->data_;
}

}
