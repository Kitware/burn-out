/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "online_track_type_classifier.h"

#include <algorithm>
#include <limits>

#include <boost/foreach.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "online_track_type_classifier_cxx" );

online_track_type_classifier
::online_track_type_classifier() :
  min_count_filter_(0),
  max_count_filter_(std::numeric_limits<unsigned>::max()),
  sort_descriptors_(false),
  average_gsd_(-1.0)
{
}

online_track_type_classifier
::~online_track_type_classifier()
{
}

bool
sorting_function( const descriptor_sptr& desc1, const descriptor_sptr& desc2 )
{
  return ( desc1->get_type().compare( desc2->get_type() ) < 0 );
}

void
online_track_type_classifier
::apply( track::vector_t& tracks, const raw_descriptor::vector_t& descriptors )
{
  // Assemble set of updated tracks, and descriptor->track mappings
  std::map< track::track_id_t, raw_descriptor::vector_t > descriptor_map;

  track_demuxer_.add_tracks( tracks );

  BOOST_FOREACH( descriptor_sptr_t descriptor, descriptors )
  {
    BOOST_FOREACH( track::track_id_t track_id, descriptor->get_track_ids() )
    {
      if( id_filter_.empty() ||
          std::find( id_filter_.begin(), id_filter_.end(), descriptor->get_type() ) != id_filter_.end() )
      {
        descriptor_map[ track_id ].push_back( descriptor );
      }
    }
  }

  // Call OTC update function for all tracks
  BOOST_FOREACH( track_sptr_t track, track_demuxer_.get_active_tracks() )
  {
    std::map< track::track_id_t, raw_descriptor::vector_t >::iterator track_descriptor_itr;

    track_descriptor_itr = descriptor_map.find( track->id() );

    if( track_descriptor_itr != descriptor_map.end() &&
        track_descriptor_itr->second.size() <= max_count_filter_ &&
        track_descriptor_itr->second.size() >= min_count_filter_ )
    {
      raw_descriptor::vector_t& track_desc = track_descriptor_itr->second;

      if( sort_descriptors_ )
      {
        std::sort( track_desc.begin(), track_desc.end(), sorting_function );
      }

      object_class_sptr_t new_score = this->classify( track, track_desc );

      if( new_score )
      {
        // Update track with new score
        prior_scores_[ track_descriptor_itr->first ] = new_score;
        set_track_object_class( track, new_score );
      }
      else
      {
        // Update track score using prior if exists
        use_last_score_for_track( track );
      }
    }
    else
    {
      // Update track score using prior if exists
      use_last_score_for_track( track );
    }
  }

  // Call terminated track functions for all terminated tracks and remove prior data
  BOOST_FOREACH( track_sptr_t track, track_demuxer_.get_terminated_tracks() )
  {
    this->terminate_track( track );
    prior_scores_.erase( track->id() );
  }
}

void
online_track_type_classifier
::use_last_score_for_track( track_sptr_t& track )
{
  std::map< track::track_id_t, object_class_sptr_t >::iterator prior_score_itr;

  prior_score_itr = prior_scores_.find( track->id() );

  if( prior_score_itr != prior_scores_.end() )
  {
    set_track_object_class( track, prior_score_itr->second );
  }
}

void
online_track_type_classifier
::set_track_object_class( track_sptr_t& track, const object_class_sptr_t& scores )
{
  track->set_pvo( *scores );
}

void
online_track_type_classifier
::set_ids_to_filter( const descriptor_id_list_t& ids )
{
  id_filter_ = ids;
}

void
online_track_type_classifier
::terminate_track( const track_sptr_t& /*track_object*/ )
{
}

void
online_track_type_classifier
::set_min_required_count( unsigned descriptor_count )
{
  min_count_filter_ = descriptor_count;
}

void
online_track_type_classifier
::set_max_required_count( unsigned descriptor_count )
{
  max_count_filter_ = descriptor_count;
}

void
online_track_type_classifier
::set_sort_flag( bool sort_descriptors )
{
  sort_descriptors_ = sort_descriptors;
}

void
online_track_type_classifier
::set_average_gsd( double gsd )
{
  average_gsd_ = gsd;
}

}
