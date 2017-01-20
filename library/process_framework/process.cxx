/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <process_framework/process.h>

namespace vidtk
{


std::string const&
process
::name() const
{
  return name_;
}


std::string const&
process
::class_name() const
{
  return class_name_;
}


void
process
::set_name( std::string const& _name )
{
  name_ = _name;
}


process
::process( std::string const& _name,
           std::string const& _class_name )
  : push_output_func_( NULL ),
    name_( _name ),
    class_name_( _class_name )
{
}


bool
process
::set_param( std::string const& _name, std::string const& value )
{
  config_block blk = this->params();
  if( blk.set( _name, value ) )
  {
    this->set_params( blk );
    return true;
  }
  else
  {
    return false;
  }
}


bool
process::
push_output(step_status status)
{
  if( this->push_output_func_ )
  {
    this->push_output_func_(status);
    return true;
  }
  return false;
}


void
delete_process_object( process* p )
{
  delete p;
}


} // end namespace vidtk
