/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/holder_process.h>
#include <utilities/unchecked_return_value.h>
#include <vcl_cassert.h>

namespace vidtk
{


template <class TData>
holder_process<TData>
::holder_process( vcl_string const& name )
  : process( name, "holder_process" ),
    get_new_value_( false ),
    default_value_(),
    input_datum_( NULL ),
    output_datum_()
{
  config_.add_parameter( "increase_count",
    "false",
    "If set to \"false\", then only the value from \"default_value\" parameter "
    " will be used. If set to \"true\", then the \"default_value\" will be ignored"
    " and the value on input port after before the first step will be used. "
    " NOTE: This parameter name might be misleading, but is only maintained "
    " back-ward compatibility." );

  config_.add_parameter( "default_value",
    "",
    "Datum value used instead of the one available on the input data port."
    " Use of this value is controlled by \"increase_count\" parameter." );
}

template <class TData>
holder_process<TData>
::~holder_process()
{
}


template <class TData>
config_block
holder_process<TData>
::params() const
{
  return config_;
}


template <class TData>
bool
holder_process<TData>
::set_params( config_block const& blk )
{
  try
  {
    bool tmp = blk.get<bool>( "increase_count" );
    if ( !tmp )
    {
      get_new_value_ = true;
    }

    default_value_ = blk.get<TData> ("default_value");
  }
  catch( unchecked_return_value& )
  {
    // reset to old values
    this->set_params( this->config_ );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class TData>
bool
holder_process<TData>
::initialize()
{
  return true;
}


template <class TData>
bool
holder_process<TData>
::step()
{
  if( input_datum_ )
  {
    // only capture the input value once
    if ( get_new_value_ )
    {
      default_value_ = *input_datum_;
      get_new_value_ = false;
    }
  }
  
  output_datum_ = default_value_;

  input_datum_ = NULL;

  return true;
}


template <class TData>
void
holder_process<TData>
::set_input_datum( TData const& item )
{
  input_datum_ = &item;
}

template <class TData>
TData &
holder_process<TData>
::get_output_datum() 
{
  return output_datum_;
}

template <class TData>
void
holder_process<TData>
::clear() 
{
  output_datum_ = TData();
}

template <class TData>
void
holder_process<TData>
::set_default_value( TData const& item )
{
  default_value_ = item;
}

} // end namespace vidtk
