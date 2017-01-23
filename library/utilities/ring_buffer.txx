/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ring_buffer.h"
#include <cassert>
#include <logger/logger.h>

VIDTK_LOGGER( "ring_buffer_cxx" );

namespace vidtk
{

template<class Data>
ring_buffer< Data >
::ring_buffer()
  : head_( 0 )
  , item_count_( 0 )
{
}


template<class Data>
ring_buffer< Data >
::~ring_buffer()
{
}


template<class Data>
void
ring_buffer< Data >
::set_capacity( unsigned cap )
{
  buffer_.resize( cap );
}


template<class Data>
unsigned
ring_buffer< Data >
::capacity() const
{
  return static_cast<unsigned>( buffer_.size() );
}


template<class Data>
unsigned
ring_buffer< Data >
::size() const
{
  return item_count_;
}


template<class Data>
void
ring_buffer< Data >
::insert( Data const& item )
{
  ++head_;

  unsigned const N = capacity();

  if ( N <= head_ )
  {
    head_ %= N;
  }

  buffer_[head_] = item;

  if ( item_count_ < N )
  {
    ++item_count_;
  }
}


template<class Data>
unsigned
ring_buffer< Data >
::head() const
{
  return head_;
}


template<class Data>
const Data&
ring_buffer< Data >
::datum_at( unsigned offset ) const
{
  assert( has_datum_at( offset ) );

  unsigned const N = capacity();

  int idx = static_cast<int>(head_) - static_cast<int>(offset) + static_cast<int>(N);
  if (idx < 0)
  {
    LOG_ERROR("Ring buffer logic error: head_ " << head_ << " offset " << offset << " N " << N << " idx " << idx );
  }
  return buffer_[ static_cast<unsigned>( idx % N ) ];
}


template<class Data>
bool
ring_buffer< Data >
::has_datum_at( unsigned offset ) const
{
  return ( offset < item_count_ );
}


template<class Data>
void
ring_buffer< Data >
::clear()
{
  head_ = 0;
  item_count_ = 0;
}


} // end namespace vidtk
