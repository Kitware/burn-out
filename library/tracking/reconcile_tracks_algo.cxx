/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "reconcile_tracks_algo.h"

#include <vgl/vgl_intersection.h>
#include <logger/logger.h>


namespace vidtk {
namespace reconciler {

VIDTK_LOGGER ("simple_track_reconciler");


static vcl_ostream& operator << (vcl_ostream& str, reconciler::track_map_key_t const& datum)
{
  str <<  "{" << datum.track_id
      << ", " << datum.tracker_type
      << ", " << datum.tracker_subtype
      << "}";

  return str;
}


static vcl_ostream& operator << (vcl_ostream& str, reconciler::track_map_data_t const& datum)
{
    str << " ["
        << (datum.is_dropped() ? "D" : " ")
        << (datum.is_processed() ? "P" : " ")
        << (datum.is_generated() ? "G" : " ")
        << (datum.is_terminated() ? "T" : " ")
        << "] ";

  return ( str );
}


// ----------------------------------------------------------------
/** Process tracks looking for tracks to suppress.
 *
 * This method compares all tracks against all tracks that are
 * originated by another tracker.
 *
 * @param[in] tts - current time for comaparison
 * @param[in,out] tracks - map of tracks
 * @param[out] terminated_tracks - list of terminated tracks
 */
void simple_track_reconciler::
process_track_map (vidtk::timestamp const& tts, track_map_t& tracks,
                   vidtk::track_vector_t* terminated_tracks)
{

  terminated_tracks->clear();

  // Nothing to do if there are no tracks here.
  if (tracks.size() <= 0)
  {
    return;
  }

  // Reset all processed flags
  for (map_iterator_t ix = tracks.begin(); ix != tracks.end(); ix++)
  {
    // make processed same as dropped state
    ix->second.set_processed ( ix->second.is_dropped() );
  }

  map_iterator_t base_ix = tracks.begin();

  // This might not be the optimal solution,
  // but it will do O(((n+1) * n)/2)  compares worst case
  while (1)
  {
    // Find the first track that is not dropped or processed - use
    // this as base track
    map_iterator_t ix1;
    for (ix1 = base_ix; ix1 != tracks.end(); ix1++)
    {
      if ( ! ix1->second.is_processed() )
      {
        base_ix = ix1;
        break; // exit for loop
      }
    } // end for

    // Test to see if there are any more to process
    if (ix1 == tracks.end())
    {
      break; // exit while loop
    }

    // If base track does not have enough states, or the last state
    // is less than target ts, skip to find another base track.
    // -- later -- move up into for loop above
    if ( (base_ix->second.track()->history().size() < params.min_track_size )
         || ( base_ix->second.track()->last_state()->time_ != tts) )
    {
      base_ix->second.set_processed();
      continue; // get a new base track
    }

    unsigned base_tracker = base_ix->first.tracker_type;
    unsigned base_subtracker = base_ix->first.tracker_subtype;

    for (map_iterator_t ix = base_ix; ix != tracks.end(); ix++)
    {
      // skip this entry if does not pass
      if ( ix->second.is_processed()
           || ( (ix->first.tracker_type == base_tracker)
                && (ix->first.tracker_subtype == base_subtracker)) )
      {
        continue;
      }

      // If the last state is not the same as the target time stamp,
      // skip this track. This may happen if there are gaps in the
      // track, or there are no new states for a track.  In the latter
      // case, the track will be terminated during track generation.
      // If target track does not have enough states, go to next track
      if ( ( ix->second.track()->history().size() < params.min_track_size )
           || ( ix->second.track()->last_state()->time_ != tts) )
      {
        ix->second.set_processed();
        continue;
      }

      // At this point we have another track that is of a different
      // tracker and needs to be processed.
      int result = compare_tracks ( tts, base_ix->second.track(), ix->second.track() );
      if ( (result == A_OVER_B) || (result == A_SAME_AS_B))
      {
        LOG_DEBUG ("algo: (A/B) dropping track " << ix->first << ix->second
                   << " at frame " << tts.frame_number()
                   << " against " << base_ix->first << base_ix->second);

        ix->second.set_dropped();

        // Only add to terminated tracks if it has been generated at
        // least once.
        if (ix->second.is_generated() )
        {
          terminated_tracks->push_back (ix->second.track());
        }
      }
      else if (result == B_OVER_A)
      {
        LOG_DEBUG ("algo: (B/A) dropping track " << base_ix->first << base_ix->second
                   << " at frame " << tts.frame_number()
                   << " against " << ix->first << ix->second);

        base_ix->second.set_dropped();

        // Only add to terminated tracks if it has been generated at
        // least once.
        if (base_ix->second.is_generated() )
        {
          terminated_tracks->push_back (base_ix->second.track());
        }
        break; // need to find another base
      }

    } // end for


    // set this track as processed since it has been compared against
    // all other tracks (or marked as dropped).
    base_ix->second.set_processed();

  } // end while

}


// ----------------------------------------------------------------
/** Compare two tracks.
 *
 * This method compares two tracks. The track length may be different,
 * but they are all end on the same frame.
 *
 * @param[in] tts Current time of comparison
 * @param[in] a One track to compare
 * @param[in] b Other track to compare
 *
 * @retval DISJOINT -
 */
unsigned simple_track_reconciler::
compare_tracks (vidtk::timestamp const& tts, track_sptr a, track_sptr b)
{
  bbox_vector_t a_box_vect;
  bbox_vector_t b_box_vect;
  int current_frame(0);

  // If both vectors do not have enough states, then return directly.
  if ( (a->history().size() < params.min_track_size)
       || (b->history().size() < params.min_track_size) )
  {
    return DISJOINT;
  }

  // iterate over track states in history
  size_t count = a->history().size();

  current_frame = a->history()[0]->time_.frame_number() - 1;

  for (size_t i = 0; i < count; i++)
  {
    int frame = a->history()[i]->time_.frame_number();
    if (frame == ++current_frame)
    {
      // get track state object
      a_box_vect.push_back( a->get_object( i )->bbox_ );
    }
    else if (frame > current_frame)
    {
      // Handle missing state
      --i; // back up and try again
      a_box_vect.push_back( bbox_t() ); // push default box
    }
    else
    {
      LOG_ERROR("Unexpected frame number");
      break;
    }
  } // end for

  // ----------------------------------------------------------------
  count = b->history().size();
  current_frame = b->history()[0]->time_.frame_number() - 1;

  for (size_t i = 0; i < count; i++)
  {
    int frame = b->history()[i]->time_.frame_number();
    if (frame == ++current_frame)
    {
      // get track state object
      b_box_vect.push_back( b->get_object( i )->bbox_ );
    }
    else if (frame > current_frame)
    {
      // Handle missing state
      --i;
      b_box_vect.push_back( bbox_t() ); // push default box
    }
    else
    {
      LOG_ERROR("Unexpected frame number");
      break;
    }

  } // end for

  unsigned result = compare_box_vector ( a_box_vect, b_box_vect);

  return result;
}


// ----------------------------------------------------------------
/** Compare two vectors of boxes.
 *
 * This method compares two vectors of boxes to determine how much
 * overlap there may be.  The boxes must be compared from identical
 * frames.
 *
 * @param[in] a_box_vect Vector of boxes from 'A' track
 * @param[in] b_box_vect Vector of boxes from 'B' track
 */
unsigned simple_track_reconciler::
compare_box_vector ( bbox_vector_t a_box_vect, bbox_vector_t b_box_vect)
{
  track2track_frame_overlap_record overlap;
  int total_count(0);
  int a_b_count(0);
  int b_a_count(0);

  // Compare two lists of boxes. The lists are compared backwards
  // because the last boxes are synchronized at the current time stamp
  // time.
  bbox_vector_t::const_reverse_iterator ix_a = a_box_vect.rbegin();
  bbox_vector_t::const_reverse_iterator ix_b = b_box_vect.rbegin();

  // Loop over all states, from back to front, comparing individual
  // boxes. Stop when either vector runs out of states. If one vector
  // is longer than the other, only compare where they both have
  // boxes.
  do
  {
    // Test to see if either b1 or b2 is invalid or a default box.
    // If so, skip this compare
    if ( (ix_a->is_empty()) || (ix_b->is_empty()) )
    {
      continue; // don't contribute to comparison
    }

    track2track_frame_overlap_record ret;
    ret = compare_box (*ix_a, *ix_b);

    // summarize results
    total_count++;

    // Test for (A n B) / A > overlap_fraction : There is more of A in
    // the intersection than B.  B, therefore, takes precedence.
    if ( (ret.overlap_area / ret.first_area) > params.overlap_fraction)
    {
      b_a_count++; // count b over a
    }

    // Test for (A n B) / B > overlap_fraction : There is more of B in
    // the intersection than A.  A, therefore, takes precedence.
    if ( (ret.overlap_area / ret.second_area) > params.overlap_fraction)
    {
      a_b_count++; // count a over b
    }

  }
  while ( (++ix_a != a_box_vect.rend()) && (++ix_b != b_box_vect.rend()) );

  // Look at the result of all state comparisons.  If enough of the
  // individual states are considered equivalent (as a fraction of
  // total number of states), then call the whole track
  if ( (a_b_count / total_count) > params.state_count_fraction)
  {
    return A_OVER_B;
  }

  if ( (b_a_count / total_count) > params.state_count_fraction)
  {
    return B_OVER_A;
  }


  return DISJOINT;
}


// ----------------------------------------------------------------
/** Compare two boxes.
 *
 * This method compares two boxes and returns information about how
 * much they overlap.  A default constructued object is returned if
 * they do not overlap.
 *
 * @param[in] b1 one box to comapre
 * @param[in] b2 other box to comapre
 *
 * @return A structure containing the summary of the intersection og
 * the two boxes, if any.
 */
track2track_frame_overlap_record
simple_track_reconciler::
compare_box ( bbox_t b1, bbox_t b2)
{
  track2track_frame_overlap_record ret;

  bbox_t bi = vgl_intersection( b1, b2 );

  if ( ! bi.is_empty() )
  {
    ret.first_area = b1.area();
    ret.second_area = b2.area();
    ret.overlap_area = bi.area();

    double dx = b1.centroid_x() - b2.centroid_x();
    double d_center_y = b1.centroid_y() - b2.centroid_y();
    double d_bottom_y = b1.max_y() - b2.max_y();

    ret.centroid_distance = vcl_sqrt( (dx*dx) + (d_center_y*d_center_y) );
    ret.center_bottom_distance = vcl_sqrt( (dx*dx) + (d_bottom_y*d_bottom_y) );
  }

  return ret;
}

} // end namespace
} // end namespace
