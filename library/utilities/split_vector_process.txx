/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "split_vector_process.h"

namespace vidtk
{

template<typename T, int Predicate(const T&)>
split_vector_process<T, Predicate>
::split_vector_process( const vcl_string &name )
: process(name, "split_vector_process"), in_(NULL)
{
}


template<typename T, int Predicate(const T&)>
split_vector_process<T, Predicate>
::~split_vector_process()
{
}


template<typename T, int Predicate(const T&)>
config_block 
split_vector_process<T, Predicate>
::params() const
{
  return this->config_;
}


template<typename T, int Predicate(const T&)>
bool 
split_vector_process<T, Predicate>
::set_params( const config_block &blk)
{
  return true;
}


template<typename T, int Predicate(const T&)>
bool 
split_vector_process<T, Predicate>
::initialize()
{
  return true;
}


template<typename T, int Predicate(const T&)>
bool 
split_vector_process<T, Predicate>
::step()
{
  if( !this->in_ )
  {
    return false;
  }
  
  this->positive_.clear();
  this->positive_.reserve(this->in_->size());
  this->zero_.clear();
  this->zero_.reserve(this->in_->size());
  this->negative_.clear();
  this->negative_.reserve(this->in_->size());
  
  for( typename vcl_vector<T>::const_iterator i = this->in_->begin();
       i != this->in_->end();
       ++i )
  {
    T tmp = *i;
    int r = Predicate(tmp);
    if( r > 0 )
    {
      this->positive_.push_back(*i);
    }
    else if( r < 0 )
    {
      this->negative_.push_back(*i);
    }
    else
    {
      this->zero_.push_back(*i);
    }
  }

  return true;
}


template<typename T, int Predicate(const T&)>
void 
split_vector_process<T, Predicate>
::set_input( const vcl_vector<T> &in )
{
  this->in_ = &in;
}


template<typename T, int Predicate(const T&)>
vcl_vector<T>& 
split_vector_process<T, Predicate>
::positive(void)
{
  return this->positive_;
}


template<typename T, int Predicate(const T&)>
vcl_vector<T>& 
split_vector_process<T, Predicate>
::zero(void)
{
  return this->zero_;
}


template<typename T, int Predicate(const T&)>
vcl_vector<T>& 
split_vector_process<T, Predicate>
::negative(void)
{
  return this->negative_;
}


}

