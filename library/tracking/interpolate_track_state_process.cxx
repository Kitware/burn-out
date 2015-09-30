/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "interpolate_track_state_process.h"

#include <vgl/vgl_homg_point_2d.h>
#include <vgl/algo/vgl_h_matrix_2d_compute_4point.h>

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/config_block.h>
#include <utilities/homography.h>

#include <tracking/tracking_keys.h>

namespace vidtk
{

typedef vcl_map< unsigned, track_state_sptr> state_history_map_type;
typedef vcl_map< unsigned, track_state_sptr>::iterator state_history_map_it;
typedef vcl_map< unsigned, track_state_sptr>::const_iterator state_history_map_cit;

typedef vcl_map< unsigned, state_history_map_type* > history_cache_map_type;
typedef vcl_map< unsigned, state_history_map_type* >::iterator history_cache_map_it;
typedef vcl_map< unsigned, state_history_map_type* >::const_iterator history_cache_map_cit;

class track_interpolation_record_type;
typedef vcl_list< track_interpolation_record_type > track_memory_type;
typedef vcl_list< track_interpolation_record_type >::iterator track_memory_type_it;

struct history_cache_type
{
  history_cache_map_type m;

  history_cache_type() {}
  ~history_cache_type();
  void synchronize( const vcl_vector< track_sptr >& track_list );
  track_state_sptr track_state_at_frame( unsigned track_id, unsigned frame) const;
  void load_track_history_up_to_frame( unsigned track_id, unsigned frame, vcl_vector< track_state_sptr >& h ) const;
  void add_interpolated_state( const track_memory_type_it& it );
}; // history_cache type

//
// holds the (TL, TS, H) tuple
//
// Instances  of this data type appear twice:
//
// - Once per input timestamp, created by to_packets() from the inputs
// - Once per output timestamp, created by either copying input packets or
//   interpolation
//
// The difference between input and output is that output tracks share track
// states, but not track structures, with input tracks.  This is so that interpolated
// states do not suddenly appear in tracks shared outside the process.
//
// The tl_index_gap_flag array is: tl_index_gap_flag[i] is true if tl[i] has a
// state at ts; false otherwise.  The packet is complete if there are no
// false entries in tl_index_gap_flag.  (Note this means an empty track
// list is complete.)
//

struct process_packet_type
{
  enum output_status_type {OUTPUT_NONE = 0x0, OUTPUT_H = 0x01, OUTPUT_T = 0x02, OUTPUT_READY = OUTPUT_H | OUTPUT_T };
  track_vector_t tl;
  timestamp ts;
  image_to_image_homography h;

  // carried along but not interpolated
  double gsd;

  unsigned gap_count;
  vcl_vector<bool> tl_index_gap_flag;

  int output_status;

public:

  process_packet_type( const history_cache_type& hc,
                       const track_vector_t& track_list,
                       const timestamp& time_stamp,
                       const image_to_image_homography& img2ref_h,
                       double g )
    : tl( track_list ), ts( time_stamp ), h( img2ref_h ), gsd(g), gap_count(0), output_status( OUTPUT_NONE )
  {
    log_assert( ts.is_valid(), "Bad timestamp " << ts << " on packet?");
    set_gap_count( tl, hc);
  }

  process_packet_type( const timestamp& time_stamp, double g )
    : ts( time_stamp ), gsd(g), gap_count(0), output_status( OUTPUT_NONE )
  {
    log_assert( ts.is_valid(), "Bad timestamp " << ts << " on packet?");
  }
  process_packet_type(): gap_count(0) {}
  unsigned track_gap_count() const { return gap_count; }
  void set_gap_count( const track_vector_t& track_list, const history_cache_type& hc )
  {
    for (unsigned i=0; i<track_list.size(); ++i)
    {
      bool index_has_state_on_ts = (hc.track_state_at_frame( track_list[i]->id(), ts.frame_number() ) != 0);
      tl_index_gap_flag.push_back( index_has_state_on_ts );
      if ( ! index_has_state_on_ts )
      {
        ++gap_count;
      }
      log_info( "ITSP: ppt-set-gap: frame " << ts.frame_number() << " track "
               << track_list[i]->id() << " has frame? " << index_has_state_on_ts << "\n");
    }
  }
};

//
// Similar to a process_packet_type; this struct holds a ts and h but only
// a single track.  Used in track interpolation; one per frame per track.
// Data cached here gets copied over into the interpolated track states
// which are eventually sent downstream.
//

struct track_interpolation_record_type
{
  track_sptr trk;
  timestamp ts;
  image_to_image_homography h;
  double gsd;
  bool is_valid_on_ts;
  track_state_sptr state_at_ts; // only valid if is_valid_on_ts is true

  // these are cached here before moving into the interpolated state
  vnl_matrix_fixed<double, 3, 4> interp_world_box;
  vnl_vector_fixed<double, 3> interp_world_velocity;
  vnl_vector_fixed<double, 3> interp_world_location;
  vnl_vector_fixed<double, 3> interp_image_location;
  vgl_box_2d<unsigned> interp_image_box;

  track_interpolation_record_type( const history_cache_type& hc, track_sptr track, const timestamp& time_stamp, const image_to_image_homography& homg )
    : trk(track), ts(time_stamp), h(homg), is_valid_on_ts(false), state_at_ts(0)
  {
    log_assert (trk != NULL, "interpolation record created for null track??" );
    state_at_ts = hc.track_state_at_frame( trk->id(), ts.frame_number() );
    is_valid_on_ts = (state_at_ts != 0);
  }

  track_state_sptr synthesize_track_state();
};

// This maps track IDs to temporally ordered lists of track_interpolation_records.
typedef vcl_map< unsigned, track_memory_type > track_map_type;
typedef vcl_map< unsigned, track_memory_type >::iterator track_map_it;

history_cache_type
::~history_cache_type()
{
  for (history_cache_map_it i=m.begin(); i != m.end(); ++i)
  {
    delete i->second;
  }
}

void
history_cache_type
::synchronize( const vcl_vector< track_sptr >& track_list )
{
  // For each track on the inputs:
  // - if it has no entry in the cache history (i.e. is a newly active track),
  // create an entry and load up the map of frame->state pointer.
  // - if it has an entry, add those states which do not already have entries.
  // These states will be upstream "true" states; we shouldn't get fed back
  // interpolated states.  After all, our inability to get fed back the
  // interpolated states we send downstream is the whole reason for this cache.
  // For each track in the map but NOT on the inputs:
  // - remove from the cache; it's no longer active.

  vcl_map< unsigned, bool > track_id_seen;
  for (unsigned i=0; i<track_list.size(); ++i )
  {
    track_sptr trk = track_list[i];
    track_id_seen[ trk->id() ] = true;
    history_cache_map_it e = m.find( trk->id());
    state_history_map_type* track_state_map = 0;
    if ( e == m.end() )
    {
      log_info( "ITSP: Adding track " << trk->id() << " to history cache with history of "
                << trk->history().size() << " states\n" );
      track_state_map = new state_history_map_type;
      m[ trk->id() ] = track_state_map;
    }
    else
    {
      track_state_map = e->second;
    }
    state_history_map_type& states = *track_state_map;

    const vcl_vector<track_state_sptr>& h = trk->history();
    for (unsigned j=0; j<h.size(); ++j)
    {
      track_state_sptr s = h[j];
      unsigned this_state_frame_number = h[j]->time_.frame_number();
      if (states.find( this_state_frame_number) == states.end() )
      {
        states[ this_state_frame_number ] = s;
      }
    }
  }
  for (history_cache_map_it i=m.begin(); i != m.end(); ++i )
  {
    if ( track_id_seen.find( i->first ) == track_id_seen.end() )
    {
      log_info( "ITSP: Removing track " << i->first << " from history cache\n" );
      history_cache_map_it target = i;
      delete i->second;
      m.erase( target );
    }
  }
}

track_state_sptr
history_cache_type
::track_state_at_frame( unsigned track_id, unsigned frame) const
{
  history_cache_map_cit e = m.find( track_id );
  log_assert( e != m.end(),
              "Request for track " << track_id << " frame " << frame << "; track not in cache" );

  state_history_map_it s = e->second->find( frame );
  return
    ( s != e->second->end() )
    ? s->second
    : 0;
}

void
history_cache_type
::load_track_history_up_to_frame( unsigned track_id, unsigned frame, vcl_vector< track_state_sptr >& h ) const
{
  history_cache_map_cit e = m.find( track_id );
  log_assert( e != m.end(),
              "Request for track " << track_id << " frame " << frame << "; track not in cache" );
  unsigned last_frame = 0;
  const state_history_map_type& states = *(e->second);
  h.clear();
  for (state_history_map_cit s = states.begin();
       s != states.end();
       ++s)
  {
    if ( s->first > frame ) return;
    h.push_back( s->second );
    if ( s != states.begin() )
    {
      unsigned gap_diff = s->first - last_frame;
      if (gap_diff != 1)
      {
        log_info( "ITSP: WHOA: Track " << track_id << " history up to " << frame
                  << ": gap of " << gap_diff << " from " << last_frame << " to " << s->first << "\n" );
      }
    }
    last_frame = s->first;
  }
}

void
history_cache_type
::add_interpolated_state( const track_memory_type_it& it )
{
  unsigned track_id = it->trk->id();
  unsigned frame = it->ts.frame_number();
  history_cache_map_it e = m.find( track_id );
  log_assert( e != m.end(),
                "Request for track " << track_id << " frame " << frame << "; track not in cache" );
  state_history_map_type& states = *(e->second);
  log_assert( states.find( frame) == states.end(),
              "Attempting to add interpolated frame " << frame
              << " to track " << track_id << " more than once?" );
  states[ frame ] = it->synthesize_track_state();
}

//
// offload the config_block logic here
//

struct itsp_params
{
  bool enabled;
  size_t tsv_live_index;
  double timestamp_to_secs;
  unsigned gap_buffer_max_length;
  itsp_params()
    : enabled(false), tsv_live_index(0), timestamp_to_secs( 1.0e6 )
  {
    blk.add_parameter( "enabled", "1", "1=enable, 0=disable (pass inputs; select timestamp from tsv via tsv_live_index)" );
    blk.add_parameter( "tsv_live_index", "0", "Offset in timestamp vector associated with homography & tracks" );
    blk.add_parameter( "timestamp_to_secs", "1.0e6", "Multiplicative factor to convert timestamps to seconds" );
    blk.add_parameter( "gap_buffer_max_length", "60", "set to same value as track_init_duration_frames" ) ;
  }
  vidtk::config_block get_params() const
  {
    return blk;
  }
  bool set_params( const vidtk::config_block& b )
  {
    try
    {
      b.get( "enabled", this->enabled );
      b.get( "tsv_live_index", this->tsv_live_index );
      b.get( "timestamp_to_secs", this->timestamp_to_secs );
      b.get( "gap_buffer_max_length", this->gap_buffer_max_length );
    }
    catch ( vidtk::unchecked_return_value& e )
    {
      log_error( "interpolate_track_state_process param error: " << e.what() );
      //      this->set_params( this->get_params() );
      return false;
    }

    this->blk.update( b );
    return true;
  }

private:
  vidtk::config_block blk;
};

//
// offload step() inputs and their status here
//

struct itsp_inputs
{
private:

  // track the set/unset state of inputs
  enum input_status_type { SET_NONE = 0x00,
                           SET_TRACKS = 0x01,
                           SET_IMG2REF_H = 0x02,
                           SET_REF2WORLD_H = 0x04,
                           SET_TSV = 0x08,
                           SET_GSD = 0x10,
                           SET_ALL = SET_TRACKS | SET_IMG2REF_H | SET_REF2WORLD_H | SET_TSV | SET_GSD };

  // checked and cleared at top of step()
  int input_status;

public:

  track_vector_t tracks;
  image_to_image_homography img2ref_h;
  image_to_utm_homography ref2world_h;
  vcl_vector< timestamp > tsv;
  double gsd;

  itsp_inputs()
    : input_status( SET_NONE )
  {}

  void set_tracks( const track_vector_t& t ) { input_status |= SET_TRACKS; tracks = t; }
  void set_img2ref_h( const image_to_image_homography& h ) { input_status |= SET_IMG2REF_H; img2ref_h = h; }
  void set_ref2world_h( const image_to_utm_homography& h ) { input_status |= SET_REF2WORLD_H; ref2world_h = h; }
  void set_tsv( const vcl_vector< timestamp >& t)
  {
    if( t.empty() )
    {
  	log_error( "ITSP input error: empty timestamp vector.\n" );
    }
    input_status |= SET_TSV; tsv = t;
  }

  void set_gsd( double g ) { input_status |= SET_GSD; gsd = g; }
  bool all_inputs_present() const { return input_status == SET_ALL; }
  void reset_flag() { input_status = SET_NONE; }
  vcl_vector< process_packet_type > to_packets( const history_cache_type& hc, size_t live_index );
};

class interpolate_track_state_process_impl;

// this class manages the anchor0 / anchor 1 setting and detection
// business, but does not own the gap packets.  From the ITSP_impl's
// point of view, emit_and_clear cannot occur until both managers agree
// (or a timeout occurs.)

class interpolation_manager
{
public:

  enum interp_status {NIL, ANCHOR_0, GAP, ANCHOR_1};
  explicit interpolation_manager( interpolate_track_state_process_impl* p ):
    parent( p )
  {
    reset();
  }

  virtual ~interpolation_manager() {}
  virtual interp_status get_and_record_status( process_packet_type& p ) = 0;
  virtual bool interpolate( const itsp_inputs& inp ) = 0;
  static vcl_string state2str( interp_status s )
  {
    switch (s)
    {
    case NIL: return "NIL";
    case ANCHOR_0: return "anchor_0";
    case GAP: return "gap";
    case ANCHOR_1: return "anchor_1";
    }
    return "bad status?";
  }
  void reset()
  {
    anchor_0.first = false;
    anchor_1.first = false;
    pending_frame_ids.clear();
  }
  interp_status get_manager_state() const
  {
    // order matters here; test most restrictive first
    if (anchor_1.first) return ANCHOR_1;
    if ( ! pending_frame_ids.empty())  return GAP;
    if (anchor_0.first) return ANCHOR_0;
    return NIL;
  }

protected:
  interpolate_track_state_process_impl* parent;
  vcl_pair< bool, unsigned> anchor_0;  // set?  frame_number
  vcl_pair< bool, unsigned> anchor_1;  // set?  frame_number
  vcl_vector< unsigned > pending_frame_ids;
};

class homography_interpolation_manager: public interpolation_manager
{
public:

  homography_interpolation_manager( interpolate_track_state_process_impl* p ):
    interpolation_manager(p)
  {}
  virtual interp_status get_and_record_status( process_packet_type& p );
  virtual bool interpolate( const itsp_inputs& inp );
  static bool normalize_h_pts( vnl_matrix_fixed<double, 3, 4>& pts )
  {
    for (unsigned i=0; i<4; ++i)
    {
      if (pts(2, i) == 0.0) return false;
      pts(0, i) /= pts(2, i);
      pts(1, i) /= pts(2, i);
      pts(2, i) = 1.0;
    }
    return true;
  }

};

class track_interpolation_manager: public interpolation_manager
{
public:

  track_interpolation_manager( interpolate_track_state_process_impl* p ):
    interpolation_manager(p)
  {}
  virtual interp_status get_and_record_status( process_packet_type& p );
  virtual bool interpolate( const itsp_inputs& inp );

  void interpolate_on_packet( unsigned frame_id, track_map_type& mem, const itsp_inputs& inp );
  void interpolate_states_for_a_track( track_memory_type& mem, const itsp_inputs& inp );
  void move_interpolated_states_into_a_track( unsigned id, track_memory_type& mem );

private:
  vcl_map<unsigned, unsigned> anchor_0_track_IDs; // track id -> 0x01

  void set_anchor_0( const process_packet_type& p )
  {
    anchor_0.first = true;
    anchor_0.second = p.ts.frame_number();
    anchor_0_track_IDs.clear();
    for (unsigned i=0; i<p.tl.size(); ++i)
    {
      anchor_0_track_IDs[ p.tl[i]->id() ] = 0x01;
    }
  }
  bool has_interval_gap( const process_packet_type& p)
  {
    vcl_map< unsigned, unsigned > local_map = anchor_0_track_IDs;
    for (unsigned i=0; i<p.tl.size(); ++i)
    {
      local_map[ p.tl[i]->id() ] += 0x02;
    }
    bool all_present = true;
    for (vcl_map<unsigned, unsigned>::iterator i=local_map.begin();
         i != local_map.end();
         ++i)
    {
      if ( i->second != 0x03) all_present = false;
    }
    log_info( "ITSP: has_interval_gap: " << p.ts.frame_number() << " w/ " << p.tl.size() << " tracks: " <<
              " gc " << p.gap_count << " a0 " << this->anchor_0.first << " all-present " << all_present << " on " << local_map.size() << " entries\n" );
    if (p.gap_count > 0) return true;
    if (! this->anchor_0.first) return false;
    return ( ! all_present );
  }
};

//
// implementation of interpolate_track_state_process
//

class interpolate_track_state_process_impl
{
public:

  explicit interpolate_track_state_process_impl( interpolate_track_state_process* parent );
  ~interpolate_track_state_process_impl() { /* fixme: clean out history cache */ }

  config_block params() const { return pblk.get_params(); }
  bool set_params( const config_block& blk ) { return pblk.set_params( blk ); }

  bool initialize() { return reset(); }
  bool reset();
  process::step_status step2();

  void set_input_tracks( const track_vector_t& tl ) { inp.set_tracks( tl ); }
  void set_input_img2ref_homography( const image_to_image_homography& hi ) { inp.set_img2ref_h( hi ); }
  void set_input_ref2world_homography( const image_to_utm_homography& hr ) { inp.set_ref2world_h( hr ); }
  void set_input_timestamps( const vcl_vector< timestamp >& tsv ) { inp.set_tsv( tsv ); }
  void set_input_gsd( double gsd ) { inp.set_gsd( gsd ); }

  const track_vector_t& output_track_list() const { return out_track_list; }
  const timestamp& output_timestamp() const { return out_timestamp; }
  const image_to_image_homography& output_img2ref_homography() const { return out_homography; }
  const image_to_utm_homography& output_ref2utm_homography() const { return out_utm_homography; }
  const double& output_gsd() const { return out_gsd; }
  history_cache_type& get_history_cache() { return history_cache; }
  double get_timestamp_to_secs() const { return pblk.timestamp_to_secs; }

  process_packet_type& get_pending_packet( unsigned frame )
  {
    vcl_map< unsigned, unsigned>::iterator i =
      this->frame_to_pending_packet_index_map.find( frame );
    log_assert( i != this->frame_to_pending_packet_index_map.end(),
                "Request for packet for frame " << frame << ": frame not in map?\n" );
    log_assert( i->second < pending_packet_list.size(),
                "Request for packet for frame " << frame << ": index "
                << i->second << " outside buffer (length "
                << this->pending_packet_list.size() << ")\n" );
    log_assert( this->pending_packet_list[ i->second ].ts.frame_number() == frame,
                "Request for packet for frame " << frame << ": index "
                << i->second << " is actually frame " <<
                this->pending_packet_list[ i->second ].ts.frame_number() << "?\n" );

    return this->pending_packet_list[ i->second ];
  }

private:

  // pointer to "parent" process to access push_output
  interpolate_track_state_process* parent;

  // user-visible params
  itsp_params pblk;

  // inputs
  itsp_inputs inp;

  // flag to synchronize the push_output process
  bool first_pump;

  // outputs
  track_vector_t out_track_list;
  timestamp out_timestamp;
  image_to_image_homography out_homography;
  image_to_utm_homography out_utm_homography;
  double out_gsd;

  // the two interpolation managers
  homography_interpolation_manager h_manager;
  track_interpolation_manager t_manager;

  // time-ordered list of pending packets
  vcl_vector< process_packet_type > pending_packet_list;

  // Maps of timestamp framenumbers to their packet index; used
  // for matching track_interpolation_records up to their packets
  vcl_map< unsigned, unsigned > frame_to_pending_packet_index_map;

  // On each step, the tracker supplies tracks whose histories know
  // nothing of whatever interpolation we've already done.  So we
  // have to maintain this cache of the histories we generate.
  // Add on first sight, remove when not in active list.
  history_cache_type history_cache;

  // methods

  void pump_outputs()
  {
    if (this->first_pump)
    {
      // don't pump the first time through
      this->first_pump = false;
    }
    else
    {
      vcl_cerr << "Outputs pumped for frame " << this->out_timestamp.frame_number() << "\n";
      this->parent->push_output( process::SUCCESS );
    }
  }

  void add_to_pending_packet_list( const process_packet_type& p )
  {
    this->pending_packet_list.push_back( p );
    this->frame_to_pending_packet_index_map[ p.ts.frame_number() ] =
      this->pending_packet_list.size()-1;
  }

  unsigned process_packet( process_packet_type& p );

  // this is only called if we're not enabled;
  // it passes through the inputs
  process::step_status copy_inputs_to_outputs()
  {
    // A non-zero live index (say, 2) means that typically
    // we would expect to see the live timestamp in tsv[2].
    // However, on startup and resets, the tsv may not be fully
    // populated, in which case we'll take the first index in the tsv.

    // see comment in copy_inputs_to_outputs()

    // note that we push first, then place new outputs on pads.
    this->pump_outputs();

    this->out_track_list = this->inp.tracks;
    this->out_homography = this->inp.img2ref_h;
    this->out_utm_homography = this->inp.ref2world_h;
    this->out_gsd = this->inp.gsd;
    if ( this->inp.tsv.empty() )
    {
      /* FIXME: change tracking superprocess to avoid this input case */
      // leave timestamp value alone for now
    }
    else
    {
      size_t match_index =
        ( this->pblk.tsv_live_index >= this->inp.tsv.size() )
        ? this->inp.tsv.size() - 1
        : this->pblk.tsv_live_index;

      this->out_timestamp = this->inp.tsv[ match_index ];
    }

    return process::SUCCESS;
  }

  bool gap_time_limit_okay( const process_packet_type& p )
  {
    if ( this->pending_packet_list.empty() ) return true;
    unsigned frame_number_diff =
      p.ts.frame_number() - this->pending_packet_list[0].ts.frame_number();
    return ( frame_number_diff < this->pblk.gap_buffer_max_length );
  }

  void mark_all_pending_as_tracking_complete()
  {
    for (unsigned i=0; i<this->pending_packet_list.size(); ++i)
    {
      this->pending_packet_list[i].output_status |= process_packet_type::OUTPUT_T;
    }
  }

  int emit_and_clear_output_packets();



};



interpolation_manager::interp_status
homography_interpolation_manager
::get_and_record_status( process_packet_type& p )
{
  log_assert( this->anchor_1.first == false,
              "Processing a packet without clearing out the old anchor1?");

  bool has_h = p.h.is_valid();
  if (( ! has_h ) && ( ! this->anchor_0.first ))
  {
    // has no homography, we haven't seen a valid homography-- reject it
    // ...and also indicate it's okay to flush it
    p.output_status |= process_packet_type::OUTPUT_H;
    return interpolation_manager::NIL;
  }
  if (( ! has_h ) && this->anchor_0.first )
  {
    // has no homography, but we have an anchor-- store it as a gap
    this->pending_frame_ids.push_back( p.ts.frame_number() );
    return interpolation_manager::GAP;
  }
  if ( has_h && (! this->anchor_0.first ))
  {
    // has homography, we need an anchor-- voila!
    this->anchor_0 = vcl_make_pair( true, p.ts.frame_number() );
    return interpolation_manager::ANCHOR_0;
  }
  if ( has_h && this->anchor_0.first )
  {
    if (this->pending_frame_ids.empty())
    {
      // no gaps to interpolate across-- move the anchor along;
      // mark previous anchor as ready for output
      process_packet_type& old_a0 =
        this->parent->get_pending_packet( this->anchor_0.second );
      old_a0.output_status |= process_packet_type::OUTPUT_H;
      this->anchor_0 = vcl_make_pair( true, p.ts.frame_number() );
      return interpolation_manager::ANCHOR_0;
    }
    else
    {
      // has homography, we have an anchor, we have gaps-- set to anchor 1
      this->anchor_1 = vcl_make_pair( true, p.ts.frame_number() );
      return interpolation_manager::ANCHOR_1;
    }
  }
  // shouldn't get here
  log_error( "Bad logic in homography interpolation manager!" );
  return interpolation_manager::NIL;
}

bool
homography_interpolation_manager
::interpolate( const itsp_inputs& inp )
{
  log_assert( (this->anchor_1.first == true) &&
              (this->anchor_0.first == true) &&
              (! this->pending_frame_ids.empty() ),
              "Preconditions for homography interpolation not set!" );

  vcl_map<unsigned, bool> dbg_flagged_frame_ids;

  // We have H0 and H1 from anchor-0 and anchor-1.  Map (0,0) and (say)
  // 1000,1000 into world space, linearly interpolate once for each of
  // the gap timestamps and use those world coords to produce new homographies.

  const double corner_pt = 1000.0;

  process_packet_type& a0 = this->parent->get_pending_packet( this->anchor_0.second );
  const process_packet_type& a1 = this->parent->get_pending_packet( this->anchor_1.second );

  a0.output_status |= process_packet_type::OUTPUT_H;
  dbg_flagged_frame_ids[ a0.ts.frame_number() ] = true;
  // set up corner points as a 3x4
  vnl_matrix_fixed< double, 3, 4 > h0_src_pts, h0_world_pts, h1_src_pts, h1_world_pts;
  h0_src_pts(0,0) = 0.0;       h0_src_pts(1,0) = 0.0;
  h0_src_pts(0,1) = corner_pt; h0_src_pts(1,1) = 0.0;
  h0_src_pts(0,2) = corner_pt; h0_src_pts(1,2) = corner_pt;
  h0_src_pts(0,3) = 0.0;       h0_src_pts(1,3) = corner_pt;
  for (unsigned i=0; i<4; ++i) h0_src_pts(2,i) = 1.0;

  h1_src_pts = h0_src_pts;

  // map src pts to ref and then to world (note the RPN-like pre-multiply order, ha ha!)

  h0_world_pts =
    inp.ref2world_h.get_transform().get_matrix() *   // ref2wld
    a0.h.get_transform().get_matrix() * // src2ref
    h0_src_pts;

  h1_world_pts =
    inp.ref2world_h.get_transform().get_matrix() *   // ref2wld
    a1.h.get_transform().get_matrix() *              // src2ref
    h1_src_pts;

  if ( ! normalize_h_pts( h0_src_pts)) return false;
  if ( ! normalize_h_pts( h1_src_pts)) return false;
  if ( ! normalize_h_pts( h0_world_pts)) return false;
  if ( ! normalize_h_pts( h1_world_pts)) return false;

  // convert into offsets from h0 per frame
  double nframes = a1.ts.frame_number() - a0.ts.frame_number();
  vnl_matrix_fixed< double, 3, 4 > h0_world_diff_per_frame =
    (h1_world_pts - h0_world_pts) / nframes;

  vcl_vector< vgl_homg_point_2d<double> > image_points;
  image_points.push_back( vgl_homg_point_2d<double>( 0.0, 0.0 ));
  image_points.push_back( vgl_homg_point_2d<double>( corner_pt, 0.0 ));
  image_points.push_back( vgl_homg_point_2d<double>( corner_pt, corner_pt ));
  image_points.push_back( vgl_homg_point_2d<double>( 0.0, corner_pt ));

  // we'll need this for each gap frame
  vgl_h_matrix_2d<double> world2ref_h = inp.ref2world_h.get_transform().get_inverse();

  /*
  vcl_cout << "ITSP::IH: a0 @ " << this->anchor_0.second.ts << "\n" << this->anchor_0.second.h << vcl_endl;
  vcl_cout << "ITSP::IH: a1 @ " << anchor_1.ts << "\n" << anchor_1.h << vcl_endl;
  vcl_cout << "ITSP::IH: r2w\n" << this->inp.ref2world_h << vcl_endl;
  vcl_cout << "ITSP::IH: w2r\n" << world2ref_h << vcl_endl;
  vcl_cout << "ITSP::IH: h0 world pts\n" << h0_world_pts << vcl_endl;
  vcl_cout << "ITSP::IH: h0 diff/frame\n" << h0_world_diff_per_frame << vcl_endl;
  */
  // for each gap...
  for (unsigned i=0; i<this->pending_frame_ids.size(); ++i)
  {
    process_packet_type& p = this->parent->get_pending_packet( this->pending_frame_ids[i] );
    if (p.h.is_valid())
    {
      p.output_status |= process_packet_type::OUTPUT_H;
      dbg_flagged_frame_ids[ p.ts.frame_number() ] = true;
      log_info( "ITSP::IH: frame " << p.ts.frame_number() << " is already valid\n" );
      continue;
    }

    // ...compute the new world points
    double offset = p.ts.frame_number() - a0.ts.frame_number();
    //    vcl_cout << "ITSP::IH gap " << i << " @ " << p.ts << " (" << offset << " frames)\n";
    vnl_matrix_fixed< double, 3, 4 > ith_world_points;
    for (unsigned j=0; j<4; ++j)
    {
      ith_world_points( 0, j ) =
        h0_world_pts(0, j) + ( h0_world_diff_per_frame(0, j) * offset );
      ith_world_points( 1, j ) =
        h0_world_pts(1, j) + ( h0_world_diff_per_frame(1, j) * offset );
      ith_world_points( 2, j) =
        1.0;
    }

    // ...map those world points into the *reference* frame
    vnl_matrix_fixed<double, 3, 4> ith_reference_points =
      world2ref_h.get_matrix() * ith_world_points;
    for (unsigned j=0; j<4; ++j)
    {
      if (ith_reference_points(2, j) == 0.0) return false;
      ith_reference_points(0, j) /= ith_reference_points(2, j);
      ith_reference_points(1, j) /= ith_reference_points(2, j);
      ith_reference_points(2, j) = 1.0;
    }

    // Those are the location of the image_points in the reference frame.
    // Our goal is the src2ref homography which maps image_points to those
    // points.

    /*
    vcl_cout << "ISTP::IH gap " << i << " world pts\n" << ith_world_points << vcl_endl;
    vcl_cout << "ISTP::IH gap " << i << " ref pts\n" << ith_reference_points << vcl_endl;
    */

    vcl_vector< vgl_homg_point_2d<double> > ith_ref_homog_points;
    for (unsigned j=0; j<4; ++j)
    {
      ith_ref_homog_points.push_back(
        vgl_homg_point_2d<double>(
          ith_reference_points(0, j),
          ith_reference_points(1, j) ));
    }

    // compute!

    vgl_h_matrix_2d<double> interpolated_src_to_ref_h;
    vgl_h_matrix_2d_compute_4point c;
    c.compute( image_points,
               ith_ref_homog_points,
               interpolated_src_to_ref_h );

    // store!

    //    vcl_cout << "ISTP::IH gap " << i << " got:\n" << interpolated_src_to_ref_h << vcl_endl;
    p.output_status |= process_packet_type::OUTPUT_H;
    dbg_flagged_frame_ids[ p.ts.frame_number() ] = true;
    p.h.set_transform( interpolated_src_to_ref_h );

  } // ...for each pending frame

  // move the anchor frame from 1 to 0, clear the pending list
  this->anchor_0 = this->anchor_1;
  this->anchor_1.first = false;
  this->pending_frame_ids.clear();
  // clear anchor 0's ready-to-output flag to retain it
  process_packet_type& new_a0 = this->parent->get_pending_packet( this->anchor_0.second );
  new_a0.output_status &= (~ process_packet_type::OUTPUT_H );
  dbg_flagged_frame_ids[ this->anchor_0.second ] = false;
  // all done

  log_info( "ITSP: Homography interpolation complete; flagged these frames:\n");
  for (vcl_map<unsigned, bool>::iterator i=dbg_flagged_frame_ids.begin(); i != dbg_flagged_frame_ids.end(); ++i)
  {
    process_packet_type& tmp = this->parent->get_pending_packet( i->first );
    log_info( "ITSP: ..." << i->first << " : " << i->second << " " << tmp.output_status << "\n");
  }
  return true;
}


interpolation_manager::interp_status
track_interpolation_manager
::get_and_record_status( process_packet_type& p )
{
  log_assert( this->anchor_1.first == false,
              "Processing a packet without clearing out the old anchor1?");

  // These are still at the PROCESS packet level.  One packet per timestamp,
  // possibly with multiple frames.

  bool has_gap = this->has_interval_gap( p );

  if ( has_gap && ( ! this->anchor_0.first ))
  {
    // has a gap, but we have no anchor0.  Bail (but mark as okay to flush)
    log_info( "ITSP: TIM: " << p.ts.frame_number() << " has gap? " << has_gap <<
              " no anchor0; -> NIL\n" );
    p.output_status |= process_packet_type::OUTPUT_T;
    return interpolation_manager::NIL;
  }
  if ( has_gap && this->anchor_0.first )
  {
    // has a gap, we've seen an anchor0 ... report as gap packet.
    log_info( "ITSP: TIM: " << p.ts.frame_number() << " has gap? " << has_gap <<
              " have anchor0; -> GAP\n" );
    this->pending_frame_ids.push_back( p.ts.frame_number() );
    return interpolation_manager::GAP;
  }
  if ( ( ! has_gap) && ( ! this->anchor_0.first ))
  {
    // All tracks present, and we have no anchor0... set it IF it has tracks
    // otherwise return NIL (and mark as okay to flush)
    if ( p.tl.empty() )
    {
      log_info( "ITSP: TIM: " << p.ts.frame_number() << " has gap? " << has_gap <<
                " no anchor0; but no tracks -> NIL\n" );
      p.output_status |= process_packet_type::OUTPUT_T;
      return interpolation_manager::NIL;
    }
    else
    {
      log_info( "ITSP: TIM: " << p.ts.frame_number() << " has gap? " << has_gap <<
                " no anchor0; -> set anchor 0\n" );
      this->set_anchor_0( p );
      return interpolation_manager::ANCHOR_0;
    }
  }
  if ( ( ! has_gap) && ( this->anchor_0.first ))
  {
    if (this->pending_frame_ids.empty())
    {
      // no gaps to interpolate across... move the anchor along (mark old for output)
      log_info( "ITSP: TIM: " << p.ts.frame_number() << " has gap? " << has_gap <<
                " anchor0; no gap frames; -> move anchor 0\n" );
      process_packet_type& old_a0 =
        this->parent->get_pending_packet( this->anchor_0.second );
      old_a0.output_status |= process_packet_type::OUTPUT_T;
      this->set_anchor_0( p );
      return interpolation_manager::ANCHOR_0;
    }
    else
    {
      // complete and material to interpolate... set as anchor 1
      log_info( "ITSP: TIM: " << p.ts.frame_number() << " has gap? " << has_gap <<
                " anchor0; gap frames; -> anchor1\n" );
      this->anchor_1 = vcl_make_pair( true, p.ts.frame_number() );
      return interpolation_manager::ANCHOR_1;
    }
  }
  // shouldn't get here
  log_error( "Bad logic in homography interpolation manager!" );
  return interpolation_manager::NIL;
}

bool
track_interpolation_manager
::interpolate( const itsp_inputs& inp )
{
  log_assert( (this->anchor_1.first == true) &&
              (this->anchor_0.first == true) &&
              (! this->pending_frame_ids.empty() ),
              "Preconditions for homography interpolation not set!" );

  // sanity check
  bool interval_complete =
    (this->pending_frame_ids.size() == (this->anchor_0.second - this->anchor_1.second + 1));
  log_info( "ITSP: Interpolating from " << this->anchor_0.second << " to "
            << this->anchor_1.second << " across " << this->pending_frame_ids.size()
            << " gap frames (complete? " << interval_complete << ")\n" );

  // Assume the gap packets all have interpolated homographies
  // (i.e. h_manager.interpolate() has already been called as
  // often as it's going to before this.)

  // This is probably a little more complicated than it needs to be.
  // We asserted that tracks cannot start or end in a gap, but
  // consider the following scenario:
  // frame 10:  tracks 1 & 2
  // frame 11:  track 1 ends; track 2 misses detection
  // frame 12:  track 2 has detection
  // ... frame 11 is a gap frame, but track 1 cannot (and shouldn't)
  // be interpolated to frame 12.  This scenario could be extended
  // to extremes where tracks start and end between the anchors.
  // The anchors are guarantees that NO interpolation will cross
  // those boundaries, but a track could have good frames between
  // the anchors, or end between them.
  //
  // Annoyingly, we need to re-do the anchor detection logic but
  // now on a per-track basis, and there may be multiple anchor-gap-anchor
  // interpolation zones within the larger range bracked by anchor0/anchor1.
  // This could be refactored but... get it working first, then make it fast.
  //

  // build up the per-track list: track_id -> list of track interpolation records
  track_map_type m;

  vcl_map<unsigned, bool> dbg_flagged_frame_ids;

  // initialize m with the active tracks in anchor_0
  process_packet_type& a0 = this->parent->get_pending_packet( anchor_0.second );
  a0.output_status |= process_packet_type::OUTPUT_T;
  dbg_flagged_frame_ids[ anchor_0.second ] = true;
  for (unsigned i=0; i<a0.tl.size(); ++i)
  {
    track_memory_type mem;
    mem.push_back(
      track_interpolation_record_type(
        this->parent->get_history_cache(), a0.tl[i], a0.ts, a0.h ));

    m[ a0.tl[i]->id() ] = mem;
    log_assert( mem.front().is_valid_on_ts, "anchor_0 not true for track "
                << a0.tl[i]->id() << " on frame "
                << mem.front().ts.frame_number() << "?\n" );
  }

  /*
  // interpolate on anchor0
  this->interpolate_on_packet( this->anchor_0.second, m, inp );
  */

  // move forward, closing out tracks as we can
  for (unsigned i=0; i<this->pending_frame_ids.size(); ++i)
  {
    this->interpolate_on_packet( this->pending_frame_ids[i], m, inp );
    dbg_flagged_frame_ids[ this->pending_frame_ids[i] ] = true;
  }

  // interpolate on anchor 1-- when we push it back to anchor 0, it'll
  // be used to reinitialize the track memory

  this->interpolate_on_packet( this->anchor_1.second, m, inp );
  dbg_flagged_frame_ids[ this->anchor_1.second ] = true;

  this->anchor_0 = this->anchor_1;
  this->anchor_1.first = false;
  this->pending_frame_ids.clear();
  // clear anchor 0's ready-to-output flag to retain it
  process_packet_type& new_a0 = this->parent->get_pending_packet( this->anchor_0.second );
  new_a0.output_status &= (~ process_packet_type::OUTPUT_T );
  dbg_flagged_frame_ids[ this->anchor_0.second ] = false;

  log_info( "ITSP: Track interpolation complete; flagged these frames:\n");
  for (vcl_map<unsigned, bool>::iterator i=dbg_flagged_frame_ids.begin(); i != dbg_flagged_frame_ids.end(); ++i)
  {
    process_packet_type& tmp = this->parent->get_pending_packet( i->first );
    log_info( "ITSP: ..." << i->first << " : " << i->second << " " << tmp.output_status << "\n");
  }

  return true;
}

void
track_interpolation_manager
::interpolate_on_packet( unsigned frame_id, track_map_type& m, const itsp_inputs& inp )
{
  process_packet_type& p = this->parent->get_pending_packet( frame_id );
  // this packet's information needs to get copied into
  // track interpolation records.  If the track list is empty,
  // create placeholder records.

  // Once a packet has been through this routine, it's as ready for output
  // as it will ever be.  We'll clear the new anchor 0's flag later.
  p.output_status |= process_packet_type::OUTPUT_T;

  vcl_map< unsigned, bool> track_id_seen_on_this_frame;

  // remember track IDs ready for interpolation after this packet
  // is added to their pile
  vcl_vector< unsigned > interp_targets;

  for (unsigned i=0; i<p.tl.size(); ++i)
  {
    unsigned this_track_id = p.tl[i]->id();
    track_memory_type& mem = m[ this_track_id ];
    track_id_seen_on_this_frame[ this_track_id ] = true;

    track_interpolation_record_type
      this_frame_record( this->parent->get_history_cache(), p.tl[i], p.ts, p.h );

    log_info( "ITSP: packet frame " << p.ts.frame_number() << " track " << this_track_id << " ( " << i << " of " << p.tl.size() << "):\n" );
    log_info( "ITSP: ...mem.empty(): " << mem.empty() << " (size " << mem.size() << ")\n");
    if ( ! mem.empty() ) {
      log_info( "ITSP: ...mem.back().is_valid_on_ts: " << mem.back().is_valid_on_ts << "\n");
      log_info( "ITSP: ...mem.back() is " << mem.back().ts.frame_number() << "\n");
    }
    log_info( "ITSP: ...this_frame_record.is_valid_on_ts:" << this_frame_record.is_valid_on_ts << "\n");
    // order matters-- check first, then add this frame
    bool can_interpolate =
      (! mem.empty() ) &&
      (! mem.back().is_valid_on_ts ) &&
      this_frame_record.is_valid_on_ts;
    if ( can_interpolate)
    {
      log_info( "ITSP: Scheduling " << this_track_id << " for interpolating up to " << p.ts.frame_number() << "\n");
      interp_targets.push_back( this_track_id );
    }

    // if can_interpolate, this_frame_record is "anchor_1" for the track
    mem.push_back( this_frame_record );
  }
  log_info( "ITSP: @" << p.ts.frame_number() << "; finished processing " << p.tl.size() << " packet tracks; memory has " << m.size() << " tracks; about to insert placeholders\n" );

  // next, for any tracks in the overall list not in this frame's
  // list, enter placeholders carrying timestamp and homography
  for (track_map_it j = m.begin(); j != m.end(); ++j)
  {
    if ( ! track_id_seen_on_this_frame[j->first] )
    {
      track_sptr this_track_ptr = j->second.front().trk;
      log_info( "ITSP: Inserting placeholder for " << j->first << "\n");
      log_assert( this_track_ptr.ptr() != 0, "placeholder for missing track " << j->first << " can't get front's track ptr?" );
      // This is a gap track; give it a clone of anchor 0's track to build
      // upon in move_interpolated_states_into_a_track().
      j->second.push_back(
        track_interpolation_record_type(
          this->parent->get_history_cache(), this_track_ptr->clone(), p.ts, p.h ));
    }
  }

  // next, interpolate any tracks we can and remove them from the track map
  for (unsigned j=0; j<interp_targets.size(); ++j)
  {
    unsigned id = interp_targets[j];
    this->interpolate_states_for_a_track( m[id], inp );
    this->move_interpolated_states_into_a_track( id, m[id] );
    m.erase( id );
  }
    // all done
}


void
track_interpolation_manager
::interpolate_states_for_a_track( track_memory_type& mem, const itsp_inputs& inp )
{
  // at this point, we have the track memory to interpolate against;
  // front and back are the anchors
  //
  // Go through and set the interp_* members of the
  // gap track_interpolation_records

  // map the anchor boxes to world coords
  log_assert( mem.front().is_valid_on_ts, "Front invalid on ts?" );
  log_assert( mem.back().is_valid_on_ts, "Back invalid on ts?" );
  vcl_vector< image_object_sptr > objs;

  if ( ! mem.front().state_at_ts->data_.get( tracking_keys::img_objs, objs ))
  {
    return;
  }
  vgl_box_2d<unsigned> a0_box = objs[0]->bbox_;

  if ( ! mem.back().state_at_ts->data_.get(  tracking_keys::img_objs, objs ))
  {
    return;
  }
  vgl_box_2d<unsigned> a1_box = objs[0]->bbox_;

  vnl_matrix_fixed< double, 3, 4 > a0_img_pts, a0_world_pts, a1_img_pts, a1_world_pts;
  a0_img_pts(0,0) = a0_box.min_x();   a0_img_pts(1, 0) = a0_box.min_y();
  a0_img_pts(0,1) = a0_box.max_x();   a0_img_pts(1, 1) = a0_box.min_y();
  a0_img_pts(0,2) = a0_box.max_x();   a0_img_pts(1, 2) = a0_box.max_y();
  a0_img_pts(0,3) = a0_box.max_x();   a0_img_pts(1, 3) = a0_box.max_y();
  for (unsigned i=0; i<4; ++i) a0_img_pts(2,i) = 1.0;

  a1_img_pts(0,0) = a1_box.min_x();   a1_img_pts(1, 0) = a1_box.min_y();
  a1_img_pts(0,1) = a1_box.max_x();   a1_img_pts(1, 1) = a1_box.min_y();
  a1_img_pts(0,2) = a1_box.max_x();   a1_img_pts(1, 2) = a1_box.max_y();
  a1_img_pts(0,3) = a1_box.max_x();   a1_img_pts(1, 3) = a1_box.max_y();
  for (unsigned i=0; i<4; ++i) a1_img_pts(2,i) = 1.0;

  a0_world_pts =
    inp.ref2world_h.get_transform().get_matrix() *
    mem.front().h.get_transform().get_matrix() *
    a0_img_pts;

  a1_world_pts =
    inp.ref2world_h.get_transform().get_matrix() *
    mem.back().h.get_transform().get_matrix() *
    a1_img_pts;

  homography_interpolation_manager::normalize_h_pts( a0_world_pts );
  homography_interpolation_manager::normalize_h_pts( a1_world_pts );

  // express a1 as per-frame differences from a0
  double nframes = mem.back().ts.frame_number() - mem.front().ts.frame_number();
  vnl_matrix_fixed< double, 3, 4 > a0_world_diff_per_frame =
    (a1_world_pts - a0_world_pts ) / nframes;

  // create the boxes and positions
  // (FIXME: verify that this is how positions should be computed)
  // (arslan sent a more stable version in email to roddy), which read:

  /*
I would be careful trying to interpolate the four corners of the box
independently. In the cases when we will have large perspective
distortion this could produce unrealistic data.

Another consideration is that the world position and velocity
(track_state::loc_, vel_) are directly produced the the Kalman filter
process, so in theory these are the *smoothed* versions of the
projected world position of the foot position.

Now that you have asked my opinion, I would suggest this:

1. Interpolate loc_ independently.
2. Interpolate vel_ independently, i.e. from the vel_ at the two ends
of your interpolation window.
3. Copy track_state::loc_ to the *corresponding*
image_object::world_pos_ (?). This will be an approximation as in the
non-interpolated loc_ doesn't have to be exactly same as world_pos_
due to Kalman smoothing. FYI, image_object::world_pos_ is the
projected (into world) version of image_object::pos_ (?).
4. image_object::pos_ = H_wld2img * image_object::world_pos_. Note
that image_object::pos_ is the *foot point*.
5. Interpolated image_object::bbox_ should be formed by merely
interpolating the width and height of the boxes (in the image space)
and aligning it with image_object::pos_ computed above as the foot
point.
  */

  track_memory_type_it it = mem.begin();  // point to a0
  track_memory_type_it anchor1 = mem.end();
  --anchor1;
  while (++it != anchor1) // skip a0 and a1
  {
    double dt = it->ts.frame_number() - mem.front().ts.frame_number();

    // set the world box
    it->interp_world_box = a0_world_pts + ( a0_world_diff_per_frame * dt );

    // map world box back to image
    vgl_h_matrix_2d<double> interp_src_to_world =
      inp.ref2world_h.get_transform().get_matrix() *
      it->h.get_transform().get_matrix(); // src2ref (interpolated)
    vnl_matrix_fixed< double, 3, 4 > interp_image_box_pts =
      interp_src_to_world.get_inverse().get_matrix() *
      it->interp_world_box;
    homography_interpolation_manager::normalize_h_pts( interp_image_box_pts );

    vgl_box_2d<unsigned>& ib = it->interp_image_box;
    ib.set_min_x( static_cast<unsigned>( interp_image_box_pts.get_row(0).min_value() ));
    ib.set_max_x( static_cast<unsigned>( interp_image_box_pts.get_row(0).max_value() ));
    ib.set_min_y( static_cast<unsigned>( interp_image_box_pts.get_row(1).min_value() ));
    ib.set_max_y( static_cast<unsigned>( interp_image_box_pts.get_row(1).max_value() ));

    // get the image object position at the "feet" of the detection
    // ...actually, this looks weird.  Place at center of box until Arslan's email is implemented.
    vnl_matrix_fixed<double, 3, 1> object_image_loc;
    object_image_loc( 0, 0 ) = (ib.min_x() + ib.max_x()) / 2.0;
    object_image_loc( 1, 0 ) = (ib.min_y() + ib.max_y()) / 2.0;
    object_image_loc( 2, 0 ) = 1.0;
    it->interp_image_location = object_image_loc.get_column( 0 );

    // map position back to world for velocity calculations
    vnl_matrix_fixed<double, 3, 1> world_object_loc =
      interp_src_to_world.get_matrix() *
      object_image_loc;
    world_object_loc(0, 0) /= world_object_loc(2, 0);
    world_object_loc(1, 0) /= world_object_loc(2, 0);
    it->interp_world_location = world_object_loc.get_column( 0 );
  }

  // now we can go back and compute velocities-- no smoothing,
  // just simple differencing
  it = mem.begin(); // reset it to a0
  vcl_pair< vnl_vector_fixed<double, 3>, timestamp > last_loc;
  last_loc.first = it->state_at_ts->loc_;
  last_loc.second = it->ts;
  while ( ++it != anchor1 )  // skip a0 and a1
  {
    double dt =
      (it->ts.time() - last_loc.second.time()) * this->parent->get_timestamp_to_secs();
    it->interp_world_velocity =
      (dt == 0.0)
      ? vnl_vector_fixed<double, 3>( 0.0 )
      : (it->interp_world_location - last_loc.first) / dt;

    // update last_loc
    last_loc.first = it->interp_world_location;
    last_loc.second = it->ts;
  }

  //
  // Now we have all the interpolated fields set in the track interpolation records
  // for this interval.
  //
  // Insert the newly interpolated states into the history cache.
  //

  it = mem.begin();
  while (++it != anchor1)
  {
    this->parent->get_history_cache().add_interpolated_state( it );
  }

  // all done!
}

//
// The track_memory contains interpolated data for the track;
// use the frame-number-to-gap-packet map to slip the interpolated
// states into the tracks pointed to by the gap packets.
//
// One subtlety is that we need to re-write the history of every
// track in the gap list, because we don't have the track views
// that Linus is using.  (First, make it work...)
// However, it shouldn't be hard-- we should be able to just append
// on to the existing track histories.

void
track_interpolation_manager
::move_interpolated_states_into_a_track( unsigned track_id, track_memory_type& mem )
{

  track_memory_type_it it = mem.begin(); // points to a0, which should require no rewriting
  track_memory_type_it anchor1 = mem.end();
  --anchor1;
  while (++it != anchor1)
  {
    process_packet_type& p = this->parent->get_pending_packet( it->ts.frame_number() );

    track_sptr trk = 0;
    for (unsigned j=0; j<p.tl.size(); ++j)
    {
      if (p.tl[j]->id() == track_id) trk = p.tl[j];
    }
    if ( trk == 0 )
    {
      trk = it->trk;
      p.tl.push_back(trk);
      log_info( "ITSP: Adding clone of track " << trk->id()
                << " to packet at frame " << it->ts.frame_number() << "\n");
    }

    vcl_vector< track_state_sptr> dst_h;
    this->parent->get_history_cache().load_track_history_up_to_frame( track_id, p.ts.frame_number(), dst_h );
    trk->reset_history( dst_h );
  }

  // all done
}

//
// external -> internal passthroughs
//

interpolate_track_state_process
::interpolate_track_state_process( const vcl_string& name )
  : process( name, "interpolate_track_state_process" )
{
  this->p = new interpolate_track_state_process_impl( this );
  this->reset();
}

interpolate_track_state_process
::~interpolate_track_state_process()
{
  delete this->p;
}

config_block
interpolate_track_state_process::params() const { return p->params(); }

bool
interpolate_track_state_process::reset() { return p->reset(); }

bool
interpolate_track_state_process::set_params( const config_block& blk ) { return p->set_params( blk ); }

bool
interpolate_track_state_process::initialize() { return p->initialize(); }

process::step_status
interpolate_track_state_process::step2() { return p->step2(); }

void
interpolate_track_state_process::set_input_tracks( const track_vector_t& tl ) { p->set_input_tracks( tl ); }

void
interpolate_track_state_process::set_input_img2ref_homography( const image_to_image_homography& hi ) { p->set_input_img2ref_homography( hi ); }

void
interpolate_track_state_process::set_input_ref2world_homography( const image_to_utm_homography& hr ) { p->set_input_ref2world_homography( hr ); }

void
interpolate_track_state_process::set_input_timestamps( const vcl_vector<timestamp>& tsv ) { p->set_input_timestamps( tsv ); }

void
interpolate_track_state_process::set_input_gsd( double gsd ) { p->set_input_gsd( gsd ); }

const track_vector_t&
interpolate_track_state_process::output_track_list() const { return p->output_track_list(); }

const timestamp&
interpolate_track_state_process::output_timestamp() const { return p->output_timestamp(); }

const image_to_image_homography&
interpolate_track_state_process::output_img2ref_homography() const { return p->output_img2ref_homography(); }

const image_to_utm_homography&
interpolate_track_state_process::output_ref2utm_homography() const { return p->output_ref2utm_homography(); }

const double&
interpolate_track_state_process::output_gsd() const { return p->output_gsd(); }

//
// Actual implementation code
//


// helper routine to clone track list
// probably overkill-- don't need to clone e.g. image objects?

track_vector_t
clone_track_list( const track_vector_t& tl )
{
  track_vector_t ret;
  for (unsigned i=0; i<tl.size(); ++i)
  {
    ret.push_back( tl[i]->clone() );
  }
  return ret;
}

track_state_sptr
track_interpolation_record_type
::synthesize_track_state()
{
  track_state_sptr s = new track_state();
  s->loc_ = this->interp_world_location;
  s->vel_ = this->interp_world_velocity;
  s->time_ = this->ts;
  image_object_sptr obj = new image_object;
  obj->img_loc_[0] = this->interp_image_location[0];
  obj->img_loc_[1] = this->interp_image_location[1];
  obj->bbox_ = this->interp_image_box;
  obj->area_ = this->interp_image_box.area();
  s->data_.set( tracking_keys::img_objs, vcl_vector< image_object_sptr >(1, obj));
  s->amhi_bbox_ = this->interp_image_box;
  return s;
}

vcl_vector< process_packet_type >
itsp_inputs
::to_packets( const history_cache_type& hc, size_t live_index )
{
  // Given the pipeline step inputs, create the stream of packets to be
  // processed.  Associate the input homography and tracklist with the
  // timestamp at live_index (or last value if live_index is out-of-bounds.)
  //
  // The only trick here is that all packets created out pipeline inputs
  // contain tracks whose track_sptrs are clones, not further shares of the sptr.
  // Since the track history will change as interpolated states are added,
  // don't change the tracks underneath other nodes in the pipeline but instead
  // mutate copies of them.
  //
  // Give everybody clones of the "live packet"'s track list, because tracks
  // can neither be added nor destroyed within an upsampling window
  //

  vcl_vector< process_packet_type > ret;
  if ( this->tsv.empty() ) return ret;

  size_t match_index =
    ( live_index >= this->tsv.size() )
    ? this->tsv.size() - 1
    : live_index;

  for (size_t i=0; i<this->tsv.size(); ++i)
  {
    if ( i == match_index)
    {
      // this packet's completeness is entirely up to the tracks.
      ret.push_back(
        process_packet_type(
          hc,
          clone_track_list( this->tracks ),
          this->tsv[i],
          this->img2ref_h,
          this->gsd
          ));
      log_info( "ITSP: Input packet " << i << " " << this->tsv[i] << " w/ " << this->tracks.size() << " tracks (match)\n" );
    }
    else
    {
      // this packet, for upsampling, cannot be complete.
      ret.push_back(
        process_packet_type(
          this->tsv[i],
          this->gsd
          ));
      log_info( "ITSP: Input packet " << i << " " << this->tsv[i] << "\n" );
    }
  }

  return ret;
}

interpolate_track_state_process_impl
::interpolate_track_state_process_impl( interpolate_track_state_process* p )
  : parent( p ),
    h_manager( this ),
    t_manager( this )
{
  this->reset();
}

bool
interpolate_track_state_process_impl
::reset()
{
  this->inp.reset_flag();
  this->pending_packet_list.clear();
  this->frame_to_pending_packet_index_map.clear();
  this->t_manager.reset();
  this->h_manager.reset();
  this->first_pump = true;
  return true;
}

process::step_status
interpolate_track_state_process_impl
::step2()
{
  this->first_pump = true;

  bool all_inputs_present = this->inp.all_inputs_present();
  this->inp.reset_flag();

  //
  // If inputs failed, and we're enabled, flush pending packets and return false.
  //
  if ( ! all_inputs_present )
  {
    if ( pblk.enabled )
    {
      this->mark_all_pending_as_tracking_complete();
      this->emit_and_clear_output_packets();
      // one extra push because we're about to return failure
      this->pump_outputs();
    }
    return process::FAILURE;
  }

  //
  // If inputs were okay but we're not enabled, just copy input to output and return
  //

  if ( ! pblk.enabled ) return this->copy_inputs_to_outputs();

  //
  // We have all our inputs, and we're on-line.  Onwards!
  //

  // synchronize our history cache with active track list
  this->history_cache.synchronize( inp.tracks );

  // convert our pipeline inputs into our packet set
  vcl_vector< process_packet_type > packets =
    this->inp.to_packets( this->history_cache, pblk.tsv_live_index );

  // process each packet
  // record if any packets were output; if not, SKIP, otherwise, SUCCESS

  unsigned output_packet_count = 0;
  for (size_t pindex=0; pindex<packets.size(); ++pindex)
  {
    output_packet_count += this->process_packet( packets[pindex] );
  }

  // all done
  return
    (output_packet_count == 0)
    ? process::SKIP
    : process::SUCCESS;
}

//
// Each packet is a single timestamp, with a track list and a
// homography.
//
// We can (and should) interpolate homographies even if we're
// still pending on incomplete track packets.
// Therefore, our anchor0-gap-anchor1-interpolate loop
// needs to happen for both homographies and track lists.
// Hmm.

// Semantically, a packet is "complete" if all the tracks are
// defined on the timestamp and the homography is valid; it is
// incomplete if the homography is invalid, or there is a track
// undefined on the timestamp.  (Empty track lists are always valid.)
//
// Note that a packet is incomplete if ANY active track is not
// defined for a timestamp.
//
// process_packet() examines the packet, and selects one of the following
// states ("dt" is the difference between the anchor-0 timestamp and the packet
// timestamp):
//
// 0: incomplete and no anchor-0: emits and returns
// 1: complete and no anchor-0: sets as anchor-0, emits, and returns
// 2: complete, anchor-0 is set, and empty gap list: resets as anchor-0,
//    emits, and returns
// 3: incomplete, anchor-0 is set, and dt < some_limit: add to gap list, do not
//    emit, return
// 4: incomplete, anchor-0 is set, and dt >= some_limit: flush gap list,
//    emit, reset anchor-0, return
// 5: complete, anchor-0 is set, non-empty gap list: interpolate across
//    gap list, emit, set anchor-0 to this, return

unsigned
interpolate_track_state_process_impl
::process_packet( process_packet_type& p )
{

  // add the packet to the pending list
  this->pending_packet_list.push_back( p );
  this->frame_to_pending_packet_index_map[ p.ts.frame_number() ] =
    this->pending_packet_list.size()-1;

  // introduce the packet to the interpolation managers
  interpolation_manager::interp_status h_stat =
    this->h_manager.get_and_record_status( p );
  interpolation_manager::interp_status t_stat =
    this->t_manager.get_and_record_status( p );

  log_info( "ITSP: itsp-state " << p.ts.frame_number()
            << " h-stat " << interpolation_manager::state2str( h_stat )
            << " t-stat " << interpolation_manager::state2str( t_stat )
            << "\n" );

  // If *both* managers return nil, flush and return.
  if ( ( h_stat == interpolation_manager::NIL ) &&
       ( t_stat == interpolation_manager::NIL ))
  {
    log_info( "ITSP: itsp-state " << p.ts.frame_number()
              << " in reset state; flushing\n" );
    this->mark_all_pending_as_tracking_complete();
    return this->emit_and_clear_output_packets();
  }

  // If the homography manager can interpolate, do so.
  // do NOT flush-- only flush when tracks are ready.
  // But we must interpolate any homographies before
  // we interpolate tracks.

  if ( h_stat == interpolation_manager::ANCHOR_1 )
  {
    log_info( "ITSP: itsp-state " << p.ts.frame_number()
              << " interpolating homographies...\n" );
    this->h_manager.interpolate( this->inp );
  }

  // If the track manager can interpolate, do so and flush.
  if ( t_stat == interpolation_manager::ANCHOR_1 )
  {
    log_info( "ITSP: itsp-state " << p.ts.frame_number()
              << " interpolating tracks...\n" );
    this->t_manager.interpolate( this->inp );
    return this->emit_and_clear_output_packets();
  }

  // If we're still here, then either homographies or tracks are
  // in an anchor-0/gap state.  Check for timeout.

  bool gap_list_within_time_limit = this->gap_time_limit_okay( p );

  if ( ! gap_list_within_time_limit )
  {
    log_info( "ITSP: itsp-state " << p.ts.frame_number()
              << " :: timeout- flushing\n" );
    this->mark_all_pending_as_tracking_complete();
    return this->emit_and_clear_output_packets();
    this->h_manager.reset();
    this->t_manager.reset();
  }

  // No timeouts, just filling out queue... return 0.
  log_info( "ITSP: itsp-state " << p.ts.frame_number()
            << " :: adding to queue (size now "
            << this->pending_packet_list.size() << ")\n" );
  return 0;
}

int
interpolate_track_state_process_impl
::emit_and_clear_output_packets()
{
  vcl_vector< process_packet_type > unready_for_output;
  vcl_map< unsigned, unsigned > unready_index_map;

  unsigned sent_count = 0;
  for (unsigned i=0; i<this->pending_packet_list.size(); ++i)
  {
    const process_packet_type& p = this->pending_packet_list[i];
      log_info( "ITSP: interpolate track report: frame " << p.ts.frame_number() << " ; "
                << p.tl.size() << " tracks:\n" );
    if (p.output_status == process_packet_type::OUTPUT_READY)
    {
      this->pump_outputs();

      this->out_track_list = p.tl;
      this->out_timestamp = p.ts;
      this->out_homography = p.h;
      this->out_gsd = p.gsd;
      sent_count++;

      for (unsigned dbg = 0; dbg < p.tl.size(); ++dbg )
      {
        track_sptr trk = p.tl[dbg];
        log_info( "ITSP: ... index " << dbg << " id " << trk->id()
                  << " with " << trk->history().size() << " states; last: "
                  << trk->last_state()->time_.frame_number() << " loc " << trk->last_state()->loc_ << "\n" );
      }
    }
    else
    {
      log_info( "ITSP: ... retained; output status " << p.output_status << "\n" );
      unready_for_output.push_back( p );
      unready_index_map[ p.ts.frame_number() ] = unready_for_output.size()-1;
    }
  } // end for

  log_info( "ITSP: emit-and-clear sent " << sent_count << " and kept " << unready_for_output.size() << "\n" );
  this->pending_packet_list = unready_for_output;
  this->frame_to_pending_packet_index_map = unready_index_map;

  return sent_count;
}



} // namespace vidtk
