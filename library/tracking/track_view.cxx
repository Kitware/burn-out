/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <tracking/track_view.h>
#include <utilities/log.h>

#include <vcl_algorithm.h>

namespace vidtk
{


// ----------------------------------------------------------------
/** Factory method.
 *
 * This method creates a new object and returns a smart pointer
 * referring to the object.
 *
 * By default, the whole track history is in the view.  You will need
 * to set the starting and ending bounds to view a subset.
 *
 * @param[in] track - track object to be viewed.
 */
  track_view::sptr track_view::
  create(vidtk::track_sptr& ptrack)
{
  return ( sptr( new track_view(ptrack) ) );
}


// ----------------------------------------------------------------
/** Constructor.
 *
 * This method initializes a new track_view object.  By default, the
 * whole track history is in the view.  You will need to set the
 * starting and ending bounds to view a subset.
 *
 * @throw vcl_runtime_error
 */
track_view::
  track_view(vidtk::track_sptr& ptrack) :
  m_track(ptrack)
{
  // currently can not handle a track with no history
  if (this->get_track()->history().size() == 0)
  {
    throw vcl_runtime_error("track has empty history");
  }

  // fill in the bounds as the whole track
  m_firstHistory = this->get_track()->first_state()->time_;
  m_lastHistory = this->get_track()->last_state()->time_;
}


track_view::
  ~track_view()
{
}


// ----------------------------------------------------------------
/** Get underlying track.
 *
 * This method returns a pointer to the whole track object that is the
 * backing for this view.
 */
const track_sptr track_view::
  get_track() const
{
  return ( m_track );
}


// ----------------------------------------------------------------
/** Get last state.
 *
 * This method returns a pointer to the last (newest) state in our
 * view of track history.
 */
track_state_sptr const& track_view::
  last_state() const
{
  return ( *( this->end() ) );
}


// ----------------------------------------------------------------
/** Get first state.
 *
 * This method returns a pointer to the first (oldest) state in our
 * view of track history.
 */
track_state_sptr const& track_view::
  first_state() const
{
  return ( *( this->begin() ) );
}


// ----------------------------------------------------------------
/** Get start iterator.
 *
 * This method returns the iterator to the first track state history
 * in the view.  An exception is thrown if the view start timestamp
 * can not be found.
 *
 * @throw vcl_runtime_error
 */
  track_view::const_iterator_t track_view::
  begin() const
{
  const_iterator_t it = vcl_find_if( this->get_track()->history().begin(),
                                     this->get_track()->history().end(),
                                     track_state_ts_pred(m_firstHistory) );

  if (this->get_track()->history().end() == it)
  {
    // start timestamp not found
    throw vcl_runtime_error("start timestamp not found");
  }

  return ( it );
}


// ----------------------------------------------------------------
/** Get ending iterator.
 *
 * This method returns the iterator to the track state history element
 * after the last one in the view (just like the STL end()).  An
 * exception is thrown if the view end timestamp can not be found.
 *
 * @throw vcl_runtime_error
 */
track_view::const_iterator_t track_view::
  end() const
{
  const_iterator_t it = vcl_find_if( this->get_track()->history().begin(),
                       this->get_track()->history().end(),
                       track_state_ts_pred(m_lastHistory) );

  if (this->get_track()->history().end() == it)
  {
    // end timestamp not found
    throw vcl_runtime_error("end timestamp not found");
  }

  // adjust iterator to point past the end timestamp.
  return ( ++it );
}


// ----------------------------------------------------------------
/** Convert to track sptr.
 *
 * Convert this view of a track into a new track object.  The returned
 * track is a copy of the original track with a new updated
 * history vector. Since this is a special purpose copy of a track, the
 * histogram and amhi data are initialized to the default state.
 *
 * @return A new track object managed by a track_sptr is returned.
 */
track_view::
operator track_sptr () const
{
  track * ltrack ( new track ( *this->get_track()) );

  ltrack->set_amhi_datum (amhi_data());  // reset to default state
  ltrack->set_histogram (image_histogram <vxl_byte, float> ());

  // replace history with only what is viewed.
  ltrack->reset_history( this->history() );

  return ( ltrack );
}


// ----------------------------------------------------------------
/** Return track history of view.
 *
 * This method returns a truncated track hsitory as defined by this
 * view.  The track state vector is a shallow copy of the original
 * track state history.  This is slightly different from how track::
 * behaves because we do not already have a correctly sized history
 * vector.
 *
 * @throw vcl_runtime_error
 */
vcl_vector< track_state_sptr > const track_view::
  history() const
{
  vcl_vector< track_state_sptr > view( this->begin(), this->end() );

  return ( view );
}


// ----------------------------------------------------------------
/** Get track ID.
 *
 * This method returns the track-ID of the track being viewed.
 */
unsigned int track_view::
  id() const
{
  return ( m_track->id() );
}


// ----------------------------------------------------------------
/** Is timestamp valid.
 *
 * This method checks the specified timestamp to see if it is in the
 * backing track history.  Note that this is different than asking if
 * the timestamp is valid in the view.
 *
 * @sa is_valid_view_ts
 *
 * @param[in] ts - timestamp to validate.
 */
bool track_view::
is_valid_track_ts (const vidtk::timestamp& ts)
{
  bool result = (m_track->first_state()->time_ <= ts)
    && (ts <= m_track->last_state()->time_);

  return (result);
}


// ----------------------------------------------------------------
/** Is timestamp valid.
 *
 * This method checks the specified timestamp to see if it is in the
 * current track view.  Note that this is different than asking if
 * the timestamp is valid in the backing store.
 *
 * @sa is_valid_track_ts
 *
 * @param[in] ts - timestamp to validate.
 */
bool track_view::
is_valid_view_ts (const vidtk::timestamp& ts)
{
  bool result = (this->first_state()->time_ <= ts)
    && (ts <= this->last_state()->time_);

  return (result);
}


// ----------------------------------------------------------------
/** Set starting frame.
 *
 * This method sets the starting frame for this view. This first frame
 * is included in the view.
 *
 * @param[in] f - starting frame number (earliest timestamp)
 *
 * @throw vcl_runtime_error
 */
void track_view::
set_view_start(const vidtk::timestamp& f)
{
  if (!is_valid_track_ts(f))
  {
    throw vcl_runtime_error("timestamp not found");
  }

  m_firstHistory = find_state(f)->time_;
}


// ----------------------------------------------------------------
/** Set ending frame.
 *
 * This method sets the ending frame for this view. This last frame
 * is included in the view.
 *
 * @param[in] f - ending frame number (latest timestamp)
 *
 * @throw vcl_runtime_error
 */
void track_view::
  set_view_end(const vidtk::timestamp& f)
{
  if ( !is_valid_track_ts(f) )
  {
    throw vcl_runtime_error("timestamp not found");
  }

  m_lastHistory = find_state(f)->time_;
}


// ----------------------------------------------------------------
/** Set view bounds
 *
 * This method sets the bounds (first and last frame) of the view.
 * The view is inclusive of these bounds. The starting timestamp is
 * the earliest timestamp in the view. The ending timestamp is the
 * latest timestamp in the view.
 *
 * @param[in] s - starting frame number
 * @param[in] e - ending frame number
 */
void track_view::
  set_view_bounds(const vidtk::timestamp& s, const vidtk::timestamp& e)
{
  set_view_start(s);
  set_view_end(e);
}


// ----------------------------------------------------------------
/** Find state
 *
 * Scan the track history to find the entry that corresponds to the
 * requested timestamp. Sometimes there is a hole in the history and
 * we have to settle for the first history element that does not
 * exceed the requested time stamp.
 *
 * The track state that is closest to the target timestamp without
 * exceeding it. With the exception of a target timestamp that is less
 * than the timestamp on the first state. In this case, the first
 * state is returned (which might not be what you want).
 *
 * It is good practive to call is_track_valid_ts() to make sure the
 * target timestamp is within the bounds of this track.
 *
 * @param f - timestamp to locate
 *
 * @sa is_track_valid_ts()
 */
const track_state_sptr& track_view::
find_state (const vidtk::timestamp& f) const
{
  const_iterator_t it;
  for (it =  this->get_track()->history().begin();
       it != this->get_track()->history().end();
       it++)
  {
    if (f == (*it)->time_)
    {
      return (*it);
    }
    else if (f < (*it)->time_)
    {
      // then we have gone past target TS
      // Need to return previous state

      // If we are still at the beginning, return first state
      if (it == this->get_track()->history().begin())
      {
        log_error ("Timestamp was before the beginning - returning begin()\n");
        return (*it);
      }

      // use timestamp from previous state
      it--;
      return (*it);
    }
  } // end for

  // Since we fell out of the loop, the last state has a ts < f
  // return last state
  it--;
  return (*it);
}


} // end namespace

// Local Variables:
// mode: c++
// fill-column: 70
// c-tab-width: 2
// c-basic-offset: 2
// c-basic-indent: 2
// c-indent-tabs-mode: nil
// end:
