/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _TRACK_DEMULTIPLEXER_H_
#define _TRACK_DEMULTIPLEXER_H_

#include <map>
#include <tracking_data/track.h>
#include <utilities/timestamp.h>

namespace vidtk {

// ----------------------------------------------------------------
/** Track demultiplexer class.
 *
 * This class demultiplexes a stream of active tracks into new tracks,
 * active tracks, and terminated tracks.
 *
 * When a new set of active tracks are added, they are sorted into
 * categories as follows:
 *
 * NEW TRACKS - those tracks that are not currently in the list of
 * active tracks (map).
 *
 * NOT_UPDATED - those tracks in the input that are missing the
 * current state.
 *
 * UPDATED - active tracks that have been updated
 *
 * The union of UPDATED and NOT_UPDATED is the set of active tracks.
 *
 * TERMINATED TRACKS - tracks that have not been updated with in the
 * last few frames (specified by track termination window).
 *
 * The timestamp applied with the add_active_tracks() method provides
 * the timebase for terminating tracks.
 *
 * This class is multi-tracker aware.
 */
class track_demultiplexer
{
public:
  // -- TYPES --

  // --- key portion of the track map ---
  struct track_map_key_t
  {
    track_map_key_t() : track_id( 0 ), tracker_type( 0 ) { }
    track_map_key_t( unsigned id, unsigned t)
      : track_id( id ), tracker_type( t ) { }
    track_map_key_t( vidtk::track_sptr trk )
    : track_id( trk->id() ), tracker_type(trk->get_tracker_type() ) { }

    unsigned track_id;
    unsigned tracker_type;
  };

  struct track_map_key_compare
  {
    bool operator() ( track_map_key_t const& a, track_map_key_t const& b ) const
    {
      // compare for less than a < b
      if ( a.track_id < b.track_id ) return true;
      if ( (a.track_id == b.track_id) && (a.tracker_type < b.tracker_type) ) return true;
      return false;
    }
  };



  // -- CONSTRUCTOR --
  track_demultiplexer();
  virtual ~track_demultiplexer();

  /** Add new set of active tracks.  The set of tracks supplied are
   * considered to be the active tracks for the current time (as
   * specified by the associated time stamp).
   *
   * @param tracks List of active tracks.
   * @param ts Current time of active tracks.
   */
  void add_tracks_for_frame( vidtk::track::vector_t const& tracks,
                             vidtk::timestamp const& ts );

  /** Add new set of active tracks without the use of a timestamp.
   * This option will populate the new tracks, active tracks, and
   * terminated tracks fields, but not the updated and not updated
   * tracks.
   *
   * @param tracks List of active tracks.
   */
  void add_tracks( vidtk::track::vector_t const& tracks );


  ///@{
  /** Get output from demultiplexer.
   */
  vidtk::track::vector_t get_new_tracks() const;
  vidtk::track::vector_t get_updated_tracks() const;
  vidtk::track::vector_t get_not_updated_tracks() const;
  vidtk::track::vector_t get_terminated_tracks() const;
  vidtk::track::vector_t get_active_tracks() const;
  ///@}

  /** Flush all active tracks to terminated port.
   */
  bool flush_tracks();
  void reset_outputs();

protected:
  typedef std::map < track_map_key_t, vidtk::track_sptr, track_map_key_compare  >track_map_t;
  typedef track_map_t::value_type map_value_t;
  typedef track_map_t::iterator map_iterator_t;
  typedef track_map_t::const_iterator map_const_iterator_t;


private:

  track_map_t  active_track_map_;

  vidtk::timestamp last_timestamp_;

  vidtk::track::vector_t new_tracks_;
  vidtk::track::vector_t terminated_tracks_;
  vidtk::track::vector_t updated_tracks_;
  vidtk::track::vector_t not_updated_tracks_;
  vidtk::track::vector_t active_tracks_;

}; // end class track_demultiplexer

} // end namespace

#endif /* _TRACK_DEMULTIPLEXER_H_ */
