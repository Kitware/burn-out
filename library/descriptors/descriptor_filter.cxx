/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "descriptor_filter.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "descriptor_filter_cxx" );


descriptor_filter
::descriptor_filter()
  : any_filter_enabled_( false ),
    merge_filter_enabled_( false )
{
}


descriptor_filter
::~descriptor_filter()
{
}


void
merge_descriptors( const std::vector< descriptor_sptr_t >& to_merge,
                   const std::vector< double > weights,
                   raw_descriptor& output )
{
  // Copy raw descriptor data
  unsigned tot_size = 0;

  for( unsigned i = 0; i < to_merge.size(); ++i )
  {
    tot_size += to_merge[i]->features_size();
  }

  output.resize_features( tot_size );

  double *pos = &( output.get_features()[0] );

  for( unsigned i = 0; i < to_merge.size(); ++i )
  {
    const double weight = weights[i];

    const double* start_loc = &( to_merge[i]->get_features()[0] );
    const double* end_loc = start_loc + to_merge[i]->features_size();

    for( const double *copy_loc = start_loc; copy_loc != end_loc; ++pos, ++copy_loc )
    {
      *pos = weight * ( *copy_loc );
    }
  }
}


bool
descriptor_filter
::configure( const descriptor_filter_settings& settings )
{
  if( settings.to_duplicate.size() != settings.duplicate_ids.size() )
  {
    LOG_ERROR( "Descriptor duplicate ID vector sizes must match" );
    return false;
  }

  if( settings.to_merge.size() != settings.merge_weights.size() )
  {
    LOG_ERROR( "Descriptor merge ID and weight vector sizes must match" );
    return false;
  }

  any_filter_enabled_ = !settings.to_duplicate.empty() ||
                        !settings.to_merge.empty() ||
                        !settings.to_remove.empty();

  merge_filter_enabled_ = !settings.to_merge.empty();

  for( unsigned i = 0; i < settings.to_duplicate.size(); ++i )
  {
    dup_map_[ settings.to_duplicate[ i ] ].push_back( settings.duplicate_ids[ i ] );
  }

  for( unsigned i = 0; i < settings.to_remove.size(); ++i )
  {
    rem_set_.insert( settings.to_remove[ i ] );
  }

  settings_ = settings;
  return true;
}


void
descriptor_filter
::filter( descriptor_vec_t& descriptors )
{
  if( !any_filter_enabled_ )
  {
    return;
  }

  std::map< track::track_id_t, descriptor_vec_t > desc_map;
  descriptor_vec_t output_list;

  for( unsigned i = 0; i < descriptors.size(); ++i )
  {
    const descriptor_sptr_t& desc_sptr = descriptors[i];

    if( !desc_sptr )
    {
      continue;
    }

    const descriptor_t& desc = *desc_sptr;

    // Handle indexing for later merging operation
    if( merge_filter_enabled_ )
    {
      for( unsigned j = 0; j < desc.get_track_ids().size(); ++j )
      {
        desc_map[ desc.get_track_ids()[j] ].push_back( desc_sptr );
      }
    }

    // Handle removing by not copying entries that are in the remove set
    if( rem_set_.find( desc.get_type() ) == rem_set_.end() )
    {
      output_list.push_back( desc_sptr );
    }

    std::map< descriptor_id_t, std::vector< descriptor_id_t > >::iterator
      dup_itr = dup_map_.find( desc.get_type() );

    // Handle duplication of descriptors with new types
    if( dup_itr != dup_map_.end() )
    {
      for( unsigned j = 0; j < dup_itr->second.size(); ++j )
      {
        descriptor_sptr_t new_desc = raw_descriptor::create( desc_sptr );
        new_desc->set_type( dup_itr->second[j] );
        output_list.push_back( new_desc );
      }
    }
  }

  // Handle descriptor merging
  if( merge_filter_enabled_ )
  {
    for( std::map< track::track_id_t, descriptor_vec_t >::iterator p = desc_map.begin();
         p != desc_map.end(); p++ )
    {
      const unsigned req_merge_size = settings_.to_merge.size();

      if( p->second.size() < req_merge_size )
      {
        continue;
      }

      std::vector< descriptor_sptr_t > descs_in_order;

      for( unsigned i = 0; i < req_merge_size; ++i )
      {
        bool found = false;

        for( unsigned j = 0; j < p->second.size(); ++j )
        {
          if( p->second[j]->get_type() == settings_.to_merge[i] )
          {
            descs_in_order.push_back( p->second[j] );
            found = true;
            break;
          }
        }

        if( !found )
        {
          break;
        }
      }

      if( descs_in_order.size() == req_merge_size )
      {
        descriptor_sptr_t new_desc = raw_descriptor::create( settings_.merged_id );

        merge_descriptors( descs_in_order, settings_.merge_weights, *new_desc );

        new_desc->add_track_ids( descs_in_order[0]->get_track_ids() );
        new_desc->set_history( descs_in_order[0]->get_history() );

        if( settings_.normalize_merge )
        {
          normalize_descriptor( new_desc );
        }

        output_list.push_back( new_desc );
      }
    }
  }

  // Replace original list with reformated one
  descriptors = output_list;
}

} // end namespace vidtk
