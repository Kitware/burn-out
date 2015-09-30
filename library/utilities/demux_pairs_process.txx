/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "demux_pairs_process.h"

namespace vidtk
{

template<typename T1, typename T2>
demux_pairs_process<T1, T2>
::demux_pairs_process( const vcl_string &name )
: process(name, "demux_pairs_process"), p_(NULL)
{
}


template<typename T1, typename T2>
demux_pairs_process<T1, T2>
::~demux_pairs_process()
{
}


template<typename T1, typename T2>
config_block 
demux_pairs_process<T1, T2>
::params() const
{
  return this->config_;
}


template<typename T1, typename T2>
bool 
demux_pairs_process<T1, T2>
::set_params( const config_block &blk)
{
  return true;
}


template<typename T1, typename T2>
bool 
demux_pairs_process<T1, T2>
::initialize()
{
  return true;
}


template<typename T1, typename T2>
bool 
demux_pairs_process<T1, T2>
::step()
{
  if( !this->p_ )
  {
    return false;
  }
  
  this->first_.clear();
  this->first_.reserve(this->p_->size());
  this->second_.clear();
  this->second_.reserve(this->p_->size());
  
  for( typename vcl_vector<vcl_pair_t1_t2>::iterator i = this->p_->begin();
       i != this->p_->end();
       ++i )
  {
    this->first_.push_back(i->first);
    this->second_.push_back(i->second);
  }

  return true;
}


template<typename T1, typename T2>
void 
demux_pairs_process<T1, T2>
::set_pairs( vcl_vector<vcl_pair<T1, T2> > &p )
{
  this->p_ = &p;
}


template<typename T1, typename T2>
vcl_vector<T1>& 
demux_pairs_process<T1, T2>
::first(void)
{
  return this->first_;
}


template<typename T1, typename T2>
vcl_vector<T2>& 
demux_pairs_process<T1, T2>
::second(void)
{
  return this->second_;
}


}

