/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "apply_functor_to_vector_process.h"

namespace vidtk
{

// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::apply_functor_to_vector_process( const std::string &_name,
                                    boost::shared_ptr<FUNCTOR> func )
  : process(_name, "apply_functor_to_vector_process"),
    in_(NULL),
    functor_( func )
{
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::~apply_functor_to_vector_process()
{
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
config_block
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::params() const
{
  return this->config_;
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
bool
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::set_params( const config_block &/*blk*/)
{
  return true;
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
bool
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::initialize()
{
  return true;
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
bool
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::step()
{
  // No input received, we must be done.
  if( !this->in_ )
  {
    return false;
  }

  this->out_.clear();

  // Iterate over vector and apply function
  for( typename std::vector<IN_T>::const_iterator i = this->in_->begin();
       i != this->in_->end();
       ++i )
  {
    IN_T const & tmp = *i;
    out_.push_back( this->functor_->operator()(tmp) );
  }

  return true;
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
void
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::input( const std::vector<IN_T> &in )
{
  this->in_ = &in;
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
std::vector<OUT_T>
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::output(void)
{
  return this->out_;
}


// ----------------------------------------------------------------
template<typename IN_T, typename OUT_T, class FUNCTOR>
void
apply_functor_to_vector_process<IN_T, OUT_T, FUNCTOR >
::reset_functor( boost::shared_ptr< FUNCTOR > func )
{
  this->functor_ = func;
}

} // end namespace
