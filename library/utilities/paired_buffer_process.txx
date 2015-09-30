/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "paired_buffer_process.h"

#include <utilities/log.h>

namespace vidtk
{

template< class keyT, class datumT >
paired_buffer_process< keyT, datumT >
::paired_buffer_process( vcl_string const& name )
  : process( name, "paired_buffer_process" ),
    next_key_( NULL ),
    next_datum_( NULL )
{
  config_.add_parameter( "length", "0",
    "Maximum number of items to store in the circular buffer." );
}

template< class keyT, class datumT >
config_block
paired_buffer_process< keyT, datumT >
::params() const
{
  return config_;
}

template< class keyT, class datumT >
bool
paired_buffer_process< keyT, datumT >
::set_params( config_block const& blk )
{
  try
  {
    unsigned length = blk.get<unsigned>( "length" );
    paired_buff_.set_length( length );
  }
  catch( config_block_parse_error & e )
  {
    log_error( name() << ": failed to set parameters, "<< e.what() <<"\n" );
    return false;
  }

  return true;
}

template< class keyT, class datumT >
bool
paired_buffer_process< keyT, datumT >
::initialize()
{
  paired_buff_.initialize();
  return true;
}

template< class keyT, class datumT >
bool
paired_buffer_process< keyT, datumT >
::step()
{
  if( !next_key_ || !next_datum_ )
  {
    return false;
  }

  typename paired_buffer_process< keyT, datumT >::pair_t 
    next_pair( *next_key_, *next_datum_ );

  paired_buff_.buffer_.push_back( next_pair );

  next_key_ = NULL;
  next_datum_ = NULL;
  return true;
}

// ---------------------------- I/O ports --------------------------------

template< class keyT, class datumT >
void
paired_buffer_process< keyT, datumT >
::set_next_key( keyT const& k )
{
  next_key_ = &k;
}

template< class keyT, class datumT >
void
paired_buffer_process< keyT, datumT >
::set_next_datum( datumT const& d )
{
  next_datum_ = &d;
}

template< class keyT, class datumT >
typename paired_buffer_process< keyT, datumT >::paired_buff_t const&
paired_buffer_process< keyT, datumT >
::buffer() const
{
  return paired_buff_;
}

template< class keyT, class datumT >
void 
paired_buffer_process< keyT, datumT >
::set_updated_items( vcl_vector< typename paired_buffer_process< keyT, datumT >::pair_t > const& uis )
{
  // TODO: *Replace* buffer_ with entries in uis.
}

} // namespace vidtk
