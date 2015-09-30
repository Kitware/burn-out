/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_MERGE_NVECTORS_PROCESS_TXX_
#define VIDTK_MERGE_NVECTORS_PROCESS_TXX_

#include "merge_n_vectors_process.h"

namespace vidtk
{

template<typename T>
merge_n_vectors_process<T>
::merge_n_vectors_process( const vcl_string &name )
: process(name, "merge_n_vectors_process"), v_in_(0), size_(0)
{
}


template<typename T>
merge_n_vectors_process<T>
::~merge_n_vectors_process()
{
}


template<typename T>
config_block
merge_n_vectors_process<T>
::params() const
{
  return this->config_;
}


template<typename T>
bool
merge_n_vectors_process<T>
::set_params( const config_block &/*blk*/)
{
  return true;
}


template<typename T>
bool
merge_n_vectors_process<T>
::initialize()
{
  return true;
}


template<typename T>
bool
merge_n_vectors_process<T>
::step()
{
  if( !v_in_.size() )
  {
    return false;
  }

  this->v_all_.clear();
  this->v_all_.reserve(this->size_);
  for( unsigned int i = 0; i < v_in_.size(); ++i )
  {
    this->v_all_.insert( this->v_all_.end(), this->v_in_[i]->begin(),
                         this->v_in_[i]->end() );
  }

  this->v_in_.clear();
  this->size_ = 0;
  return true;
}


template<typename T>
void
merge_n_vectors_process<T>
::add_vector( const vcl_vector<T> &v1 )
{
  this->v_in_.push_back(&v1);
  this->size_+=v1.size();
}


template<typename T>
vcl_vector<T>&
merge_n_vectors_process<T>
::v_all( void )
{
  return this->v_all_;
}


}

#endif

