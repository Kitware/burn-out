/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "shot_stitching_process.h"

#include <vcl_deque.h>

#include <utilities/unchecked_return_value.h>
#include <tracking/shot_stitching_algo.h>
#include <logger/logger.h>


VIDTK_LOGGER ("shot_stitching_process");


namespace /* anon */
{

//
// offload config block logic here
//

struct ssp_param_type
{
  vidtk::config_block blk;

  bool enabled;
  int max_n_glitch_frames;
  double max_n_glitch_seconds;
  bool verbose;
  int min_shot_size;

  ssp_param_type()
    : enabled( true ),
      max_n_glitch_frames( 5 ),
      max_n_glitch_seconds( -1.0 ),
      verbose( false ),
      min_shot_size( 0 )
  {
    blk.add_parameter( "enabled", "0", "1 = enable; 0 = disable (pass all inputs)" );
    blk.add_parameter( "max_n_glitch_frames", "5", "After N glitch frames, give up (-1 to wait forever)" );
    blk.add_parameter( "max_n_glitch_seconds", "-1", "After N glitch seconds, give up  (-1 to wait forever)" );
    blk.add_parameter( "min_shot_size", "2", "Min number of frames after new-ref to be considered a valid shot");
    blk.add_parameter( "verbose", "0", "increase verbosity" );
  }

  bool set_params( const vidtk::config_block& b )
  {
    try
    {
      b.get( "enabled", this->enabled );
      b.get( "max_n_glitch_frames", this->max_n_glitch_frames );
      b.get( "max_n_glitch_seconds", this->max_n_glitch_seconds );
      b.get( "min_shot_size", this->min_shot_size );
      b.get( "verbose", this->verbose );
    }
    catch (vidtk::unchecked_return_value& e )
    {
      LOG_ERROR( "shot_stitching_process: param error: " << e.what() );
      return false;
    }

    this->blk.update( b );
    return true;
  }
};

} // anon namespace

//
// Data flow and the state machine:
// We only need to buffer those states whose pipeline status we need
// to update from shot-end to not-processed:
//
// init -> init: pass-through
// init -> steady-state: pass-through
// steady-state -> steady-state: pass-through
// steady-state -> in_glitch: buffer
// in_glitch -> in_glitch: buffer
// in_glitch -> {steady_state, init}: flush + pass-through
//
// To isolate the state machine from the nitty-gritty of
// pumping the outputs, all the "pass-throughs" are implemented
// via the buffering data_queue anyway.  In particular, the problem
// of remembering the "last good packet" for the stitching
// algorithm is handled in the data_queue processing, not the
// state machine.
//

namespace vidtk
{

template< typename PixType >
class shot_stitching_process_impl
{
public:

  // user-visible parameters
  ssp_param_type params;

  // our local state data
  enum ssp_state_type { INIT, STEADY_STATE, WITHIN_GLITCH } ssp_state;
  int glitch_frame_count;

  // the data queue
  vcl_deque< typename shot_stitching_process<PixType>::pads_type > data_queue;

  // our anchor packet for stitching (set in flush_queue() )
  struct
  {
    bool valid;
    typename shot_stitching_process<PixType>::pads_type packet;
  } anchor;

  // reference to parent (interface) object for pads and push_output
  shot_stitching_process<PixType>* parent;

  // stitching algorithm object
  shot_stitching_algo<PixType> stitcher;

  explicit shot_stitching_process_impl( shot_stitching_process<PixType>* p )
    : parent( p ),
      min_shot_size_counter (0)
  {
    // import the algorithm parameters
    params.blk.add_subblock( stitcher.params(), "algo" );
    reset();
  }

  void reset()
  {
    ssp_state = INIT;
    glitch_frame_count = 0;
    // fixme: Arslan's code review comments:

    // This seems to be the only place where count is being reset.
    // I'm under the impression that in the new pipeline this process
    // won't be reset() anymore at the shot boundaries. This should
    // probably be discussed in person.

    data_queue.clear();
    anchor.valid = false;
  }

  process::step_status step2();

private:
  process::step_status flush_queue();
  ssp_state_type record_glitch_frame();
  void attempt_stitching();

  // This is our running adjustment that we apply to all
  // frames we generate.
  image_to_image_homography major_adjustment_;

  int min_shot_size_counter;
};


// ----------------------------------------------------------------
/** Main process method.
 *
 * This method does the work of stepping through the state machine and
 * calling the shot-stitching algorithm when necessary.
 *
 * It's important that flush_queue() only gets called at most once per
 * step2(); its internal logic for calling push_output() on the parent
 * object depends on this.
 *
 */
template< typename PixType >
process::step_status
shot_stitching_process_impl<PixType>
::step2()
{
  if (params.verbose)
  {
    LOG_INFO( "ssp: input status " << parent->pads.input_status
              << " for " <<  parent->pads.shot_break_flags_pad_
              << "  " << parent->pads.timestamp_pad_);
  }


  bool inputs_valid = parent->pads.is_valid();
  parent->pads.reset_status();

  // Always place the data on the input pads on the queue.
  data_queue.push_back( parent->pads );

  // if the inputs failed, flush the queue and return
  if ( ! inputs_valid )
  {
    flush_queue();
    return process::FAILURE;
  }

  // If we're disabled, empty the queue and return.
  if ( ! params.enabled )
  {
    return flush_queue();
  }

  // Otherwise, work through our state machine
  process::step_status rc;

  // is the current frame usable?
  bool current_frame_usable = parent->pads.shot_break_flags_pad_.is_frame_usable();
  bool homog_valid          = parent->pads.src2ref_h_pad_.is_valid();
  bool homog_new_ref        = parent->pads.src2ref_h_pad_.is_new_reference();

  if ( params.verbose )
  {
    LOG_INFO( "ssp: ssp_state " << ssp_state
              << "; anchor valid " << anchor.valid
              << "; fu " << current_frame_usable
              << "; homog valid " << homog_valid
              << "; homog new ref " << homog_new_ref
              << "; dq length " << data_queue.size());
  }

  switch (ssp_state)
  {

    //
    // We start in this state and get back to it if the glitch exceeds
    // our limits.  We stay in this state until a usable frame is
    // received.
    //
  case INIT:
    {
      // disable our adjustment and let the current reference frame be
      // used.
      major_adjustment_.set_valid(false);

      if ( current_frame_usable )
      {
        // move from INIT to STEADY_STATE
        ssp_state = STEADY_STATE;
      } // ...else loop; either way, always flush

      rc = flush_queue();
    }
    break;

    //
    // We are in this state after we have seen a usable frame.  We
    // stay in this state until we see a shot-break.
    //
  case STEADY_STATE:
    {
      if ( current_frame_usable && homog_valid && !homog_new_ref )
      {
        // loop in STEADY_STATE; flush the output
        rc = flush_queue();
      }
      else // not all the above
      {
        // either frame is not usable
        // -or- homog is invalid
        // -or- homog has a new ref (indicates start of shot)

        // reset counter - start of glitch found
        min_shot_size_counter = params.min_shot_size;
        glitch_frame_count = 0;

        // start recording glitches; transition
        // either to INIT or WITHIN_GLITCH depending on
        // timeout parameters
        ssp_state = record_glitch_frame();
        rc =
          ( ssp_state == INIT )
          ? flush_queue()       // don't think this should happen here? but if it does, flush
          : process::SKIP;      // on to WITHIN_GLITCH, hence buffer
      }
    }
    break;

    //
    // We enter this state when a shot break is encountered and remain
    // here until then end of the glitch is found or the allowable
    // bounds of the glitch is exceeded.
    //
  case WITHIN_GLITCH:
    {
      if ( current_frame_usable && homog_valid && !homog_new_ref )
      {
        // Need to see <N> frames with !homog_new_ref to make sure we
        // are really at the start of a shot.  Then back-up and end
        // glitch at first frame we see with homog_new_ref set (going
        // backwards).

        min_shot_size_counter--; // count frame in new shot
        if (min_shot_size_counter <= 0)
        {
          // We can exit the glitch state-- try stitching...
          attempt_stitching();

          // ...either it succeeded or it didn't; either way, flush the queue...
          rc = flush_queue();

          // ...and return to steady_state
          ssp_state = STEADY_STATE;
        }
        else
        {
          // stay in this state
          rc = process::SKIP;
        }

      }
      else
      {
        // loop in glitch state
        ssp_state = record_glitch_frame();

        // reset counter - start of glitch found
        min_shot_size_counter = params.min_shot_size;

        rc =
          ( ssp_state == INIT )
          ? flush_queue()       // if we timed out, flush queue
          : process::SKIP;      // stay in glitch state, buffer
      }
    }
    break;

  default:
    LOG_ERROR( "shot_stitching_process: bad ssp state " << ssp_state << "?" );
    return process::FAILURE;
  }

  if ( params.verbose )
  {
    LOG_INFO( "ssp: step returning " << rc );
  }
  return rc;
}


// ----------------------------------------------------------------
/** Flush queued frames.
 *
 *
 */
template< typename PixType >
process::step_status
shot_stitching_process_impl<PixType>
::flush_queue()
{
  // Every call to step2() pushes an input on the queue, so the
  // queue should never be empty when this is called.
  //
  // Always check the back (most recent) packet; if its state
  // is "usable", copy it as the anchor for any subsequent
  // stitching.  (Inefficient if we're looping in steady state,
  // but the alternative is to flush all-but-one for some cases
  // and flush everything in others, which gets really complicated.)
  //
  // Since we only get called at most once per step, we can handle
  // all the push_output logic here and don't need persistent state
  // outside flush_queue() but within step2() (for example, no
  // first_pump flags.)
  //

  if (data_queue.empty())
  {
    LOG_ERROR( "shot_stitching_process: Flush_queue called on empty queue?" );
    return process::FAILURE;
  }

  // update anchor status
  if ( data_queue.back().shot_break_flags_pad_.is_frame_usable()
       && ! data_queue.back().src2ref_h_pad_.is_new_reference() )
  {
    anchor.valid  = true;
    anchor.packet = data_queue.back(); // last one in

    if (params.verbose)
    {
      LOG_INFO ("ssp: setting anchor " << anchor.packet.timestamp_pad_ );
    }
  }
  else
  {
    anchor.valid = false;
  }

  // send the data out; call push_outputs for all but the last one
  while ( ! data_queue.empty() )
  {
    parent->pads = data_queue.front();

    // If we have a valid major adjustment, apply it now.  This
    // adjustment accounts for all previous glitches we have stitched
    // over.
    if (major_adjustment_.is_valid()
        && parent->pads.src2ref_h_pad_.is_valid() )
    {
      image_to_image_homography temp = parent->pads.src2ref_h_pad_;
      parent->pads.src2ref_h_pad_ = major_adjustment_ * temp;

      if (params.verbose)
      {
        LOG_INFO ("ssp: applying major adjustment to " << temp
                  << "---- yielding: "
                  << parent->pads.src2ref_h_pad_
          );
      }
    }

    if (params.verbose)
    {
      LOG_INFO ("ssp: flushing frame " << parent->pads.timestamp_pad_ );
    }

    data_queue.pop_front();
    if ( ! data_queue.empty() )
    {
      parent->push_output( process::SUCCESS );
    }
  } // end while

  // all done
  return process::SUCCESS;
}


// ----------------------------------------------------------------
/** Record frame in the glitch.
 *
 *
 */
template< typename PixType >
typename shot_stitching_process_impl<PixType>::ssp_state_type
shot_stitching_process_impl<PixType>
::record_glitch_frame()
{
  // the glitch frame is always in the data_queue at this point.
  ++glitch_frame_count;

  bool timeout_frames =
    (params.max_n_glitch_frames != -1) &&
    (glitch_frame_count > params.max_n_glitch_frames);

  bool timeout_seconds =
    (params.max_n_glitch_seconds != -1.0) &&
    ((parent->pads.timestamp_pad_.diff_in_secs( anchor.packet.timestamp_pad_ ) > params.max_n_glitch_seconds));

  ssp_state_type rc;

  if (timeout_frames || timeout_seconds)
  {
    // timed out - exceeded limits
    LOG_INFO ("ssp: record: exceeded limits (frame_count " << timeout_frames
              << "  max time " << timeout_seconds << ").  Giving up."
      );

    rc = INIT; // go back to init state
  }
  else
  {
    rc = WITHIN_GLITCH;
  }

  if (params.verbose)
  {
    LOG_INFO( "ssp: record: timeout-frames? " << timeout_frames
              << "; timeout-seconds? " << timeout_seconds
              << "; " << glitch_frame_count << " glitch frames; rc " << rc );
  }
  return rc;
}


// ----------------------------------------------------------------
/** Attempt to stitch across glitch.
 *
 *
 */
template< typename PixType >
void
shot_stitching_process_impl< PixType >
::attempt_stitching()
{
  bool new_ref_found(false);
  unsigned nri;
  // Scan the queue backwards looking for the entry with new-ref set.
  // That entry is the other end of the stitch.
  for (nri = data_queue.size()-1; nri >= 0; nri--)
  {
    if (data_queue[nri].src2ref_h_pad_.is_new_reference())
    {
      new_ref_found = true;
      break;
    }
  }

  // On exit, nri->end of glitch, if new_ref_found is true.
  if ( ! new_ref_found)
  {
    // we scanned the whole queue and did not find a homog with a
    // new_ref. This is unexpected.
    LOG_ERROR ("ssp::attempt_stitching - could not find new reference.");
    return;
  }

  if ( params.verbose )
  {
    LOG_INFO("ssp: stitching from " << anchor.packet.timestamp_pad_
             << " to " << data_queue[nri].timestamp_pad_
             << "\n" << anchor.packet.src2ref_h_pad_ );
  }


  // using current as "src" and anchor as "dest" means we don't have
  // to invert the result to factor it into anchor.src2ref to get
  // current2ref.

  image_to_image_homography current2anchor =
    stitcher.stitch(data_queue[nri].image_pad_, anchor.packet.image_pad_);

  // if it didn't work, just return
  if ( !current2anchor.is_valid() )
  {
    LOG_INFO ("ssp: stitching failed");

    // disable our adjustment and let the current reference frame be
    // used.
    major_adjustment_.set_valid(false);

    return;
  }

  // it worked!  Factor the new homography into the pads and change
  // the status on the unusable frames in queue.
  current2anchor.set_new_reference(false);
  current2anchor.set_source_reference(data_queue[nri].src2ref_h_pad_.get_source_reference())
    .set_dest_reference(anchor.packet.src2ref_h_pad_.get_dest_reference());

  if (params.verbose)
  {
    LOG_INFO ("ssp: stitching homog " << current2anchor);
  }

  //
  // Adjust our major adjustment to go from the original ref frame
  // to current ref frame.
  //
  if (major_adjustment_.is_valid())
  {
    LOG_INFO ("ssp: original major_adjustment: " << major_adjustment_ );
    major_adjustment_ = major_adjustment_ * current2anchor;
  }
  else
  {
    major_adjustment_ = current2anchor;
  }

  if (params.verbose)
  {
    LOG_INFO ("ssp: updated major_adjustment to: " << major_adjustment_ );
  }

  //
  // The contents of the queue from [nri, size) are valid frames and
  // will get their s2r's updated.
  //
  // The contents of the queue from [0, nri) are dropped from queue,
  // never to be seen again.
  //
  if ( params.verbose )
  {
    LOG_INFO("ssp: stitched; data queue size " << data_queue.size()
             << "\nUsing bridge homog: " << current2anchor );
  }

  // Delete all the elements in the queue (up to, but not including, the
  // new ref index) since they are unusable.
  for (unsigned i = 0; i < nri; ++i)
  {
    if ( params.verbose )
    {
      shot_break_flags& f = data_queue[0].shot_break_flags_pad_;

      LOG_INFO("ssp: stitched; dropping data queue element " << i
               << " status was " << f
               << " at " << data_queue[0].timestamp_pad_);
    }

    data_queue.pop_front();
  } // end for

} /* attempt_stitching */


// ===============================================================================
// process interfaces

template< typename PixType >
shot_stitching_process<PixType>
::shot_stitching_process( const vcl_string& name )
  : process( name, "shot_stitching_process" ),
    p(0)
{
  p =  new shot_stitching_process_impl< PixType >( this );
}


template< typename PixType >
shot_stitching_process<PixType>
::~shot_stitching_process()
{
  delete p;
}


template< typename PixType >
config_block
shot_stitching_process<PixType>
::params() const
{
  return p->params.blk;
}


template< typename PixType >
bool
shot_stitching_process<PixType>
::set_params( const config_block& cfg )
{
  bool rc = p->stitcher.set_params( cfg.subblock( "algo" ));
  rc |= p->params.set_params( cfg );
  return rc;
}


template< typename PixType >
bool
shot_stitching_process<PixType>
::initialize()
{
  p->reset();
  return true;
}


template< typename PixType >
bool
shot_stitching_process<PixType>
::reset()
{
  p->reset();
  return true;
}


template< typename PixType >
process::step_status
shot_stitching_process<PixType>
::step2()
{
  return p->step2();
}

} // namespace vidtk
