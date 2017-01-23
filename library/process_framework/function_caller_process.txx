/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "function_caller_process.h"

namespace vidtk
{


template <class ArgType>
function_caller_process<ArgType>
::function_caller_process( std::string const& _name )
  : process( _name, "function_caller_process" )
{
}


template <class ArgType>
function_caller_process<ArgType>
::~function_caller_process()
{
}


template <class ArgType>
config_block
function_caller_process<ArgType>
::params() const
{
  return config_block();
}


template <class ArgType>
bool
function_caller_process<ArgType>
::set_params( config_block const& /*blk*/ )
{
  return true;
}


template <class ArgType>
bool
function_caller_process<ArgType>
::initialize()
{
  return true;
}


template <class ArgType>
bool
function_caller_process<ArgType>
::step()
{
  func_( this->arg1_.value() );

  return true;
}


template <class ArgType>
void
function_caller_process<ArgType>
::set_function( boost::function< void(ArgType) > const& f )
{
  func_ = f;
}


template <class ArgType>
void
function_caller_process<ArgType>
::set_argument( ArgType v )
{
  arg1_ = helper::holder<ArgType>( v );
}


} // end namespace vidtk
