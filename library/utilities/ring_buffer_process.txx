/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/ring_buffer_process.h>
#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <vcl_cassert.h>

namespace vidtk
{


template <class Data>
ring_buffer_process<Data>
::ring_buffer_process( vcl_string const& name )
  : process( name, "ring_buffer_process" ),
    disable_capacity_error_( false ),
    next_datum_( NULL ),
    disabled_(false)
{
  config_.add_parameter(
    "disabled",
    "false",
    "Whether or not to execute this block."
    );
  config_.add_parameter(
    "length",
    "1",
    "The maximum number of elements that can be held in the buffer."
    );
  config_.add_parameter(
    "disable_capacity_error",
    "false",
    "If set to \"true\", it will not be an error to request an element"
    " at position N when the length (capacity) is M < N.  Normally, this"
    " indicates an error, because there will never be an element at"
    " position N since the buffer does not have enough capacity."
    );
}


template <class Data>
ring_buffer_process<Data>
::~ring_buffer_process()
{
}


template <class Data>
config_block
ring_buffer_process<Data>
::params() const
{
  return config_;
}


template <class Data>
bool
ring_buffer_process<Data>
::set_params( config_block const& blk )
{
  try
  {
    unsigned length = blk.get<unsigned>( "length" );
    if( length == 0 )
    {
      throw unchecked_return_value( "length is 0" );
    }

    blk.get( "disabled", disabled_ );

    blk.get( "disable_capacity_error", disable_capacity_error_ );

    this->buffer_.resize( length );
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


template <class Data>
bool
ring_buffer_process<Data>
::initialize()
{
  this->clear();
  return true;
}


template <class Data>
bool
ring_buffer_process<Data>
::reset()
{
  return this->initialize();
}
 
template <class Data>
bool
ring_buffer_process<Data>
::step()
{
  if( disabled_ )
  {
    return true;
  }

  // Check to see if all required inputs are present.
  // Terminate if not.
  if ( NULL == next_datum_ )
  {
    return false;
  }

  this->insert( *next_datum_ );

  next_datum_ = NULL;

  return true;
}


template <class Data>
void
ring_buffer_process<Data>
::set_next_datum( Data const& item )
{
  next_datum_ = &item;
}


template <class Data>
Data const&
ring_buffer_process<Data>
::datum_nearest_to( unsigned offset ) const
{
  assert( this->size() > 0 );

  // If the capacity of the buffer means that it will be impossible to
  // get this offset, then it probably implies a overall configuration
  // error.
  if( ! disable_capacity_error_ &&
      offset >= this->capacity() )
  {
    log_fatal_error( name() << ": request for offset " << offset
                     << ", but capacity is " << this->capacity()
                     << ", so the requirement will never be met. Set "
                     << "disable_capacity_error to \"true\" to disable "
                     << "this check\n" );
  }

  // if beyond the bounds, return the last item.
  if( offset >= this->size() )
  {
    offset = this->size() - 1;
  }

  return this->datum_at( offset );
}


template <class Data>
vidtk::buffer<Data> const&
ring_buffer_process<Data>
::buffer() const
{
  return *this;
}


} // end namespace vidtk
