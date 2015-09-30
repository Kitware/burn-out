/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "function_caller_2_process.h"

namespace vidtk
{


template <class Arg1Type, class Arg2Type>
function_caller_2_process<Arg1Type,Arg2Type>
::function_caller_2_process( vcl_string const& name )
  : process( name, "function_caller_2_process" )
{
}


template <class Arg1Type, class Arg2Type>
function_caller_2_process<Arg1Type,Arg2Type>
::~function_caller_2_process()
{
}


template <class Arg1Type, class Arg2Type>
config_block
function_caller_2_process<Arg1Type,Arg2Type>
::params() const
{
  return config_block();
}


template <class Arg1Type, class Arg2Type>
bool
function_caller_2_process<Arg1Type,Arg2Type>
::set_params( config_block const& /*blk*/ )
{
  return true;
}


template <class Arg1Type, class Arg2Type>
bool
function_caller_2_process<Arg1Type,Arg2Type>
::initialize()
{
  return true;
}


template <class Arg1Type, class Arg2Type>
bool
function_caller_2_process<Arg1Type,Arg2Type>
::step()
{
  func_( arg1_.value(), arg2_.value() );

  return true;
}


template <class Arg1Type, class Arg2Type>
void
function_caller_2_process<Arg1Type,Arg2Type>
::set_function( boost::function< void(Arg1Type,Arg2Type) > const& f )
{
  func_ = f;
}


template <class Arg1Type, class Arg2Type>
void
function_caller_2_process<Arg1Type,Arg2Type>
::set_argument_1( Arg1Type v )
{
  arg1_ = helper::holder<Arg1Type>( v );
}


template <class Arg1Type, class Arg2Type>
void
function_caller_2_process<Arg1Type,Arg2Type>
::set_argument_2( Arg2Type v )
{
  arg2_ = helper::holder<Arg2Type>( v );
}


} // end namespace vidtk
