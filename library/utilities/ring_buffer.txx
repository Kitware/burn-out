/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "ring_buffer.h"

#include <vcl_cassert.h>

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
::set_capacity( unsigned size )
{
  buffer_.resize( size );
}


template<class Data>
unsigned
ring_buffer< Data >
::capacity() const
{
  return buffer_.size();
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
Data const&
ring_buffer< Data >
::datum_at( unsigned offset ) const
{
  assert( has_datum_at( offset ) );

  unsigned const N = capacity();

  return buffer_[( head_ - offset + N ) % N];
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
