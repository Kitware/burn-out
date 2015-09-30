/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>

#include "list_writer_process.h"

namespace vidtk
{

template<typename T>
list_writer_process<T>
::list_writer_process( const vcl_string &name )
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
    blk.get("disabled", this->disabled_);
  }
  catch( unchecked_return_value& )
  {
    // Reset to old values
    log_error( name() << ": couldn't set parameters\n" );
    this->set_params( this->config_ );
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
  
  for( typename vcl_vector<T>::const_iterator i = this->data_->begin();
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
::set_stream( vcl_ostream &stream )
{
  this->stream_ = &stream;
}


template<typename T>
void 
list_writer_process<T>
::set_data( const vcl_vector<T>& d )
{
  this->data_ = &d;
}

}

