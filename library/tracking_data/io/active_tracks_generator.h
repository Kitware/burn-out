/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _ACTIVE_TRACKS_GENERATOR_H_
#define _ACTIVE_TRACKS_GENERATOR_H_

#include <tracking_data/track.h>
#include <tracking_data/track_view.h>
#include <string>
#include <map>


namespace vidtk
{

// ----------------------------------------------------------------
/** \brief Active tracks generator.
 *
 * This class takes a set of terminated tracks and generates a list of
 * active tracks at each frame in the input set.  The add_tracks()
 * method can be called to add tracks to the working set.
 */
class active_tracks_generator
{
public:
  active_tracks_generator();
  virtual ~active_tracks_generator();

  /** \brief Add tracks to working set.
   *
   * The specified tracks are added to the internal working
   * set. Multiple sets of tracks can be added before creating the
   * active tracks lists by calling create_active_tracks(). This
   * approach allows flexibility in how the input tracks are
   * collected.
   *
   * These tracks are shared with the caller through the smart pointer
   * in the vector, so beware of changing these tracks.
   *
   * @param[in] tracks - Vector of tracks to add to working set.
   * @throw std::runtime_error - if error encountered
   */
  void add_tracks( vidtk::track_vector_t const& tracks);


///@{
  /** \brief Create active tracks from full tracks.
   *
   * Active tracks are created for each frame in the input set. This
   * method is be called after all tracks have been added.
   *
   * @throw std::runtime_error - if error encountered
   */
  void create_active_tracks();
  void create_active_tracks_alt(); ///< faster but different version
///@}


  /** \brief Create terminated tracks from full tracks.
   *
   * A track is added to the terminated set of the framer \b after the
   * last frame in the track. his method should be called after all
   * tracks have been added.
   */
  void create_terminated_tracks();

  /** \brief Return active tracks for a frame.
   *
   * The active tracks for the specified frame are returned to the
   * caller. The active tracks returned belong to the caller and can
   * be modified as needed.  Note that there may be no active tracks
   * for any given frame.
   *
   * @param[in] frame - Frame number for active tracks.
   * @param[out] tracks - Vector of tracks for frame.
   *
   * @return \c true always
   *
   * @throw std::runtime_error - if error encountered
   */
  bool active_tracks_for_frame( unsigned frame, vidtk::track_vector_t & tracks);


  /** \brief Return tracks terminated on frame.
   *
   * The tracks that are terminated on the specified frame are
   * returned. It is possible to get an enpty vector if no tracks are
   * terminated on the frame.
   *
   * @param[in] frame - Frame number for terminated tracks.
   * @param[out] tracks - Vector of tracks for frame.
   *
   * @retval true - always (dont ask).
   */
  bool terminated_tracks_for_frame( unsigned frame, vidtk::track_vector_t & tracks);


  /** \brief Get timestamp for frame.
   *
   * Return the full timestamp for the specified frame. A full and
   * correct timestamp is frequently needed for a frame when building
   * active tracks input sets.  The problem comes in when there is a
   * missing state in a track and the timestamp for that frame can not
   * be easily determined.
   *
   * In the case where estimation is forced, the linear approximation of
   * the time is used.  If there is a gap in the timestamps, then the
   * desired time is estimated using the endpoints of the gap, unless
   * there is a problem with one or more of the endpoints.
   *
   * @param[in] frame - Frame for timestamp
   * @param[out] ts - The timestamp for the specified frame is returned.
   * @param[in] est - Force timestamp estimation.
   *
   * @retval true - actual timestamp
   * @retval false - estimated timestamp
   */
  bool timestamp_for_frame( unsigned frame, vidtk::timestamp & ts, bool est = false );

  //@{
  /** Return the first/last frame in the active tracks list. This is
   * handy for callers to determinethe bounds of the active tracks in
   * this generator.
   */
  unsigned first_frame() const;
  unsigned last_frame() const;
  //@}


private:
  vidtk::timestamp find_timestamp (unsigned frame, const vidtk::track_sptr ptr);

  void create_timestamp_map();
  /// storage for full (terminated) tracks
  vidtk::track_vector_t full_tracks_;

  /// storage for track views at each frame number
  std::map < unsigned, vidtk::track_view::vector_t > active_tracks_;

  /// storage for track views at each frame number
  std::map < unsigned, std::vector< vidtk::track_sptr > > terminated_tracks_;

  /// Map from frame number to timestamp
  std::map < unsigned, vidtk::timestamp > ts_map_;

  unsigned lowest_frame_;
  unsigned highest_frame_;

  double seconds_per_frame_;
  double base_timestamp_;
}; // end class active_tracks_generator

} // end namespace

#endif /* _ACTIVE_TRACKS_GENERATOR_H_ */
