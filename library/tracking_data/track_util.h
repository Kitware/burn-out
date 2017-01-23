/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTKTRACK_UTIL_H_
#define _VIDTKTRACK_UTIL_H_

#include <tracking_data/track_sptr.h>
#include <tracking_data/track_state.h>

#include <vgl/vgl_box_2d.h>
#include <utilities/geo_coordinate.h>



namespace vidtk {

  class timestamp;


/** \brief Does track have state for timestamp.
 *
 * This function checks the history in the track to see if there is a
 * state for the specified timestamp. It is not sufficient for the
 * requested timestamp to be within the span of the history. There
 * must be an actual state for the requested timestamp.
 *
 * @param track Track to search
 * @param ts Timestamp to locate
 *
 * @return \c true if there is a track state for the timestamp; \c
 * false otherwise.
 */
bool has_timestamp( vidtk::track_sptr track, vidtk::timestamp const& ts );


/** \brief Reverse the order of track history.
 *
 * This merely reverse the order detections/states in the history
 * buffer.  Other track fields outside of the history buffer
 * (e.g. tracking_keys::most_recent_clip ) should be handled
 * externally based on the application's needs.
 *
 * The history is reversed in place, so beware.
 *
 * @param track Track to have history reversed.
 */
void reverse_history( vidtk::track_sptr track );


/** \brief Erases history after the specified time.
 *
 * Remove the last trailing bit of track history, leaving \c ts as
 * the last state.  If the state for \c ts is in a gap in the
 * history, truncation starts at the next state as usual.
 *
 * @param track Track to have history truncated.
 * @param ts last state to remain in history.
 *
 * @return \c false if operation fails.
 */
bool truncate_history( vidtk::track_sptr const track, timestamp const& ts);


/** \brief Get the spatial bounds of track.
 *
 * Get bounding box for all of track history.
 *
 * @param track Track to inspect
 * @param type Coordinate type code
 *
 * @return Bounding box for track hsitory.
 */
vgl_box_2d< double > get_spatial_bounds(
  vidtk::track_sptr const track,
  track_state::location_type type = track_state::stabilized_plane );

/** \brief Get the spatial bounds of track.
 *
 * Get bounding box for subset of track history.
 *
 * @param track Track to inspect
 * @param begin Starting state (inclusive)
 * @param end Ending state (inclusive)
 * @param type Coordinate type code
 *
 * @return Bounding box for selected track history.
 */
vgl_box_2d< double > get_spatial_bounds(
  vidtk::track_sptr track,
  timestamp const& begin,
  timestamp const& end,
  track_state::location_type type = track_state::stabilized_plane );

geo_coord::geo_bounds
get_geo_bounds( vidtk::track_sptr const track);

geo_coord::geo_bounds
get_geo_bounds( vidtk::track_sptr const track,
                timestamp const& begin,
                timestamp const& end );

} // end namespace

#endif /* _VIDTKTRACK_UTIL_H_ */
