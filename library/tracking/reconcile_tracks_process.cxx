/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/reconcile_tracks_process.h>

#include <tracking/tracking_keys.h>
#include <tracking/reconcile_tracks_algo.h>

#include <utilities/ring_buffer.txx>
#include <utilities/unchecked_return_value.h>
#include <vcl_algorithm.h>
#include <vcl_ostream.h>
#include <vcl_sstream.h>
#include <boost/foreach.hpp>
#include <vcl_fstream.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER("reconcile_tracks_process");

namespace {

static vcl_ostream& FormatStates(vcl_ostream& str, const vidtk::track_sptr& obj)
{
  vcl_vector< track_state_sptr > const& hist = obj->history();

  for (unsigned int i = 0; i < hist.size(); i++)
  {
    str << "State " << i << ": " << hist[i]->time_
    << vcl_endl;
  }

  return ( str );
}


static vcl_ostream& operator << ( vcl_ostream & str, const vidtk::track_sptr & obj )
{
  str << "track (id: " << obj->id()
  << "   history from " << obj->first_state()->time_
  << " to " << obj->last_state()->time_
  << "  size: " << obj->history().size() << " states."
  << vcl_endl;

  return ( str );
}

static vcl_ostream& operator << (vcl_ostream& str, reconciler::track_map_key_t const& datum)
{
  str <<  "{" << datum.track_id
      << ", " << datum.tracker_type
      << ", " << datum.tracker_subtype
      << "}";

  return str;
}

static vcl_ostream& operator << (vcl_ostream& str, reconciler::track_map_data_t const& datum)
{
    str << "["
        << (datum.is_dropped() ? "D" : " ")
        << (datum.is_processed() ? "P" : " ")
        << (datum.is_generated() ? "G" : " ")
        << (datum.is_terminated() ? "T" : " ")
        << "] last seen at: " << (datum.last_seen())
        << " " << (datum.track());

  return ( str );
}


static vcl_ostream& FormatLUT(vcl_ostream& str, reconciler::track_map_t const& map)
{
  str << "---- LUT Contents ----\n";

  reconciler::track_map_t::const_iterator it;
  for (it = map.begin();
       it != map.end();
       it++)
  {
    str << (it->first)
        << " "
        << (it->second);
  }

  return ( str );
}


// ----------------------------------------------------------------
/** Find timestamp in a track for a frame number.
 *
 * Returns the frame index for the state that is less than the
 * specified frame number.  This is designed for trimming track
 * histories.
 */
static int
find_frame (unsigned frame, const vidtk::track_sptr ptr)
{
  vcl_vector< vidtk::track_state_sptr > const& hist (ptr->history());
  for (unsigned i = 1; i < hist.size(); i++)
  {
    if (hist[i]->time_.frame_number() >= frame)
    {
      return i-1;
    }
  }

  // could be -1 if no states.
  return hist.size()-1;
}


// ================================================================
/** Set of synchronized inputs/outputs
 *
 * This class represents a set of synchronized inputs that move
 * through the reconcilation process together. The actual set of inputs
 * is defined in the RECONCILER_DATUM_LIST macro.
 */
  class InputSet
  {
  public:
    // -- TYPES --
    // bit mask for inputs that are present
    enum {
#define DATUM_DEF(T, N, I) N ## _present,
      RECONCILER_DATUM_LIST
#undef DATUM_DEF

      NUMBER_OF_INPUTS
    };


    InputSet() : m_inputCount(0), m_inputMask(0)  { }
    ~InputSet() { }

    // Reset to initialized state
    void Reset()
    {
      m_inputCount = 0;
      m_inputMask = 0;

      // Use datum list to generate initializers
#define DATUM_DEF(T, N, I) m_ ## N = I;
  RECONCILER_DATUM_LIST // macro magic
#undef DATUM_DEF

    } // end Reset()


//
// Define the accessors for data elements
//
#define DATUM_DEF(T,N,I)                                \
  public:                                               \
    T const& N() const { return ( this->m_ ## N ); }    \
    T & N() { return ( this->m_ ## N ); }               \
  InputSet & N(T const& val) { this->m_ ## N = val;     \
    this->m_inputCount++;                               \
    this->m_inputMask |= (1 << N ## _present);          \
    return ( *this ); }                                 \
  void reset_ ## N() { m_ ## N = I; }                   \
  private:                                              \
    T m_ ## N;                                          \
  public:

    RECONCILER_DATUM_LIST // macro magic

#undef DATUM_DEF


    int InputCount() const { return this->m_inputCount; }

    // this really should use a bitmask approach to determining that the required
    // parameters are present.
    bool InputsValid() const { return (this->m_inputCount >= (2)); }

    vcl_ostream & InputsPresent (vcl_ostream & str) const
    {
#define DATUM_DEF(T, N, V)                                              \
      str << # N;                                                       \
      if ( ! (m_inputMask & (1 << N ## _present)) )   str << " not";    \
      str << " present, ";

      RECONCILER_DATUM_LIST // macro magic

#undef DATUM_DEF

      return (str);
    }


  protected:


  private:

    int m_inputCount;
    unsigned m_inputMask;

  }; // end class InputSet

} // end namespace

// instantiate our custom ring buffer
template class vidtk::ring_buffer < InputSet >;


// Define our operational states.
enum { ST_RUNNING = 1, ST_FLUSHING };



// ================================================================
/** Reconcile output implementation
 *
 * This impl class is clever, but does nothing for me when I want to
 * allow for sub-clssing.  Even if I were to put the impl class in
 * separate files, I would need to write a new process class to use a
 * sub-class of the impl.  Sub-clssing is easier if the process can
 * expose methods that can be specialized.
 */
class reconciler::reconcile_tracks_process_impl
{
public:

  vidtk::process::step_status step_running();
  vidtk::process::step_status step_flushing();
  vidtk::process::step_status reconcile_tracks(int idx);
  void reset_outputs();
  void reset_inputs();

  // Configuration parameters
  config_block config;
  unsigned m_delayFrames;         // used to reset buffer index

  // These manage the passthrough data items
  InputSet m_currentInputs;
  InputSet m_currentOutputs;

  // Computed Outputs
  vidtk::track_vector_t  out_terminated_tracks;


  // Buffer of synchronized data
  vidtk::ring_buffer < InputSet > m_inputBuffer;


  /** Active tracks lookup table (LUT). This LUT (map) holds tracks
   * and is indexed by track ID.  It holds tracks that have been
   * detected but not yet terminated. When a new track is received as
   * input with an Id that is already in the LUT, the old one is
   * replaced.
   */
  typedef track_map_t::iterator iterator_t;
  track_map_t m_activeTracksLUT;

  unsigned m_highWaterMark;

  int m_state; // current operating state
  int m_refBufferIdx; // current reconciling buffer index

  reconcile_tracks_process * m_base;
  int m_verbose;
  bool m_enabled;

  reconciler::reconciler_delegate * m_delegate;
  vidtk::reconciler::simple_track_reconciler::param_set m_params;

  bool IsVerbose(int i) const { return ( m_verbose >= i ); }


// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
reconcile_tracks_process_impl(reconcile_tracks_process * b)
    : m_delayFrames(0),
      m_highWaterMark(0),
      m_state(ST_RUNNING),
      m_refBufferIdx(0),
      m_base(b),
      m_verbose(0),
      m_enabled(false),
      m_delegate(0)
  {
    // supply default implementation.
    m_delegate = new reconciler::reconciler_delegate();

    reset();
  }


// ----------------------------------------------------------------
  ~reconcile_tracks_process_impl()
  {
    delete m_delegate;

    LOG_INFO (m_base->name() << ": Max size of LUT: " << m_highWaterMark);
  }


// ----------------------------------------------------------------
/** Reset implementation data areas.
 *
 * When a reset is received, the process is returned to its initial state.
 */
  bool reset()
  {
    m_refBufferIdx = m_delayFrames - 1; // set index from size
    m_state = ST_RUNNING;

    // clear input buffers
    m_inputBuffer.clear();
    m_inputBuffer.set_capacity(m_delayFrames);

    // clear LUT
    this->m_activeTracksLUT.clear();

    reset_outputs();
    reset_inputs();

    return true;
  }


// ----------------------------------------------------------------
/** Add track to LUT
 *
 * This method adds the supplied track to our LUT based on track ID.
 * If there was a track with that ID there before, it is replaced.
 * The maximum number of tracks (high water mark) is also updated.
 *
 * @param[in] trk - track to add to LUT
 */
  void AddTrack(track_sptr const& trk)
  {
    unsigned tracker_type(0);
    unsigned tracker_subtype(0);


    if ( trk->data().has( vidtk::tracking_keys::tracker_type) )
    {
      trk->data().get( vidtk::tracking_keys::tracker_type, tracker_type );
    }

    if ( trk->data().has( vidtk::tracking_keys::tracker_subtype ) )
    {
      trk->data().get( vidtk::tracking_keys::tracker_subtype, tracker_subtype );
    }

    reconciler::track_map_key_t key(trk->id(), tracker_type, tracker_subtype);

    // check to see if track is already in the table
    if (this->m_activeTracksLUT.find (key) != this->m_activeTracksLUT.end() )
    {
      // Update existing entry if it is not dropped.  If it is
      // dropped, the existing state is maintained because it may be
      // pending finalization.  In any event, a dropped track will
      // never be generated as an active track again, so it does not
      // matter that we do not update this track.
      if ( ! this->m_activeTracksLUT[key].is_dropped() )
      {
        this->m_activeTracksLUT[key].set_track(trk);
      }

      // Always update track last seen
      this->m_activeTracksLUT[key].set_last_seen(this->m_currentInputs.timestamp() );
    }
    else
    {
      reconciler::map_value_t val (key, track_map_data_t(trk, this->m_currentInputs.timestamp() ) );
      this->m_activeTracksLUT.insert ( val );
    }

    // Track high water mark
    if (this->m_activeTracksLUT.size() > m_highWaterMark)
    {
      m_highWaterMark = this->m_activeTracksLUT.size();
    }

    if (IsVerbose(2))
    {
      LOG_INFO ( m_base->name() << ": Added to LUT: " << key
                 << " " << this->m_activeTracksLUT[key]);

      if (IsVerbose(3))
      {
        vcl_stringstream stuff;
        FormatLUT(stuff, this->m_activeTracksLUT);
        LOG_INFO (stuff.str() );
      }
    } // end verbose
  }


  // ----------------------------------------------------------------
  /** Generate active track views.
   *
   * This method generates the relevant view on all currently active
   * tracks.
   *
   * @param[in] tts - target timestamp
   * @return True if all processed o.k., False otherwise.
   */
  bool generate_active_tracks(const vidtk::timestamp& tts)
  {
    // the target timestamp is tts

    iterator_t ix;
    for (ix = this->m_activeTracksLUT.begin(); ix != this->m_activeTracksLUT.end(); /* blank */)
    {
      // Check track (ix.second.track()) last history state timestamp.
      // If that TS is < target_ts, then the last history entry is
      // older than what we are generating, delete this entry.
      if ( (ix->second.track()->last_state()->time_ < tts)
           && ix->second.is_generated()
           && ( ! ix->second.is_terminated() ) )
      {
        // Only terminate generated and non-terminated tracks here.
        LOG_INFO (m_base->name() << ": Terminating track - " << ix->first << " " << ix->second);
        out_terminated_tracks.push_back ( ix->second.track() );

        ix->second.set_terminated();

        ix++;
        continue;
      }

      // If this track has not been seen for a long time, then delete it
      if ( ix->second.last_seen() < tts)
      {
        if (IsVerbose(2))
        {
          LOG_INFO( m_base->name() << ": Removing track from LUT @ target-" << tts
                    << " - " << ix->second);
        }
        // This preserves the validity of the iterator through the
        // erase.
        iterator_t dix = ix;
        ix++; // preserve validity of iterator

        this->m_activeTracksLUT.erase(dix);

        continue;
      }

      if ( (ix->second.track()->first_state()->time_ <= tts)
           && ( ! ix->second.is_dropped() ) )
      {
        // Now we have a track view to generate
        ix->second.set_generated();
        track_sptr trk = ix->second.track();
        track_view::sptr tv = track_view::create(trk);

        tv->set_view_bounds(trk->first_state()->time_, tts);

        // This may be considered as a little wasteful, in that we are
        // creating a full track object where just a lightweight view
        // will suffice.
        trk = *tv; // convert to track
        this->m_currentOutputs.tracks().push_back(trk);

        if (IsVerbose(2))
        {
          LOG_INFO( m_base->name() << ": generate_active_tracks: Reconciled track @ target-"
                    << tts << " - " << trk);
        }

        ix++;
        continue;
      }

      // not in the window
      ix++;

    } // end for

    return ( true );
  } /* generate_active_tracks */

}; // end class reconciler::reconcile_tracks_process_impl

//================================================================
//================================================================

// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
reconcile_tracks_process::
  reconcile_tracks_process(vcl_string const& name)
    : process(name, "reconcile_tracks_process"),
      m_impl(0)
{
  m_impl = new reconciler::reconcile_tracks_process_impl(this);

  m_impl->config.add_parameter(
    "enabled",
    "false",
    "Turn the module off and pass inputs to outputs." );

  m_impl->config.add_parameter(
    "delay_frames",
    "10",
    "Number of frames to dealy before reconciling tracks.");

  m_impl->config.add_parameter(
    "verbose",
    "0",
    "Controls the amount of diagnostic output generated by this process. "
    "0 - no output, 1 - mostly per frame output, 2 - per track output, "
    "3 - per state output (larger numbers mean more output).");

  // reconciler algo process
  m_impl->config.add_parameter(
    "min_track_size",
    "8",
    "Minimum track size to compare.  If either track does not have "
    "enough states, then it will not be comapred.");

  m_impl->config.add_parameter(
    "overlap_fraction",
    "0.50",
    "Box overlap fraction. If the boxes overlap more than the "
    "specified percentage, then they are considered identical."
    "Overlap fraction is calculated (A n B) / A   and (A n B) / B");

  m_impl->config.add_parameter(
    "state_count_fraction",
    "0.50",
    "Fraction of all state that overlap (by overlap fraction, "
    "above) for tracks to be considered similar.");
}


reconcile_tracks_process::
  ~reconcile_tracks_process()
{
  delete m_impl;
}


// ================================================================
// Config related methods.

config_block reconcile_tracks_process::
  params() const
{
  return ( m_impl->config );
}


// ----------------------------------------------------------------
bool reconcile_tracks_process::
  set_params(config_block const& blk)
{
  bool retcode(true);

  try
  {
    blk.get("enabled", m_impl->m_enabled );
    blk.get("delay_frames", m_impl->m_delayFrames );
    blk.get("verbose", m_impl->m_verbose );

    // These parameters are really part of the algo delegate class
    blk.get("min_track_size", m_impl->m_params.min_track_size );
    blk.get("overlap_fraction", m_impl->m_params.overlap_fraction );
    blk.get("state_count_fraction", m_impl->m_params.state_count_fraction );
  }
  catch(unchecked_return_value & e)
  {
    LOG_ERROR(this->name() << ": couldn't set parameters: " << e.what());

    // reset to old values
    this->set_params(m_impl->config);
    return ( false );
  }

  m_impl->config.update(blk);

  if (m_impl->m_delayFrames <= 0)
  {
    LOG_ERROR ( name() << "delay_frames (" << m_impl->m_delayFrames << ") must be greater than zero");
    retcode = false;
  }

  if (m_impl->m_params.min_track_size  < 1)
  {
    LOG_ERROR(this->name() << ": min_track_size must be greater than one");
    retcode = false;
  }

  if (m_impl->m_params.min_track_size > m_impl->m_delayFrames)
  {
    LOG_ERROR ( name() <<  "min_track_size (" << m_impl->m_params.min_track_size
                << ") must be less than delay_frames (" << m_impl->m_delayFrames << ")");
    retcode = false;
  }

  // create algorithm
  set_reconciler_delegate ( new vidtk::reconciler::simple_track_reconciler(m_impl->m_params) );

  return retcode;;
}


// ----------------------------------------------------------------
/** process initialize.
 *
 */
bool reconcile_tracks_process::
  initialize()
{

  return m_impl->reset();
}


// ----------------------------------------------------------------
/** Reset this process.
 *
 * This method is called when the pipeline issues a reset.
 * In this case, the reset is passed to the implementation method.
 *
 */
bool reconcile_tracks_process::
reset()
{
  if (m_impl->IsVerbose(1))
  {
    LOG_INFO ( name() << "::reset() called");
  }

  return m_impl->reset();  // reset implementation
}


// ----------------------------------------------------------------
/** Main process step.
 *
 *
 */
vidtk::process::step_status
reconcile_tracks_process::
step2()
{
  // The input count is used to determine if we have reached end of
  // input.  Normally we will receive all inputs. We will receive
  // none of them if our predecessor in the pipeline has reached end
  // of input.
  bool input_valid ( m_impl->m_currentInputs.InputsValid());

  if (! m_impl->m_enabled)
  {
    if( !input_valid )
    {
      return vidtk::process::FAILURE;
    }

    // copy inputs to outputs

    // There are two schools of thought about what outputs to produce.
    //
    // 1) Produce no outputs so it becomes obvious that the reconciler
    // is not enabled in cases where it is needed.
    //
    // 2) A process should behave according to some
    // specification. Meaning that it has a known set ov inputs and
    // outputs.  It should produce outputs if in consumes inputs.
    //
    // Whichever approach is chosen, it may take some time (hours to
    // days) to realize that the enable flag is set the wrong way.
    //
    m_impl->m_currentOutputs = m_impl->m_currentInputs;

    m_impl->reset_inputs();

    // Does not generate terminated tracks reconciled tracks or
    // reconciled track views when disabled
    return vidtk::process::SUCCESS;
  }

  LOG_DEBUG (this->name() << ": input at " << m_impl->m_currentInputs.timestamp()
             << " input count: " << m_impl->m_currentInputs.InputCount()
             << "  active track count: " << m_impl->m_currentInputs.tracks().size());

  // Test what state we are in.
  if (m_impl->m_state == ST_RUNNING)
  {
    // Check to see if we received any new tracks. If we have all
    // inputs required, then process new input.
    if (input_valid)
    {
      if (m_impl->IsVerbose(2))
      {
        vcl_stringstream stuff;

        m_impl->m_currentInputs.InputsPresent(stuff);
        LOG_DEBUG (this->name() << ": Inputs: " << stuff.str());
      }

      // If there are inputs but the timestamp is invalid, then skip
      // until we get a better set of inputs.
      if (! m_impl->m_currentInputs.timestamp().is_valid() )
      {
        m_impl->reset_inputs();
        return vidtk::process::SKIP;
      }

      return ( m_impl->step_running() );
    }

    // end if input detected, switch to flushing and flush one step.
    m_impl->m_state = ST_FLUSHING;

    // Set the start buffer index for flushing based on the minimum of
    // current buffer size and buffer capacity.  This handles the case
    // where the buffer is not full when we switch to flushing mode.
    m_impl->m_refBufferIdx = vcl_min(m_impl->m_inputBuffer.size() - 1,
                                     m_impl->m_delayFrames - 1 );

    LOG_INFO(this->name() << ": Switching to flushing state");

    return ( m_impl->step_flushing() );
  }
  else if (m_impl->m_state == ST_FLUSHING)
  {
    return ( m_impl->step_flushing() );
  }
  else
  {
    // unexpected state
    LOG_ERROR(this->name() << ": Unexpected state (" << m_impl->m_state <<  ")");
  }

  return ( vidtk::process::FAILURE );
} /* step */


// ----------------------------------------------------------------
/** Standard running mode processing.
 *
 *
 */
vidtk::process::step_status
reconciler::reconcile_tracks_process_impl::
step_running()
{
  reset_outputs();

  // push our inputs to the ring buffer
  this->m_inputBuffer.insert ( this->m_currentInputs );

  // Add input tracks to our master track list.
  BOOST_FOREACH (const track_sptr ptr, this->m_currentInputs.tracks())
  {
    this->AddTrack(ptr);
  } // end foreach

  if (IsVerbose(2))
  {
    LOG_INFO ( m_base->name() << ": LUT usage:  max: " << this->m_highWaterMark
               << "  current: " << this->m_activeTracksLUT.size());
  }

  if (IsVerbose(1))
  {
    LOG_INFO ( m_base->name() << ": buffering " << this->m_inputBuffer.size()
               << "/" << (this->m_refBufferIdx + 1));
  }

  // Reset all inputs - safe to do since they have been pushed into a buffer
  reset_inputs();

  // Find the index of the reference timestamp.  This value is from
  // the configuration and determines how many frames to buffer.

  // If we have not buffered that many frames, just return.  We must
  // have a full buffer before we can reconcile any tracks.
  if ( static_cast< unsigned int >(this->m_refBufferIdx) >= this->m_inputBuffer.size() )
  {
    return ( vidtk::process::SKIP );
  }

  // We now have enough frames buffered to start reconciling tracks.
  return ( reconcile_tracks(this->m_refBufferIdx) );
} /* step_running */


// ----------------------------------------------------------------
/** Flushing mode processing.
 *
 * This method flushes the oldest set of tracks and other inputs to
 * the output ports.
 */
vidtk::process::step_status
reconciler::reconcile_tracks_process_impl::
step_flushing()
{
  vidtk::process::step_status retcode;

  reset_outputs();

  // Find the index of the reference timestamp.  Since there has been
  // no new inputs, we start working the input queues from the back to
  // the front, decrementing the reference index until we consume all
  // the buffered data.  When the index goes negative, all elements
  // have been consumed and we terminate with a false return.
  //
  int ref_idx = --this->m_refBufferIdx;

  if (IsVerbose(1))
  {
    LOG_INFO(m_base->name() << ": Flushing index " << ref_idx);
  }

  if (ref_idx < 0)
  {
    LOG_INFO(m_base->name() << ": Flushing complete - process resetting.");
    reset(); // reset to buffering state
    return ( vidtk::process::FAILURE);
  }

  retcode = reconcile_tracks(ref_idx);

  // If this is the last iteration of the flushing operation, then we
  // have to "reconcile" these tracks. We need to do it at this point
  // because all reconciled tracks have been generated (and all tracks
  // deleted from the LUT). This step must return with a SUCCESS if we
  // terminate any tracks. We can not do this on the last iteration
  // which returns SKIP.
  if (ref_idx == 0) // last track
  {
    if (this->m_activeTracksLUT.size()  > 0)
    {
      retcode = vidtk::process::SUCCESS;
    }

    LOG_INFO (m_base->name() << ": size of LUT to terminate: " << this->m_activeTracksLUT.size());

    // Make sure all tracks remaining in the LUT get terminated
    BOOST_FOREACH (track_map_t::value_type trk, this->m_activeTracksLUT)
    {
      if ( trk.second.is_generated()
           && ( ! trk.second.is_dropped())
           && ( ! trk.second.is_terminated()) )
      {
        LOG_INFO (m_base->name() << ": Terminating track - " << trk.first << " " << trk.second);
        out_terminated_tracks.push_back (trk.second.track());
      }

    }
  }

  return ( retcode );
}


// ----------------------------------------------------------------
/** Set reconciler deletage object.
 *
 *
 */
void reconcile_tracks_process::
set_reconciler_delegate (reconciler::reconciler_delegate * del)
{
  delete m_impl->m_delegate;
  m_impl->m_delegate = del;
}


// ----------------------------------------------------------------
/** Perform reconciling process
 *
 * This method reconciles all known tracks based on the index provided.
 * The supplied index is used to select a stripe of consistent data
 * values from our input buffers adn generate output tracks based on
 * the timestamp retrieved.
 *
 * An output track will be generated for each known track that has a
 * state at or before the target time stamp.
 *
 * @param ref_idx - buffer reference index
 */
vidtk::process::step_status
reconciler::reconcile_tracks_process_impl::
reconcile_tracks(int ref_idx)
{
  vidtk::track_vector_t terminated_tracks; // local set of tracks
  vidtk::timestamp tts = this->m_inputBuffer.datum_at(0).timestamp(); // most recent timestamp

  m_delegate->process_track_map (tts, this->m_activeTracksLUT, &terminated_tracks);

  // Generate terminated tracks up to, but not including, current
  // tts. The termianted tracks vector points back to the tracks in
  // the LUT.
  //
  // These tracks are valid as terminated tracks up to, but not
  // including the TTS. That is why we have to trim off the state with
  // the TTS and all following states.
  BOOST_FOREACH (track_sptr ptr, terminated_tracks)
  {
    int frame = find_frame (tts.frame_number(), ptr);
    if (frame > 0)
    {
      // make track[frame] the last frame in the track
      vidtk::timestamp end_ts = ptr->history()[frame]->time_;
      ptr->truncate_history (end_ts);
      LOG_DEBUG (m_base->name() << ": At TTS " << tts << " Trimmed track " << ptr);
    }
  } // end foreach

  // Set our outputs to the frame/timestamp we are reconciling. This
  // value is obviously behind the last input we received.
  this->m_currentOutputs = this->m_inputBuffer.datum_at(ref_idx);
  this->m_currentOutputs.tracks().clear();

  // Use our output time as target TS (TTS)
  // Then scan through our list of active tracks to see which ones apply to
  // the frame we are passing along.  This frame may have zero or more tracks.
  bool isOK(false);
  tts = this->m_inputBuffer.datum_at(ref_idx).timestamp(); // oldest

  try {
    isOK = this->generate_active_tracks ( tts );
  } catch(vcl_runtime_error & e)
  {
    LOG_ERROR( m_base->name() << ": Exception thrown while generating tracks.\n"
                              << e.what() );
    // For now, let this go as a success - revisit later
    isOK = true;
  }
  if ( !isOK )
  {
    LOG_ERROR( m_base->name() << ": Could not generate tracks.");
    return ( vidtk::process::FAILURE );
  }

  if (IsVerbose(1)) // dump all reconciled tracks
  {
    LOG_DEBUG ( m_base->name() << ": Outputting frame @  " << this->m_inputBuffer.datum_at(ref_idx).timestamp()
                << ", " << this->m_currentOutputs.tracks().size() << " active tracks." );

    if (IsVerbose(2)) // dump all reconciled tracks
    {
      vcl_stringstream stuff;

      BOOST_FOREACH(track_sptr ptr, this->m_currentOutputs.tracks())
      {
        stuff << ptr;

        if (IsVerbose(3)) // dump all reconciled tracks
        {
          FormatStates (stuff, ptr);
        }
      }

      LOG_DEBUG ( stuff.str() );
    } // end verbose
  }

  return ( vidtk::process::SUCCESS );
} /* reconcile_tracks */


// ----------------------------------------------------------------
/** Reset process outputs.
 *
 * This method resets outputs to a state indicating that there are no
 * real outputs.
 */
void reconciler::reconcile_tracks_process_impl::
reset_outputs()
{
  // Reset all fields in the output data slice.
  this->m_currentOutputs.Reset();

  // Reset outputs produced in the process
  this->out_terminated_tracks.clear();
}


// ----------------------------------------------------------------
/** Reset all process inputs.
 *
 *
 */
void reconciler::reconcile_tracks_process_impl::
reset_inputs()
{
  // Reset all fields in the input data slice
  this->m_currentInputs.Reset();
}


// ================================================================
// Output methods

vidtk::track_vector_t const&
reconcile_tracks_process::
output_terminated_tracks() const
{
  return ( m_impl->out_terminated_tracks );
}


// ================================================================
// Pass through methods
// Use datum list to generate input and output methods

#define PROCESS_PASS_THRU(T,N)                                  \
void reconcile_tracks_process::                                 \
input_ ## N (T const& val) { m_impl->m_currentInputs.N(val); }  \
T const& reconcile_tracks_process::                             \
output_ ## N () const { return m_impl->m_currentOutputs.N(); }

#define DATUM_DEF(T, N, I) PROCESS_PASS_THRU(T,N)

  RECONCILER_DATUM_LIST // macro magic

#undef DATUM_DEF
#undef PROCESS_PASS_THRU

} // end namespace vidtk

