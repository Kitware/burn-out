/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_reconcile_tracks_process_h_
#define vidtk_reconcile_tracks_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vcl_map.h>

#include <tracking/track.h>
#include <tracking/track_view.h>
#include <tracking/shot_break_flags.h>

#include <utilities/timestamp.h>
#include <utilities/config_block.h>
#include <utilities/ring_buffer.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>


namespace vidtk {
namespace reconciler {


  class reconcile_tracks_process_impl;

  // key portion of the track map
  struct track_map_key_t
  {
    track_map_key_t() : track_id(0), tracker_type(0), tracker_subtype(0) { }
    track_map_key_t( unsigned id, unsigned t, unsigned st) : track_id(id), tracker_type(t), tracker_subtype(st)
    {
    }

    unsigned track_id;
    unsigned tracker_type;
    unsigned tracker_subtype;
  };

  struct track_map_key_compare
  {
    bool operator() (track_map_key_t const& a, track_map_key_t const& b) const
    {
      // compare for less than a < b
      if (a.track_id < b.track_id) return true;
      if ( (a.track_id == b.track_id) && (a.tracker_type < b.tracker_type) ) return true;
      if ( (a.track_id == b.track_id) && (a.tracker_type == b.tracker_type) && (a.tracker_subtype< b.tracker_subtype) ) return true;
      return false;
    }
  };


  // Data portion of the track map
  struct track_map_data_t
  {
  track_map_data_t()
    : track_ptr(0),
      track_dropped(false),
      track_processed(false),
      track_generated(false),
      track_terminated(false)
    { }

    track_map_data_t(track_sptr t, vidtk::timestamp const& ts)
      : track_ptr(t),
        track_dropped(false),
        track_processed(false),
        track_generated(false),
        track_terminated(false),
        track_last_seen(ts)
    {
    }

    // -- ACCESSORS --
    track_sptr track() { return this->track_ptr; }
    vidtk::timestamp const& last_seen() const { return this->track_last_seen; }
    const track_sptr track() const { return this->track_ptr; }
    bool is_dropped() const { return this->track_dropped; }
    bool is_processed() const { return this->track_processed; }
    bool is_generated() const { return this->track_generated; }
    bool is_terminated() const { return this->track_terminated; }

    // -- MANIPULATORS --
    void set_dropped(bool v = true) { this->track_dropped = v; }
    void set_processed(bool v = true) { this->track_processed = v; }
    void set_generated(bool v = true) { this->track_generated = v; }
    void set_terminated(bool v = true) { this->track_terminated = v; }
    void set_track(track_sptr trk) { this->track_ptr = trk; }
    void set_last_seen( vidtk::timestamp const& ts) { this->track_last_seen = ts; }


  private:
    track_sptr track_ptr;
    bool track_dropped;
    bool track_processed;
    bool track_generated; // set if track generated at least once
    bool track_terminated; // set if published on terminated tracks port
    vidtk::timestamp track_last_seen;  // last time track seen on active tracks port
  };


  typedef vcl_map < track_map_key_t, track_map_data_t, track_map_key_compare > track_map_t;
  typedef vcl_pair < track_map_key_t, track_map_data_t > map_value_t;
  typedef track_map_t::iterator map_iterator_t;
  typedef track_map_t::const_iterator map_const_iterator_t;



// ----------------------------------------------------------------
/** output reconciler delegate class base class
 *
 * Methods of this class are called at specific times through the
 * lifecycle of the list of tracks.
 *
 * This is the base class for all delegete processing. It also serves
 * as the default delegate if no other is specified.
 */
class reconciler_delegate
{
public:
  reconciler_delegate() { }
  virtual ~reconciler_delegate() { }


  /** Process finalized tracks.
   *
   * This method is called after the list of finalized tracks is
   * assembled for a frame time.
   *
   * @param[in] ts Frame time for vector of tracks
   * @param[in,out] tracks Vector of finalized tracks.
   */
  virtual void process_finalized_tracks(vidtk::timestamp const& ts, vidtk::track_vector_t& tracks) { }


  /** Process updated track map.
   *
   * This method is called after all new tracks have been added to the
   * track map.
   *
   * @param[in] ts Frame time for last update of map.
   * @param[in,out] tracks Map of currently active tracks.
   */
  virtual void process_track_map (vidtk::timestamp const& ts, track_map_t& tracks,
                                  vidtk::track_vector_t* terminated_tracks) { }

};

} // end namespace


// ----------------------------------------------------------------
/** Reconcile track.
 *
 * This class represents a process that reconciled output tracks.
 *
 * Reconciling takes tracks from a set of different trackers and
 * determines if there are any duplicate tracks.
 */
class reconcile_tracks_process
  : public process
{
public:
  typedef reconcile_tracks_process self_type;


  reconcile_tracks_process(vcl_string const &);
  ~reconcile_tracks_process();

  // Process interface
  virtual config_block params() const;
  virtual bool set_params(config_block const& blk);
  virtual bool initialize();
  virtual bool reset();
  virtual vidtk::process::step_status step2();

// This is the list of the data items that have to be synchronized
// over input and output.  Just add another line below to add another
// data value to be synchronized.

// (type, name, initial value)
#define RECONCILER_DATUM_LIST                                           \
  DATUM_DEF ( vidtk::timestamp,        timestamp,    vidtk::timestamp() ) \
  DATUM_DEF ( vidtk::track_vector_t,   tracks,       vidtk::track_vector_t() )


#define PROCESS_INPUT(T,N) void input_ ## N (T); VIDTK_OPTIONAL_INPUT_PORT (input_ ## N, T)
#define PROCESS_OUTPUT(T,N) T output_ ## N() const; VIDTK_OUTPUT_PORT (T, output_ ## N)
#define PROCESS_PASS_THRU(T,N) PROCESS_INPUT(T,N); PROCESS_OUTPUT(T,N)

  // The pass through approach is for the input processes to store
  // data into a data frame and the output processes to return data
  // from a different data frame. They do not use a shared frame.

  // ---- Process pass through ----
#define DATUM_DEF(T, N, I) PROCESS_PASS_THRU(T const& ,N);
  RECONCILER_DATUM_LIST // macro magic
#undef DATUM_DEF

  // -- output ports --
  PROCESS_OUTPUT ( vidtk::track_vector_t const&, terminated_tracks );

#undef PROCESS_INPUT
#undef PROCESS_OUTPUT
#undef PROCESS_PASS_THRU

  void set_reconciler_delegate (reconciler::reconciler_delegate * del);

private:
  reconciler::reconcile_tracks_process_impl * m_impl;

}; // class reconcile_tracks_process

} // end namespace vidtk

#endif // vidtk_reconcile_tracks_process_h_
