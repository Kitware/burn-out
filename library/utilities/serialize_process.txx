/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_serialize_process_txx_
#define vidtk_serialize_process_txx_

#include <vsl/vsl_binary_io.h>
#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>

#include "serialize_process.h"

namespace vidtk
{

template<typename T>
serialize_process<T>
::serialize_process( const vcl_string &name )
: process(name, "serialize_process"), 
  disabled_(false),
  stream_(NULL), bstream_(NULL), data_(NULL)
{
  config_.add_parameter("disabled", "false", 
                        "Whether or not the serialization process is disabled");
}


template<typename T>
serialize_process<T>
::~serialize_process()
{
  if( this->bstream_ )
  {
    this->bstream_->os().flush();
    delete this->bstream_;
  }
}


template<typename T>
config_block 
serialize_process<T>
::params() const
{
  return this->config_;
}


template<typename T>
bool 
serialize_process<T>
::set_params( const config_block &blk)
{
  try
  {
    blk.get("disabled", this->disabled_);
  }
  catch( unchecked_return_value& )
  {
    // Reset to old values
    log_error( name() << ": couldn't set parameters\n" );
    this->set_params( this->config_ );
    return false;
  }

  return true;
}


template<typename T>
bool 
serialize_process<T>
::initialize()
{
  return true;
}


template<typename T>
bool 
serialize_process<T>
::step()
{
  if( this->disabled_ )
  {
    return true;
  }
  if( !(this->data_) || !(this->stream_) || !(this->stream_->good()) )
  {
    return false;
  }
  
  if( !this->bstream_ )
  {
    this->bstream_ = new vsl_b_ostream( this->stream_ );
    this->stream_->flush();
  }

  this->bstream_->clear_serialisation_records();
  vsl_b_write( *this->bstream_, *this->data_ );
  this->data_ = NULL;
  return this->bstream_->os();
}


template<typename T>
void 
serialize_process<T>
::set_stream( vcl_ostream &stream )
{
  this->stream_ = &stream;
}


template<typename T>
void 
serialize_process<T>
::set_data( const T& d )
{
  this->data_ = &d;
}

}
#endif
