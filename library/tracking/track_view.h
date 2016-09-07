/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _TRACK_VIEW_H_
#define _TRACK_VIEW_H_


#include <tracking/track.h>
#include <utilities/timestamp.h>
#include <vbl/vbl_smart_ptr.hxx>


namespace vidtk
{

// ----------------------------------------------------------------
/** View of a track.
 *
 * This class represents a customized read only view of a track
 * history.  Multiple track_view objects can have different views of
 * the same concrete track.
 *
 * Since this object is managed with a smart pointer, a factory method
 * has been provided as the only way to create a new object of this
 * type.
 */
class track_view
  : public vbl_ref_count
{
public:
  // -- TYPES --
  typedef vbl_smart_ptr< track_view > sptr;
  typedef vcl_vector < sptr > vector_t;
  typedef vcl_vector< track_state_sptr >::iterator iterator_t;
  typedef vcl_vector< track_state_sptr >::const_iterator const_iterator_t;

  // -- CONSTRUCTOR --
  static sptr create (track_sptr& track);
  virtual ~track_view();

  // -- ACCESSORS --
  const track_sptr get_track() const;
  const_iterator_t begin() const;
  const_iterator_t end() const;
  operator track_sptr () const;


  // track interface
  track_state_sptr const& last_state() const;
  track_state_sptr const& first_state() const;
  vcl_vector< track_state_sptr > const history() const;
  unsigned id() const;


  // -- MANIPULATORS --
  void set_view_start (const vidtk::timestamp& f);
  void set_view_end (const vidtk::timestamp& f);
  void set_view_bounds (const vidtk::timestamp& s, const vidtk::timestamp& e);


protected:


private:
  explicit track_view(track_sptr& track); // CTOR

  const track_state_sptr& find_state (const vidtk::timestamp & ts) const;
  bool is_valid_track_ts (const vidtk::timestamp& ts);
  bool is_valid_view_ts (const vidtk::timestamp& ts);

  /// The timestamp of the first history element in the view.  This
  /// timestamp should not be before the oldest history element.
  vidtk::timestamp m_firstHistory;

  /// The timestamp of the last valid history in the track.  This time
  /// should not exceed the most recent history element.
  vidtk::timestamp m_lastHistory;

  /// Pointer to the track to view.
  vidtk::track_sptr m_track;
}; // end class track_view

}

#endif /* _TRACK_VIEW_H_ */

// Local Variables:
// mode: c++
// fill-column: 70
// c-tab-width: 2
// c-basic-offset: 2
// c-basic-indent: 2
// c-indent-tabs-mode: nil
// end:


/*
  Tests: TBD
  1) create a track with no history and view it - fails, must have history
  2) create a track with 1 history and view it - success
  3) create a track with 10 history and view it - success

  4) change view of (2) to larger than one state - fails
  5) change view of (3) to first only, first - mid, last only, mid-last,
  6) change view of (3) to last-first (reversed TS)

  7) test public methods get_track(), begin(), end(), last_state(),
  first_state(), assignment, history(), id()

 */
