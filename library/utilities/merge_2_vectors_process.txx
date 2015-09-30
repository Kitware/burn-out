/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_MERGE_2VECTORS_PROCESS_TXX_
#define VIDTK_MERGE_2VECTORS_PROCESS_TXX_

#include "merge_2_vectors_process.h"

namespace vidtk
{

template<typename T>
merge_2_vectors_process<T>
::merge_2_vectors_process( const vcl_string &name )
: process(name, "merge_2_vectors_process"), v1_(NULL), v2_(NULL)
{
}


template<typename T>
merge_2_vectors_process<T>
::~merge_2_vectors_process()
{
}


template<typename T>
config_block 
merge_2_vectors_process<T>
::params() const
{
  return this->config_;
}


template<typename T>
bool 
merge_2_vectors_process<T>
::set_params( const config_block &blk)
{
  return true;
}


template<typename T>
bool 
merge_2_vectors_process<T>
::initialize()
{
  return true;
}


template<typename T>
bool 
merge_2_vectors_process<T>
::step()
{
  if( !this->v1_ || !this->v2_)
  {
    return false;
  }
  
  this->v_all_.clear();
  this->v_all_.reserve(this->v1_->size() + this->v2_->size());
  this->v_all_.insert(this->v_all_.end(), this->v1_->begin(), 
                      this->v1_->end());
  this->v_all_.insert(this->v_all_.end(), this->v2_->begin(), 
                      this->v2_->end());
  
  this->v1_ = NULL;
  this->v2_ = NULL;
 
  return true;
}


template<typename T>
void 
merge_2_vectors_process<T>
::set_v1( const vcl_vector<T> &v1 )
{
  this->v1_ = &v1;
}


template<typename T>
void 
merge_2_vectors_process<T>
::set_v2( const vcl_vector<T> &v2 )
{
  this->v2_ = &v2;
}


template<typename T>
vcl_vector<T>& 
merge_2_vectors_process<T>
::v_all( void )
{
  return this->v_all_;
}


}

#endif

