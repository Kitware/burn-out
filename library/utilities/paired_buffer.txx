/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "paired_buffer.h"


#include <algorithm>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_paired_buffer_txx__
VIDTK_LOGGER("paired_buffer_txx");


namespace vidtk
{

// ----------------------- Find datum related ----------------------------


template< class keyT, class datumT >
paired_buffer< keyT, datumT >
::paired_buffer()
  : length_( 0 ),
    default_return_value_( datumT() )
{}

template< class keyT, class datumT >
paired_buffer< keyT, datumT >
::paired_buffer( unsigned len )
  : length_( len ),
    default_return_value_( datumT() )
{}

template< class keyT, class datumT >
bool
paired_buffer< keyT, datumT >
::initialize()
{
  // Currently using an arbitrary min_capacity. If there is a need in the
  // future one can expose this as a config parameter.
  unsigned min_capacity = length_ / 3;
  typename buffer_t::capacity_type capacity_cntl( length_, min_capacity );
  buffer_.set_capacity( capacity_cntl );
  return true;
}

template< class keyT, class datumT >
bool
paired_buffer< keyT, datumT >
::reset()
{
  buffer_.clear();
  return true;
}

template< class keyT, class datumT >
void
paired_buffer< keyT, datumT >
::set_length( unsigned len )
{
  length_ = len;
}

template< class keyT, class datumT >
datumT const&
paired_buffer< keyT, datumT >
::find_datum( keyT const& key,
              bool & found,
              bool is_sorted ) const
{
  typename buffer_t::const_iterator iter;
  found = find_datum( key, iter, is_sorted );
  if( found )
  {
    return iter->datum_;
  }

  // reset the value each time because the caller may
  // have changed it if they ignored the found flag in
  // the non-const version.

  default_return_value_ = datumT();
  return default_return_value_;
}

template< class keyT, class datumT >
datumT &
paired_buffer< keyT, datumT >
::find_datum( keyT const& key,
              bool & found,
              bool is_sorted )
{
  // Calling the const version
  const paired_buffer< keyT, datumT > * const_this;
  const_this = const_cast< const paired_buffer< keyT, datumT > * >( this );

  // note that if the caller chooses to ignore the found flag, the default value
  // may be changed; see comment in const version above
  return const_cast< datumT & >( const_this->find_datum( key, found, is_sorted ) );
}

template< class keyT, class datumT >
bool
paired_buffer< keyT, datumT >
::find_datum( keyT const& key,
              typename paired_buffer< keyT, datumT >::buffer_t::const_iterator & iter,
              bool is_sorted ) const
{
  bool found = false;
  typename paired_buffer< keyT, datumT >::key_datum_t search_pair( key, datumT() );

  if( is_sorted )
  {
    std::pair< typename buffer_t::const_iterator,
              typename buffer_t::const_iterator > res_range;
    res_range = std::equal_range( buffer_.begin(), buffer_.end(), search_pair );
    if( res_range.first == res_range.second )
    {
      found = false;
    }
    else if( res_range.first+1 != res_range.second )
    {
      //LOG_ERROR( "Found duplicate entry in the buffer." );
      found = false;
    }
    else
    {
      found = true;
      iter = res_range.first;
    }
  }
  else
  {
    typename buffer_t::const_iterator res;
    res = std::find( buffer_.begin(), buffer_.end(), search_pair );
    if( res != buffer_.end() )
    {
      found = true;
      iter = res;
    }
  }

  return found;
}

template< class keyT, class datumT >
bool
paired_buffer< keyT, datumT >
::replace_items( std::vector< typename paired_buffer< keyT, datumT >::key_datum_t > const& /*replacements*/ )
{
  // TODO: *Replace* buffer_ with entries in uis.
  LOG_ERROR("set_updated_items() not implemented yet." );
  return false;
}

} // namespace vidtk
