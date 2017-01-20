/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "moving_training_data_container.h"

#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "moving_training_data_container_txx" );

template< typename FeatureType >
moving_training_data_container< FeatureType >
::moving_training_data_container()
{
  this->configure( moving_training_data_settings() );
}


template< typename FeatureType >
void
moving_training_data_container< FeatureType >
::configure( const moving_training_data_settings& settings )
{
  max_entries_ = settings.max_entries;
  features_per_entry_ = settings.features_per_entry;
  insert_mode_ = settings.insert_mode;

  data_.resize( max_entries_ * features_per_entry_, 0 );

  this->reset();
}

template< typename FeatureType >
void
moving_training_data_container< FeatureType >
::reset()
{
  std::fill( data_.begin(), data_.end(), 0 );

  stored_entry_count_ = 0;
  current_position_ = 0;
}

template< typename FeatureType >
void
moving_training_data_container< FeatureType >
::insert( const feature_t* new_entry )
{
  this->insert( std::vector< const feature_t* >( 1, new_entry ) );
}

template< typename FeatureType >
void
moving_training_data_container< FeatureType >
::insert( const std::vector< const feature_t* >& new_entries )
{
  if( max_entries_ == 0 )
  {
    LOG_AND_DIE( "Cannot insert entries into buffer with capacity 0." );
  }

  if( new_entries.empty() )
  {
    return;
  }

  const unsigned new_entry_count = new_entries.size();
  const unsigned bytes_per_entry = features_per_entry_ * sizeof( feature_t );

  unsigned empty_slots = max_entries_ - stored_entry_count_;
  unsigned pivot_index = empty_slots;

  const std::ptrdiff_t entry_step = features_per_entry_;

  feature_t* dst_start = &data_[0];
  feature_t* dst_pointer;

  if( stored_entry_count_ < max_entries_ )
  {
    unsigned src_entries_to_copy = std::min( empty_slots, new_entry_count );

    for( unsigned i = 0; i < src_entries_to_copy; ++i )
    {
      dst_pointer = dst_start + ( stored_entry_count_ + i ) * entry_step;
      std::memcpy( dst_pointer, new_entries[i], bytes_per_entry );
    }

    stored_entry_count_ += src_entries_to_copy;
    current_position_ = stored_entry_count_ % max_entries_;
  }

  if( empty_slots < new_entry_count )
  {
    if( insert_mode_ == moving_training_data_settings::RANDOM )
    {
      for( unsigned i = pivot_index; i < new_entry_count; ++i )
      {
        unsigned random_index = std::rand() % stored_entry_count_;
        dst_pointer = dst_start + random_index * entry_step;
        std::memcpy( dst_pointer, new_entries[i], bytes_per_entry );
      }
    }
    else if( insert_mode_ == moving_training_data_settings::FIFO )
    {
      for( unsigned i = pivot_index; i < new_entry_count; ++i )
      {
        current_position_ = ( current_position_ + 1 ) % stored_entry_count_;
        dst_pointer = dst_start + current_position_ * entry_step;
        std::memcpy( dst_pointer, new_entries[i], bytes_per_entry );
      }
    }
  }
}

template< typename FeatureType >
void
moving_training_data_container< FeatureType >
::insert( const data_block_t& new_data_block )
{
  if( new_data_block.empty() )
  {
    return;
  }

  std::vector< const feature_t* > entry_pointers;

  if( features_per_entry_ != 0 )
  {
    if( new_data_block.size() % features_per_entry_ != 0 )
    {
      LOG_AND_DIE( "Input data block has invalid size" );
    }

    unsigned new_entries = new_data_block.size() / features_per_entry_;

    for( unsigned i = 0; i < new_entries; ++i )
    {
      entry_pointers.push_back( &new_data_block[ i * features_per_entry_ ] );
    }
  }

  this->insert( entry_pointers );
}

template< typename FeatureType >
void
moving_training_data_container< FeatureType >
::insert( const std::vector< data_block_t >& new_entries )
{
  if( new_entries.empty() )
  {
    return;
  }

  std::vector< feature_t const* > entry_pointers;

  if( features_per_entry_ != 0 )
  {
    for( unsigned i = 0; i < new_entries.size(); ++i )
    {
      if( new_entries[i].size() != features_per_entry_ )
      {
        LOG_AND_DIE( "Input entry has invalid size" );
      }

      entry_pointers.push_back( &(new_entries[i][0]) );
    }
  }

  this->insert( entry_pointers );
}

template< typename FeatureType >
unsigned
moving_training_data_container< FeatureType >
::size() const
{
  return stored_entry_count_;
}

template< typename FeatureType >
bool
moving_training_data_container< FeatureType >
::empty() const
{
  return ( stored_entry_count_ == 0 );
}

template< typename FeatureType >
unsigned
moving_training_data_container< FeatureType >
::capacity() const
{
  return max_entries_;
}

template< typename FeatureType >
unsigned
moving_training_data_container< FeatureType >
::feature_count() const
{
  return features_per_entry_;
}

template< typename FeatureType >
FeatureType const*
moving_training_data_container< FeatureType >
::entry( unsigned id ) const
{
  return &data_[ id * feature_count() ];
}

template< typename FeatureType >
typename moving_training_data_container< FeatureType >::data_block_t const&
moving_training_data_container< FeatureType >
::raw_data() const
{
  return data_;
}

template< typename FeatureType >
std::ostream& operator<<( std::ostream& out, const moving_training_data_container< FeatureType >& data )
{
  out << std::endl << "Data buffer entries: " << std::endl;
  out << std::endl << data.size() << std::endl;
  out << std::endl << "Data buffer contents: " << std::endl;

  for( unsigned i = 0; i < data.size(); ++i )
  {
    out << std::endl;

    for( unsigned j = 0; j < data.feature_count(); ++j )
    {
      out << static_cast< unsigned >( data.entry( i )[ j ] ) << " ";
    }
  }

  out << std::endl;

  return out;
}

} // end namespace vidtk
