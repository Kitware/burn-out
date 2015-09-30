/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_async_observer_process_txx_
#define vidtk_async_observer_process_txx_

#include "async_observer_process.h"

namespace vidtk
{

template<typename T>
async_observer_process<T>
::async_observer_process( const vcl_string &name )
: process(name, "async_observer_process"),
  data_available_(false),
  data_set_(false),
  is_done_(false)
{
}


template<typename T>
async_observer_process<T>
::~async_observer_process()
{
}


template<typename T>
config_block
async_observer_process<T>
::params() const
{
  return this->config_;
}


template<typename T>
bool
async_observer_process<T>
::set_params( const config_block &/*blk*/)
{
  return true;
}


template<typename T>
bool
async_observer_process<T>
::initialize()
{
  return true;
}


template<typename T>
bool
async_observer_process<T>
::step()
{
  {
    boost::unique_lock<boost::mutex> lock(this->mut_);
    while( this->data_available_ )
    {
      this->cond_data_available_.wait(lock);
    }
    if( !this->data_set_ )
    {
      this->is_done_ = true;
    }
    this->data_available_ = true;
    this->data_set_ = false;
  }
  this->cond_data_available_.notify_one();
  return !this->is_done_;
}


template<typename T>
void
async_observer_process<T>
::set_input( const T& data )
{
  {
    boost::unique_lock<boost::mutex> lock(this->mut_);
    while( this->data_available_ )
    {
      this->cond_data_available_.wait(lock);
    }
    this->data_set_ = true;
    this->data_ = data;
  }
}


template<typename T>
bool
async_observer_process<T>
::get_data(T& data)
{
  {
    boost::unique_lock<boost::mutex> lock(this->mut_);
    while( !this->data_available_ )
    {
      this->cond_data_available_.wait(lock);
    }
    if( this->is_done_ )
    {
      return false;
    }
    this->data_available_ = false;
    data = this->data_;
  }
  this->cond_data_available_.notify_one();
  return true;
}

}

#endif
