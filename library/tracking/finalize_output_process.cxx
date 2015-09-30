/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/finalize_output_process.h>

#include <utilities/ring_buffer.txx>
#include <utilities/video_modality.h>
#include <utilities/unchecked_return_value.h>
#include <vcl_algorithm.h>
#include <vcl_ostream.h>
#include <vcl_sstream.h>
#include <boost/foreach.hpp>
#include <vcl_fstream.h>

#include <logger/logger.h>


/// @todo This file conains a GSD writer that should be moved out to
/// another file.

namespace vidtk
{

VIDTK_LOGGER("finalize_output_process");

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


static vcl_ostream& FormatLUT(vcl_ostream& str, vcl_map< unsigned, track_sptr > const& map)
{
  vcl_map< unsigned, track_sptr >::const_iterator it;
  for (it = map.begin();
       it != map.end();
       it++)
  {
    str << ( it->second );
  }

  return ( str );
}


// ================================================================
/** Set of synchronized inputs/outputs
 *
 * This class represents a set of synchronized inputs that move
 * through the finalization process together. The actual set of inputs
 * is defined in the FINALIZER_DATUM_LIST macro.
 */
  class InputSet
  {
  public:
    // -- TYPES --
    // bit mask for inputs that are present
    enum {
#define DATUM_DEF(T, N, I, R) N ## _present,
      FINALIZER_DATUM_LIST
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
#define DATUM_DEF(T, N, I, R) m_ ## N = I;
  FINALIZER_DATUM_LIST // macro magic
#undef DATUM_DEF

    } // end Reset()


//
// Define the accessors for data elements
//
#define GET_SET(T,N,I)                                   \
  public:                                               \
    T const& N() const { return ( this->m_ ## N ); }    \
  InputSet & N(T const& val) { this->m_ ## N = val;     \
    this->m_inputCount++;                               \
    this->m_inputMask |= (1 << N ## _present);          \
    return ( *this ); }                                 \
  void reset_ ## N() { m_ ## N = I; }                   \
  private:                                              \
    T m_ ## N;                                          \
  public:

// use datum list to generate accessor methods
#define DATUM_DEF(T, N, I, R) GET_SET(T,N,I)
    FINALIZER_DATUM_LIST // macro magic
#undef DATUM_DEF
#undef GET_SET


    int InputCount() const { return this->m_inputCount; }

    // this really should use a bitmask approach to determining that the required
    // parameters are present.
    bool InputsValid() const { return (this->m_inputCount >= (6)); }

    vcl_ostream & ToStream (vcl_ostream & str) const
    {
      // Use datum list to generate display code
#define DATUM_DEF(T,N,V, R) << "  " # N << ": " << this->m_ ## N << "\n"

      str << "Finalizer input/output set\n  "
         << InputCount() << " values present\n"

        FINALIZER_DATUM_LIST // macro magic

#undef DATUM_DEF

        ;
     return (str);
    }

    vcl_ostream & InputsPresent (vcl_ostream & str) const
    {
#define DATUM_DEF(T, N, V, R)                                           \
      str << # N;                                                       \
      if ( ! (m_inputMask & (1 << N ## _present)) )   str << " not";    \
      str << " present, ";

      FINALIZER_DATUM_LIST // macro magic

#undef DATUM_DEF

      return (str);
    }


  protected:


  private:

    int m_inputCount;
    unsigned m_inputMask;

  }; // end class InputSet

inline vcl_ostream & operator << (vcl_ostream & str, InputSet const & obj)
{ return obj.ToStream(str); }

} // end namespace

// instantiate our custom ring buffer
template class vidtk::ring_buffer < InputSet >;


// Define our operational states.
enum { ST_RUNNING = 1, ST_FLUSHING };


// ================================================================
/** Finalize output implementation
 *
 *
 */
class finalize_output_process_impl
{
public:

  vidtk::process::step_status step_running();
  vidtk::process::step_status step_flushing();
  vidtk::process::step_status finalize_tracks(int idx);
  void reset_outputs();
  void reset_inputs();

  // Configuration parameters
  config_block config;
  int m_delayFrames;         // used to reset buffer index

  // These manage the passthrough data items
  InputSet m_currentInputs;
  InputSet m_currentOutputs;

  // inputs
  vidtk::track_vector_t         in_active_tracks;
  vidtk::track_vector_t         in_filtered_tracks;
  vidtk::track_vector_t         in_linked_tracks;

  // Computed Outputs
  vidtk::track_vector_t         out_finalized_tracks;
  vidtk::track_vector_t         out_terminated_tracks;
  vidtk::track_view::vector_t   out_finalized_track_views;

  vidtk::track_vector_t         out_filtered_tracks;
  vidtk::track_vector_t         out_linked_tracks;

  // Buffer of synchronized data
  vidtk::ring_buffer < InputSet > m_inputBuffer;


  /** Active tracks lookup table (LUT). This LUT (map) holds tracks
   * and is indexed by track ID.  It holds tracks that have been
   * detected but not yet terminated. When a new track is received as
   * input with an Id that is already in the LUT, the old one is
   * replaced.
   */
  typedef vcl_map< unsigned, track_sptr > track_map_t ;
  typedef track_map_t::iterator iterator_t;
  track_map_t m_activeTracksLUT;

  unsigned m_highWaterMark;

  int m_state; // current operating state
  vidtk::timestamp m_lastFinalizedTs; // last TS needed for duplicate checking
  int m_refBufferIdx; // current finalizing buffer index

  finalize_output_process * m_base;
  int m_verbose;
  bool m_enabled;

  //+ @FIXME - see below
  vcl_string m_gsdFilename;
  vcl_ofstream m_fstr;


  bool IsVerbose(int i) const { return ( m_verbose >= i ); }


// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
  finalize_output_process_impl(finalize_output_process * b) :
    m_delayFrames(0),
    m_highWaterMark(0),
    m_state(ST_RUNNING),
    m_refBufferIdx(0),
    m_base(b),
    m_verbose(0),
    m_enabled(false)
  {
    reset();
  }


// ----------------------------------------------------------------
  ~finalize_output_process_impl()
  {
    if (m_fstr)
    {
      m_fstr.close();
    }


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
  void AddTrack(const track_sptr& trk)
  {
    this->m_activeTracksLUT [ trk->id() ] = trk;

    // Track high water mark
    if (this->m_activeTracksLUT.size() > m_highWaterMark)
    {
      m_highWaterMark = this->m_activeTracksLUT.size();
    }
  }


  // ----------------------------------------------------------------
  /** Generate active track views.
   *
   * This method generates the relevant view on all currently active
   * tracks.
   *
   * @param[in] tts - target timestamp
   */
  bool generate_active_tracks(const vidtk::timestamp& tts)
  {
    // the target timestamp is tts

    iterator_t ix;
    for (ix = this->m_activeTracksLUT.begin(); ix != this->m_activeTracksLUT.end(); /* blank */)
    {
      // Check track (ix.second) last history state timestamp.  If
      // this TS is < finalized_ts, then the last history entry is
      // older than what we are generating, do delete this entry.
      if ( (*ix).second->last_state()->time_ < tts)
      {
        if (IsVerbose(2))
        {
          LOG_INFO( m_base->name() << ": Removing track from LUT @ target-" << tts
                    << " - " << ix->second);
        }

        // This preserves the validity of the iterator through the
        // erase.

        iterator_t dix = ix;
        ix++; // go next element

        // save termianted track
        out_terminated_tracks.push_back ( dix->second );
        this->m_activeTracksLUT.erase(dix);
      }
      else if (ix->second->first_state()->time_ <= tts)
      {

        // Now we have a track view to generate
        track_sptr trk = ix->second;
        track_view::sptr tv = track_view::create(trk);

        tv->set_view_bounds(trk->first_state()->time_, tts);

        // This may be considered as a little wasteful, in that we are
        // creating a full track object where just a lightweight view
        // will suffice.
        trk = *tv; // convert to track
        this->out_finalized_tracks.push_back(trk);
        this->out_finalized_track_views.push_back (tv);

        if (IsVerbose(2))
        {
          LOG_INFO( m_base->name() << ": generate_active_tracks: Finalized track @ target-"
                    << tts << " - " << trk);
        }

        ix++;
      }
      else
      {
        ix++;
      }

    } // end for

    return ( true );
  } /* generate_active_tracks */


}; // end class finalize_output_process_impl

//================================================================


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
finalize_output_process::
  finalize_output_process(vcl_string const& name) :
  process(name, "finalize_output_process"),
  m_impl(new finalize_output_process_impl(this) )
{
  m_impl->config.add_parameter(
    "enabled",
    "false",
    "Turn the module off and pass inputs to outputs." );

  m_impl->config.add_parameter(
    "delay_frames",
    "1",
    "Number of frames to dealy before finalizing tracks.");

  m_impl->config.add_parameter(
    "verbose",
    "0",
    "Controls the amount of diagnostic output generated by this process."
    "0 - no output, 1 - mostly per frame output, 2 - per track output, "
    "3 - per state output (larger numbers mean more output).");

  //+ @FIXME - this output file should be generated in a separate process
  m_impl->config.add_parameter(
    "filename",
    "",
    "Name of file for metadata output.");
}


finalize_output_process::
  ~finalize_output_process()
{
  delete m_impl;
}


// ================================================================
// Config related methods.

config_block finalize_output_process::
  params() const
{
  return ( m_impl->config );
}


// ----------------------------------------------------------------
bool finalize_output_process::
  set_params(config_block const& blk)
{
  try
  {
    blk.get( "enabled", m_impl->m_enabled );
    blk.get("delay_frames", m_impl->m_delayFrames );
    blk.get("verbose", m_impl->m_verbose );
    blk.get("filename", m_impl->m_gsdFilename ); // @FIXME - see other
  }
  catch(unchecked_return_value & e)
  {
    LOG_ERROR(this->name() << ": couldn't set parameters: " << e.what());

    // reset to old values
    this->set_params(m_impl->config);
    return ( false );
  }

  m_impl->config.update(blk);

  assert (m_impl->m_delayFrames > 0);

  return ( true );
}


// ----------------------------------------------------------------
/** process initialize.
 *
 */
bool finalize_output_process::
  initialize()
{
  // @FIXME - open output file
  if( ! m_impl->m_gsdFilename.empty() )
  {
    m_impl->m_fstr.open( m_impl->m_gsdFilename.c_str() );
    if( ! m_impl->m_fstr )
    {
      LOG_ERROR( name() << ": Couldn't open " << m_impl->m_gsdFilename << " for writing." );
      return false;
    }
    else
    {
      // write header line
      m_impl->m_fstr << "## gsd\ttimestamp\tvideo-mode\n";
    }
  }

  return m_impl->reset();
}


// ----------------------------------------------------------------
/** Reset this process.
 *
 * This method is called when the pipeline issues a reset.
 * In this case, the reset is passed to the implementation method.
 *
 */
bool finalize_output_process::
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
  vidtk::process::step_status finalize_output_process::
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
    // 1) Produce no outputs so it becomes obvious that the finalizer
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

    // Also copy filtered tracks and linked tracks forward.
    // If we are not enabled, then we do not provide these outputs.
    m_impl->out_filtered_tracks = m_impl->in_filtered_tracks;
    m_impl->out_linked_tracks = m_impl->in_linked_tracks;

    // Reset all input data areas now that we have moved what is
    // needed to the output area.
    m_impl->reset_inputs();

    // Does not generate terminated tracks finalized tracks or
    // finalized track views when disabled
    return vidtk::process::SUCCESS;
  }

  LOG_DEBUG (this->name() << ": input count: " << m_impl->m_currentInputs.InputCount()
             << "  active track count: " << m_impl->in_active_tracks.size());

  // Test what state we are in.
  if (m_impl->m_state == ST_RUNNING)
  {
    // Check to see if we received any new tracks. If we have all
    // inputs required, then process new input.
    if (input_valid)
    {
      if (m_impl->IsVerbose(1))
      {
        vcl_stringstream stuff;
        LOG_INFO (this->name() << ": Input timestamp: " << m_impl->m_currentInputs.timestamp());

        if (m_impl->IsVerbose(2))
        {
          m_impl->m_currentInputs.InputsPresent(stuff);
          LOG_DEBUG (this->name() << ": Inputs: " << stuff.str());
        }

        if (m_impl->IsVerbose(3))
        {
          LOG_DEBUG (this->name() << "; " << m_impl->m_currentInputs);
        }
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
    m_impl->m_refBufferIdx = vcl_min( static_cast< int >(m_impl->m_inputBuffer.size() - 1),
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
  vidtk::process::step_status finalize_output_process_impl::
  step_running()
{
  reset_outputs();

  // push our inputs to the ring buffer
  this->m_inputBuffer.insert ( this->m_currentInputs );

  // Add input tracks to our master track list.
  BOOST_FOREACH (const track_sptr ptr, this->in_active_tracks)
  {
    this->AddTrack(ptr);

    if (IsVerbose(2))
    {
      LOG_INFO ( m_base->name() << ": Added to LUT: " << ptr);

      if (IsVerbose(3))
      {
        vcl_stringstream stuff;
        FormatLUT(stuff, this->m_activeTracksLUT);
        LOG_INFO (stuff.str() );
      }
    } // end verbose

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

  // Reset all inputs - safe to do since they have been pushed into buffer
  reset_inputs();

  // Find the index of the reference timestamp.  This value is from
  // the configuration and determines how many frames to buffer.

  // If we have not buffered that many frames, just return.  We must
  // have a full buffer before we can finalize any tracks.
  if ( static_cast< unsigned int >(this->m_refBufferIdx) >= this->m_inputBuffer.size() )
  {
    return ( vidtk::process::SKIP );
  }

  // We now have enough frames buffered to start finalizing tracks.
  return ( finalize_tracks(this->m_refBufferIdx) );
} /* step_running */


// ----------------------------------------------------------------
/** Flushing mode processing.
 *
 *
 */
  vidtk::process::step_status finalize_output_process_impl::
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

  retcode = finalize_tracks(ref_idx);

  // If this is the last iteration of the flushing operation, then we
  // have to "finalize" these tracks. We need to do it at this point
  // because all finalized tracks have been generated (and all tracks
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
      LOG_INFO (m_base->name() << ": Terminating track - " << trk.second);
      out_terminated_tracks.push_back (trk.second);
    }
  }

  return ( retcode );
}


// ----------------------------------------------------------------
/** Perform finalizing process
 *
 * This method finalizes all known tracks based on the index provided.
 * The supplied index is used to select a stripe of consistent data
 * values from our input buffers adn generate output tracks based on
 * the timestamp retrieved.
 *
 * An output track will be generated for each known track that has a
 * state at or before the target time stamp.
 *
 * @param ref_idx - buffer reference index
 */
vidtk::process::step_status finalize_output_process_impl::
finalize_tracks(int ref_idx)
{
  // Check to see if there is a duplicate entry in the queue.
  // This sometimes happens at the end of video.
  if ( this->m_inputBuffer.datum_at(ref_idx).timestamp() == this->m_lastFinalizedTs )
  {
    LOG_INFO (m_base->name() << ": duplicate timestamp found, skipping : "
              << this->m_inputBuffer.datum_at(ref_idx).timestamp() );
    return ( vidtk::process::SKIP );
  }

  // Set our outputs to the frame/timestamp we are finalizing. This
  // value is obviously behind the last input we received.
  this->m_currentOutputs = this->m_inputBuffer.datum_at(ref_idx);

  // Save our TS so we can detect duplicate inputs.
  this->m_lastFinalizedTs = this->m_inputBuffer.datum_at(ref_idx).timestamp();

  //+ @FIXME
  // Write gsd and timestamp to output file
  if( m_fstr )
  {
    m_fstr.precision( 20 );
    m_fstr <<  this->m_currentOutputs.world_units_per_pixel() << "\t"
           << this->m_currentOutputs.timestamp() << "\t"
           << vidtk::video_modality_to_string ( this->m_currentOutputs.video_modality() )
           << "\n";
  }
  //+ @FIXME


// Use our output time as target TS (TTS)
  // Then scan through our list of active tracks to see which ones apply to
  // the frame we are passing along.  This frame may have zero or more tracks.
  bool isOK(false);

  try {
    isOK = this->generate_active_tracks ( this->m_inputBuffer.datum_at(ref_idx).timestamp() );
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

  if (IsVerbose(1)) // dump all finalized tracks
  {
    LOG_DEBUG ( m_base->name() << ": Outputting frame @  " << this->m_inputBuffer.datum_at(ref_idx).timestamp()
                << ", " << this->out_finalized_tracks.size() << " active tracks." );

    if (IsVerbose(2)) // dump all finalized tracks
    {
      vcl_stringstream stuff;

      LOG_DEBUG (m_base->name() << ": Synchronized Outputs: " << this->m_currentOutputs);

      BOOST_FOREACH(track_sptr ptr, this->out_finalized_tracks)
      {
        stuff << ptr;

        if (IsVerbose(3)) // dump all finalized tracks
        {
          FormatStates (stuff, ptr);
        }
      }

      LOG_DEBUG ( stuff.str() );
    } // end verbose
  }

  return ( vidtk::process::SUCCESS );
} /* finalize_tracks */


// ----------------------------------------------------------------
/** Reset process outputs.
 *
 * This method resets outputs to a state indicating that there are no
 * real outputs.
 */
void finalize_output_process_impl::
reset_outputs()
{
  // Reset all fields in the output data slice.
  this->m_currentOutputs.Reset();

  // Reset outputs produced in the process
  this->out_finalized_tracks.clear();
  this->out_terminated_tracks.clear();
  this->out_finalized_track_views.clear();
}


// ----------------------------------------------------------------
/** Reset all process inputs.
 *
 *
 */
void finalize_output_process_impl::
reset_inputs()
{
  // Reset all fields in the input data slice
  this->m_currentInputs.Reset();

  // Reset non-pasthrough inputs.
  this->in_active_tracks.clear();
  this->in_filtered_tracks.clear();
  this->in_linked_tracks.clear();
}


// ================================================================
// Input methods

void finalize_output_process::
set_input_active_tracks(vidtk::track_vector_t const& val)
{
  // Copy list of track pointers to our local list.
  m_impl->in_active_tracks = val;
}


void finalize_output_process::
set_input_filtered_tracks(vidtk::track_vector_t const& val)
{
  // Copy list of track pointers to our local list.
  m_impl->in_filtered_tracks = val;
}


void finalize_output_process::
set_input_linked_tracks(vidtk::track_vector_t const& val)
{
  // Copy list of track pointers to our local list.
  m_impl->in_linked_tracks = val;
}


// ================================================================
// Pass through methods
// Use datum list to generate input and output methods

#define PROCESS_PASS_THRU(T,N)                                          \
void finalize_output_process::                                          \
set_input_ ## N (T const& val) { m_impl->m_currentInputs.N(val); }      \
T const& finalize_output_process::                                      \
get_output_ ## N () const { return m_impl->m_currentOutputs.N(); }

#define DATUM_DEF(T, N, I, R) PROCESS_PASS_THRU(T,N)

  FINALIZER_DATUM_LIST // macro magic

#undef DATUM_DEF
#undef PROCESS_PASS_THRU


// ================================================================
// Output methods

vidtk::track_vector_t const&
finalize_output_process::
  get_output_finalized_tracks() const
{
  // return list of finalized tracks
  // Should convert returned value to list of track_view's
  return ( m_impl->out_finalized_tracks );
}


vidtk::track_vector_t const&
finalize_output_process::
  get_output_terminated_tracks() const
{
  LOG_DEBUG ( "finalizer supplying "<< m_impl->out_terminated_tracks.size() << " terminated tracks.");
  return ( m_impl->out_terminated_tracks );
}


vidtk::track_view::vector_t const&
finalize_output_process::
  get_output_finalized_track_views() const
{
  // return list of finalized track views
  return ( m_impl->out_finalized_track_views );
}


vidtk::track_vector_t const&
finalize_output_process::
  get_output_filtered_tracks() const
{
  // return list of finalized tracks
  // Should convert returned value to list of track_view's
  return ( m_impl->out_filtered_tracks );
}


vidtk::track_vector_t const&
finalize_output_process::
  get_output_linked_tracks() const
{
  // return list of finalized tracks
  // Should convert returned value to list of track_view's
  return ( m_impl->out_linked_tracks );
}


// ----------------------------------------------------------------
/** Format internal state to a stream.
 *
 * This method formats the internal state of this process on the
 * supplied stream.
 *
 * @param[in] str - stream to format on
 */
vcl_ostream& finalize_output_process::
  to_stream(vcl_ostream& str) const
{
  str << this->class_name() << "::" << this->name()
      << "  Last frame finalized for:\n" << m_impl->m_currentOutputs

      << "   LUT has " << m_impl->m_activeTracksLUT.size() << " entries"
      << vcl_endl;

  FormatLUT(str, m_impl->m_activeTracksLUT);

  str << "High water mark: " << m_impl->m_highWaterMark << " entries"
      << vcl_endl;

  return ( str );
}

} // end namespace vidtk
