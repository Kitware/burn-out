/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_SHOT_STITCHING_PROCESS_H
#define INCL_SHOT_STITCHING_PROCESS_H

// The process-level logic of the across-a-glitch stitching algorithm.
//
// Takes the following as input:
// (1) image (required)
// (2) metadata vector
// (3) timestamp vector
// (4) timestamp

// (5) shot break flags
// (6) src2ref_homog (required)
// (7) unprocessed image mask
//
// Collectively, 1-7 make up a packet.
//
// The shot break flags (5) is considered to be either GOOD (frame_usable)
// or BAD (shot_end or frame_not_processed.)
//
// Generally, while good, packets are passed through.  Once (5) goes bad,
// packets are buffered until a good packet is seen, when an attempt to
// stabilize between the current and last good packet is made.  If
// unsuccesful, the packets are flushed unchanged.  If the attempt is successful,
// the status of the buffered bad packets (5) are changed to "frame not processed"
// (but the homographies are unchanged) and the stitched homography is
// factored into (6); 1-4  and 7 are always passed through unchanged.
//
// states:
// (a) init (no anchor frame)
// (b) steady-state (we have an anchor frame)
// (c) within-glitch (anchor, but current frame is bad)
//
// transitions (se = shot_end, u = frame_usable; np = frame_not_processed)
//
//       se/np     u
//  a    a        b[1]
//  b    c[2]     b[3]
//  c    a/c[2]   b[4]
//
// notes:
// [1] Once we see a good frame, remember it and enter steady-state.
// [2] Once we lose track, need to loop in (c) until we see a good frame again.
//     If we timeout, ditch the anchor frame and return to (a).
// [3] As long as we have a good frame, loop and update the anchor frame.
// [4] We exit the glitch loop on a good frame, and attempt to correct.
//     Regardless of the outcome, we return to (b).
//
// There are complications due to the need to buffer data accumulated in
// (b), but this is the general idea.
//
// config params:
// - enabled: when false, all inputs are passed through
// - max_glitch_frame_count: maximum number of frames to wait to exit from
// state (c) above before bailing and going to (a).  Set to -1 to wait forever.
// - max_glitch_elapsed_time: maximum number of seconds to wait to exit from
// state (c) above before bailing and going to (a).  Set to -1 to wait forever.
//
// If both max_glitch_{frame_count, elapse_time} are set to != -1, whichever
// condition is met first will trigger transition to (a).
//


#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>
#include <kwklt/klt_track.h>

#include <tracking/shot_break_flags.h>


namespace vidtk
{

template< typename PixType > class shot_stitching_process_impl;

template< typename PixType >
class shot_stitching_process
  : public process
{
public:
  typedef shot_stitching_process self_type;
  friend class shot_stitching_process_impl<PixType>;  // so impl can see pads_type and push_output

  explicit shot_stitching_process( const vcl_string& name );
  virtual ~shot_stitching_process();

  virtual config_block params() const;
  virtual bool set_params( const config_block& cfg );

  virtual bool initialize();
  virtual bool reset();
  virtual process::step_status step2();

private:
  shot_stitching_process_impl< PixType >* p;

// Define list of ports
//  name, type
#define PORT_LIST                                                       \
  PORT_DEF(image,                   vil_image_view<PixType>)            \
  PORT_DEF(src2ref_h,               image_to_image_homography)          \
  PORT_DEF(timestamp,               vidtk::timestamp)                   \
  PORT_DEF(timestamp_vector,        vidtk::timestamp::vector_t )        \
  PORT_DEF(shot_break_flags,        vidtk::shot_break_flags)            \
  PORT_DEF(metadata_vector,         video_metadata::vector_t )          \
  PORT_DEF(klt_tracks,              vcl_vector< klt_track_ptr > )       \
  PORT_DEF(metadata_mask,           vil_image_view< bool > )


  // because this process runs asynchronously, we need to recognize
  // when our inputs fail.  This is facilitated by folding all the
  // inputs into a struct and explicitly recording when they're set.
  enum input_status_type {
    SET_NONE = 0x000,

    // Define input set bit number
#define PORT_DEF(N, T) SET_ ## N,

    PORT_LIST   // expand over all ports

#undef PORT_DEF

    SET_LAST  // number of inputs +1
  };

//
// Establish required inputs (not pass-through)
//
#define INPUT_COUNT (SET_LAST-1) // number of inputs currently configured

#define REQUIRED_INPUTS ( (1 << SET_shot_break_flags) | (1 << SET_timestamp) | (1 << SET_src2ref_h) )
#define OPTIONAL_INPUT_COUNT (1) // inputs that are simetimes not connected.

// ----------------------------------------------------------------
/* Input data set
 *
 *
 */
  struct pads_type
  {

// Define input storage areas (pads)
#define PORT_DEF(N,T)   T N ## _pad_;

    PORT_LIST   // expand over all ports

#undef PORT_DEF

    unsigned input_status;
    int input_count;

  pads_type() : input_status( SET_NONE ), input_count(0) {}
    void reset_status() { input_status = SET_NONE; }

    /** Is this a valid set of inputs. If the inputs are not valid,
     * then it is assumed to be end of video and the process should
     * fail.
     */
    bool is_valid() const
    {
      return (input_status & REQUIRED_INPUTS)
        && (input_count >= (INPUT_COUNT - OPTIONAL_INPUT_COUNT) ) // required inputs
        && this->timestamp_pad_.is_valid();
    }
  } pads;

public:

// Define input and output ports and access methods
#define PORT_DEF(N,T)                                                   \
  void input_ ## N( const T& i ) { pads.N ## _pad_ = i; pads.input_status |= ( 1 << SET_ ## N); pads.input_count++; } \
  VIDTK_OPTIONAL_INPUT_PORT( input_ ## N, const T& );                   \
  const T& output_ ## N() const { return pads.N ## _pad_; }              \
  VIDTK_OUTPUT_PORT( const T&, output_ ## N );

    PORT_LIST   // expand over all ports

#undef PORT_DEF

};

} // namespace vidtk

#endif
