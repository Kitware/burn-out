/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _TRACK_VIEW_H_
#define _TRACK_VIEW_H_


#include <tracking_data/track.h>
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
  typedef std::vector < sptr > vector_t;
  // Iterators provide access to track states
  typedef std::vector< track_state_sptr >::iterator iterator_t;
  typedef std::vector< track_state_sptr >::const_iterator const_iterator_t;
  typedef std::vector< track_state_sptr >::reverse_iterator reverse_iterator_t;
  typedef std::vector< track_state_sptr >::const_reverse_iterator const_reverse_iterator_t;

  // -- CONSTRUCTOR --
  static sptr create (const track_sptr track); // factory method
  virtual ~track_view();

  // -- ACCESSORS --
  const track_sptr get_track() const;
  ///@{
  /** Get starting iterator for list of states. These iterators
   * behave like standard iterator.
   */
  const_iterator_t begin() const;
  const_reverse_iterator_t rbegin() const;
  //@}

  //@{
  /** Get ending iterator for list of states. These iterators
   * behave like standard iterator.
   */
  const_iterator_t end() const;
  const_reverse_iterator_t rend() const;
  ///@}

  operator track_sptr () const;

  size_t frame_span() const;

  // track interface
  track_state_sptr const& last_state() const;
  track_state_sptr const& first_state() const;
  std::vector< track_state_sptr > const history() const;
  unsigned id() const;


  // -- MANIPULATORS --
  void set_view_start (const vidtk::timestamp& f);
  void set_view_end (const vidtk::timestamp& f);
  void set_view_bounds (const vidtk::timestamp& s, const vidtk::timestamp& e);

private:
  explicit track_view(const track_sptr track); // CTOR

  const track_state_sptr find_state (const vidtk::timestamp & ts) const;
  bool is_valid_track_ts (const vidtk::timestamp& ts);
  bool is_valid_view_ts (const vidtk::timestamp& ts);

  /// The timestamp of the first history element in the view.  This
  /// timestamp should not be before the oldest history element.
  vidtk::timestamp m_firstHistory;

  /// The timestamp of the last valid history in the track.  This time
  /// should not exceed the most recent history element.
  vidtk::timestamp m_lastHistory;

  /// Pointer to the track to view.
  const vidtk::track_sptr m_track;
}; // end class track_view

}

#endif /* _TRACK_VIEW_H_ */
