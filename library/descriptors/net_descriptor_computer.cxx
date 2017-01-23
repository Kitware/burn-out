/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "net_descriptor_computer.h"

#include <algorithm>
#include <limits>

#include <boost/foreach.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "net_descriptor_computer_cxx" );


net_descriptor_computer
::net_descriptor_computer()
  : enabled_( false )
{
}


net_descriptor_computer
::~net_descriptor_computer()
{
}

void
merge_vector_into( const std::vector< double >& source,
                   std::vector< double >& dest,
                   const double weight )
{
  const unsigned start_pos = dest.size();

  dest.insert( dest.end(), source.begin(), source.end() );

  for( unsigned i = start_pos; i < dest.size(); ++i )
  {
    dest[i] = dest[i] * weight;
  }
}

void
average_descriptors( const std::vector< descriptor_sptr_t >& to_merge,
                     std::vector< double >& output )
{
  if( to_merge.empty() )
  {
    return;
  }

  const unsigned desc_size = to_merge[0]->get_features().size();
  output = std::vector< double >( desc_size, 0.0 );
  unsigned counter = 0;

  for( unsigned i = 0; i < to_merge.size(); ++i )
  {
    std::vector< double >& feats = to_merge[i]->get_features();

    if( feats.size() != desc_size )
    {
      continue;
    }

    for( unsigned j = 0; j < feats.size(); ++j )
    {
      output[j] += feats[j];
    }

    counter++;
  }

  if( counter != 0 )
  {
    for( unsigned j = 0; j < output.size(); ++j )
    {
      output[j] = output[j] / counter;
    }
  }
}


bool
net_descriptor_computer
::configure( const net_descriptor_computer_settings& settings )
{
  if( settings.to_merge.size() != settings.merge_weights.size() )
  {
    LOG_ERROR( "Descriptor merge ID and weight vector sizes must match" );
    return false;
  }

  enabled_ = !settings.to_merge.empty();
  settings_ = settings;
  return true;
}


void
net_descriptor_computer
::compute( const track::vector_t& tracks, descriptor_vec_t& descriptors )
{
  if( !enabled_ )
  {
    return;
  }

  track_demuxer_.add_tracks( tracks );

  // Add new tracks to stored history
  BOOST_FOREACH( descriptor_sptr_t descriptor, descriptors )
  {
    BOOST_FOREACH( track::track_id_t track_id, descriptor->get_track_ids() )
    {
      descriptors_[ track_id ].first++;
      descriptors_[ track_id ].second[ descriptor->get_type() ].push_back( descriptor );
    }
  }

  // Check if any new joint descriptors should be generated based on samping criteria
  if( settings_.sampling_rate > 0 )
  {
    for( track_descriptor_map_t::iterator p = descriptors_.begin(); p != descriptors_.end(); p++ )
    {
      unsigned& counter = ( p->second ).first;

      if( counter > 0 && counter % settings_.sampling_rate )
      {
        descriptor_sptr_t new_desc = generate_net_descriptor( ( p->second ).second );

        if( new_desc )
        {
          descriptors.push_back( new_desc );
        }

        counter++;
      }
    }
  }

  // Call terminated track functions for all terminated tracks and remove prior data for track
  BOOST_FOREACH( track_sptr track, track_demuxer_.get_terminated_tracks() )
  {
    track_descriptor_map_t::iterator p = descriptors_.find( track->id() );

    if( p != descriptors_.end() )
    {
      descriptor_sptr_t new_desc = generate_net_descriptor( ( p->second ).second );

      if( new_desc )
      {
        descriptors.push_back( new_desc );
      }

      descriptors_.erase( track->id() );
    }
  }
}

descriptor_sptr_t
net_descriptor_computer
::generate_net_descriptor( const descriptor_map_t& descriptors )
{
  descriptor_sptr_t new_desc;

  if( descriptors.size() < settings_.to_merge.size() )
  {
    return new_desc;
  }

  std::vector< const descriptor_vec_t* > ordered_entries;

  for( unsigned i = 0; i < settings_.to_merge.size(); ++i )
  {
    for( descriptor_map_t::const_iterator p = descriptors.begin(); p != descriptors.end(); p++ )
    {
      if( p->first.find( settings_.to_merge[i] ) != std::string::npos )
      {
        ordered_entries.push_back( &( p->second ) );
        break;
      }
    }
  }

  if( ordered_entries.size() == settings_.to_merge.size() )
  {
    // Formulate a new descriptor
    new_desc = raw_descriptor::create( settings_.merged_id );

    for( unsigned i = 0; i < ordered_entries.size(); ++i )
    {
      std::vector< double > average;
      average_descriptors( *ordered_entries[i], average );
      merge_vector_into( average, new_desc->get_features(), settings_.merge_weights[i] );
    }

    if( settings_.normalize_merge )
    {
      normalize_descriptor( new_desc );
    }

    new_desc->add_track_ids( ordered_entries[0]->back()->get_track_ids() );
    new_desc->set_history( ordered_entries[0]->back()->get_history() );
  }

  return new_desc;
}

} // end namespace vidtk
