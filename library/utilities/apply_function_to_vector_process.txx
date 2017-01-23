/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "apply_function_to_vector_process.h"

namespace vidtk
{

template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
apply_function_to_vector_process<IN_T, OUT_T, Function>
::apply_function_to_vector_process( const std::string &_name )
: process(_name, "apply_function_to_vector_process"), in_(NULL)
{
}


template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
apply_function_to_vector_process<IN_T, OUT_T, Function>
::~apply_function_to_vector_process()
{
}


template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
config_block
apply_function_to_vector_process<IN_T, OUT_T, Function>
::params() const
{
  return this->config_;
}


template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
bool
apply_function_to_vector_process<IN_T, OUT_T, Function>
::set_params( const config_block &/*blk*/)
{
  return true;
}


template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
bool
apply_function_to_vector_process<IN_T, OUT_T, Function>
::initialize()
{
  return true;
}


template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
bool
apply_function_to_vector_process<IN_T, OUT_T, Function>
::step()
{
  if( !this->in_ )
  {
    return false;
  }

  this->out_.clear();
  for( typename std::vector<IN_T>::const_iterator i = this->in_->begin();
       i != this->in_->end();
       ++i )
  {
    IN_T const & tmp = *i;
    out_.push_back(Function(tmp));
  }

  return true;
}


template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
void
apply_function_to_vector_process<IN_T, OUT_T, Function>
::set_input( const std::vector<IN_T> &in )
{
  this->in_ = &in;
}


template<typename IN_T, typename OUT_T, OUT_T Function(const IN_T&)>
std::vector<OUT_T>
apply_function_to_vector_process<IN_T, OUT_T, Function>
::get_output(void)
{
  return this->out_;
}


}

