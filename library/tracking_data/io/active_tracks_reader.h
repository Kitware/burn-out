/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _ACTIVE_TRACKS_READER_H_
#define _ACTIVE_TRACKS_READER_H_

#include <tracking_data/io/track_reader.h>
#include <tracking_data/track.h>
#include <tracking_data/track_view.h>

namespace vidtk
{

class active_tracks_generator;

// ----------------------------------------------------------------
/** \brief Active track reader decorator.
 *
 * This class decorates a track reader object and returns the tracks
 * as active tracks based on the supplied frame number.
 *
 * This class owns the object being decorated. It will be deleted when
 * this decorator is deleted.
 *
 * The active_tracks_generator class is used to implement the track
 * generation functionality.
 *
 * \sa active_tracks_generator
 */
class active_tracks_reader
{
public:
  active_tracks_reader(track_reader * r);
  virtual ~active_tracks_reader();

  /** \brief Read active tracks for current frame.
   *
   * A list of active tracks for the specified frame is returned.
   *
   * @param[in] frame - Request active tracks on this frame.
   * @param[out] trks - List of tracks that are active in the
   * specified frame. List may be empty.
   *
   * @retval true Active tracks are returned
   * @retval false No more active tracks, stop calling.
   */
  bool read_active(unsigned const frame, vidtk::track_vector_t & trks);

  /** \brief Read terminated tracks for current frame.
   *
   * The output set will contain the list of tracks that are
   * terminated on the specified frame. The last state in any of the
   * tracks returned will be (frame - 1).
   *
   * @param[in] frame - Return tracks terminated on this frame.
   * @param[out] trks - List of tracks terminated on specified frame.
   * @retval true - always (dont ask).
   */
  bool read_terminated(unsigned const frame, vidtk::track_vector_t & trks);

  /** \brief Extract the read in timestamp for the frame.
   *
   * \sa active_tracks_generator::timestamp_for_frame()
   */
  bool get_timestamp( unsigned const frame, timestamp & ts );

  //@{
  /** Return the first/last frame in the active tracks list. This is
   * handy for callers to determine the bounds of the active tracks.
   */
  unsigned first_frame();
  unsigned last_frame();
  //@}


private:
  /** Add tracks to current set of tracks.  The tracks read from the
   * file are added to the current list of full tracks.
   */
  bool load_tracks();


  /// Decorated reader
  track_reader * reader_;

  active_tracks_generator * generator_;

  bool tracks_loaded_;

}; // end class active_tracks_reader

} // end namespace

#endif /* _ACTIVE_TRACKS_READER_H_ */
