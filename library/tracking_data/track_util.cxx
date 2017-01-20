/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_util.h"
#include "track.h"

#include <utilities/timestamp.h>

#include <algorithm>

namespace {

  // private local functions

  bool state_comp_less( vidtk::track_state_sptr const& a, vidtk::track_state_sptr const& b )
  {
    return (a->get_timestamp() < b->get_timestamp());
  }

}

namespace vidtk {


// ----------------------------------------------------------------
bool has_timestamp(vidtk::track_sptr track, vidtk::timestamp const& ts)
{
  // since the states are in ascending timestamp order, we can use a
  // binary search
  vidtk::track_state_sptr val_ptr( new vidtk::track_state);
  val_ptr->set_timestamp( ts );
  return std::binary_search( track->history().begin(), track->history().end(),
                             val_ptr, state_comp_less );
}


// ----------------------------------------------------------------
void reverse_history( vidtk::track_sptr track )
{
  track_state::vector_t new_hist( track->history() );

  std::reverse( new_hist.begin(), new_hist.end() );
  track->reset_history( new_hist );
}


// ----------------------------------------------------------------
  bool truncate_history(vidtk::track_sptr track, timestamp const& qts)
{
  track_state::vector_t new_hist( track->history() );

  track_state::vector_t::iterator it = std::find_if(
    new_hist.begin(), new_hist.end(), track_state_ts_pred( qts ) );

  if ( it == new_hist.end() )
  {
    return false;
  }

  // track_state_ts_pred could return the first timestamp > qts
  // in case of a gap in the history
  if ( (*it)->time_ == qts )
  {
    // adjust iterator to point past the end timestamp
    ++it;
  }

  new_hist.erase( it, new_hist.end() );
  track->reset_history( new_hist );

  return true;
}


// ----------------------------------------------------------------
vgl_box_2d< double >
get_spatial_bounds(
  vidtk::track_sptr const track,
  track_state::location_type loc )
{
  return get_spatial_bounds( track, track->first_state()->time_,
                                    track->last_state()->time_,
                                    loc );
}

geo_coord::geo_bounds
get_geo_bounds( vidtk::track_sptr const track)
{
  return get_geo_bounds(
    track,
    track->first_state()->time_,
    track->last_state()->time_);
}

// ----------------------------------------------------------------
vgl_box_2d< double >
get_spatial_bounds(
  vidtk::track_sptr const     track,
  timestamp const&            begin,
  timestamp const&            end,
  track_state::location_type  loc )
{
  vgl_box_2d< double > result;
  track_state::vector_t::const_iterator iter = track->history().begin();

  // Skip to the begin marker
  for ( /* nothing */ ; iter != track->history().end() && ( *iter )->time_ < begin; ++iter )
  {
    /* nothing */
  }

  // If we have reached the end of history, then return empty region
  if ( iter == track->history().end() )
  {
    return result;
  }

  for ( /* nothing */ ; iter != track->history().end() && ( *iter )->time_ <= end; ++iter )
  {
    vnl_vector_fixed< double, 3 > l;
    ( *iter )->get_location( loc, l );
    result.add( vgl_point_2d< double > ( l[0], l[1] ) );
  }

  return result;
}

// ----------------------------------------------------------------
geo_coord::geo_bounds
get_geo_bounds(
  vidtk::track_sptr const track,
  timestamp const& begin,
  timestamp const& end )
{
  geo_coord::geo_bounds result;
  track_state::vector_t::const_iterator iter = track->history().begin();

  // Skip to the begin marker
  for ( /* nothing */ ; iter != track->history().end() && ( *iter )->time_ < begin; ++iter )
  {
    /* nothing */
  }

  // If we have reached the end of history, then return empty region
  if ( iter == track->history().end() )
  {
    return result;
  }

  for ( /* nothing */ ; iter != track->history().end() && ( *iter )->time_ <= end; ++iter )
  {
    geo_coord::geo_coordinate geo = ( *iter )->get_geo_location();
    result.add( geo );
  }

  return result;
}

} // end namespace
