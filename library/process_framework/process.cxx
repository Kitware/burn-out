/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <process_framework/process.h>

namespace vidtk
{


vcl_string const&
process
::name() const
{
  return name_;
}


vcl_string const&
process
::class_name() const
{
  return class_name_;
}


void
process
::set_name( vcl_string const& name )
{
  name_ = name;
}


process
::process( vcl_string const& name,
           vcl_string const& class_name )
  : push_output_func_( NULL ),
    name_( name ),
    class_name_( class_name )
{
}


bool
process
::set_param( vcl_string const& name, vcl_string const& value )
{
  config_block blk = this->params();
  if( blk.set( name, value ) )
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
