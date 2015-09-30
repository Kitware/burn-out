/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_INTERPOLATE_TRACK_STATE_PROCESS_H
#define INCL_INTERPOLATE_TRACK_STATE_PROCESS_H

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/track.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/config_block.h>
#include <utilities/buffer.h>

// ----------------------------------------------------------------
/** Interpolate to ensure each frame in the output track has state.
 ** Mantis: VIRAT 574

 **
 ** Background
 **

Abstractly, at each step we have an input tuple of (TL, TS, H), where
TL is a list of active tracks, TS is a single timestamp, and H is the
img2world homography for TS. TL has an arbitrary amount of history up
to and including TS. As soon as a gap is detected between the
timestamps TS[last-step] and TS[this-step], synthesize the following
by interpolation for all the gap timestamps (i.e. timestamps between
but not including TS[last-step] and TS[this-step]):

-- homography
-- a track state for all active tracks with
----- bounding box
----- velocity
----- location

Create a copy of the track and insert these synthesized states into
the track history.  (Don't modify the existing track history to avoid
confusing any other pipeline nodes which may also be looking at the
track.)

Note that this implies the following:

-- We have to wait until the end of the gap is detected before we can
   do anything.  Call the earlier good interpolation data "anchor-0";
   call the second one (which triggers the interpolation) "anchor-1".
-- We only interpolate, not extrapolate.
-- We have a mechanism to generate missing timestamps.
-- The timestamps are such that gaps can be detected.
-- Tracks cannot begin or end in a gap frame.  (However, this gets
   more complicated when you have multiple tracks in a frame,
   which is of course the reality of the situation.  See
   the implementation of interpolate_track_states() for a discussion.)

 **
 ** Feature creep
 **

As stated, this will fill in gaps created by missed detections in the
process.  But with a slight modification to the time stamp structure,
we can also use this to upsample 10Hz tracks to 30Hz (or other frame
rates.)

 **
 ** Inputs
 **

- TL, vector of active tracks
- TSV, vector of timestamps
- HI, image-to-reference homography (img->img)
- HR, reference-to-world homography (img->world)
- DIM, image dimensions

TL and HI are relative to the current step.  HR is relative to the
current shot (i.e. constant across many steps.)  TSV is a vector of
timestamps, one of which is associated with the current step, the
others are timestamps for frames to be synthesized (for upsampling.)
DIM is constant at least as long as HR is constant and probably is
fixed throughout the lifetime of the process.

All inputs are optional and may fail, leading to a flush of the gap buffer
(step [3] in workflow, below.)

 **
 ** Outputs
 **

- TL, vector of active tracks
- TS, a (single) timestamp
- HI, image-to-reference homography (img->img)

One set of outputs is pushed for each timestamp in TSV; in other
words, this process produces output faster than it reads it.

 **
 ** What is a gap?
 **

Let a packet be a tuple (TL, TS, H).  There is one "packet" per video frame.
A "complete" packet has the following properties:

- H is not invalid (all zeros)
- TS is not invalid
- Every track in TL has a track state defined at t=TS.

Then "incomplete" frames arise in four circumstances:

1) For frames to be upsampled, H is invalid (and TL is empty)
2) For frames missing detections, H is valid, tracks in TL have states
   undefined at t=TS
3) For frames processed but with no tracks, H is valid, but TL is
   empty
4) After a reset, before the first "good frame", H is invalid, TL is
   empty, and, for good measure, HR is invalid

In general, interpolation is possible between two complete frames into
any incomplete frame with a timestamp.  A gap is defined to be a
sequence of incomplete frames bounded by two complete frames.

 **
 ** Input / output in more detail
 **

Output is always generated for each timestamp in TSV.  If TL is empty
on input, it is empty on output.

We assume the ref-to-world homography is constant for any gap over
which interpolation is requested.

Homography interpolation occurs by:
-- Linearly interpolating the four corner points in world coords
   and deriving the homography from the correspondences of those world
   corner points to the corners from (0,0) x DIM.

Track state interpolation occurs in world coordinates thusly:
-- Linearly interpolate position
-- Linearly interpolate box width and height
-- Assume position is the center bottom of the (world) box (i.e. feet position)
-- Create new world boxes, map into image space using interpolated
   homographies.

Time stamps are never modified.

 **
 ** Workflow
 **

For the following, "input packets" are created from the input port at each
step by taking the TL and H, associating with the "live" timestamp from TSV
(see params below), and creating incomplete packets from the remaining TSV
timestamps for upsampling.  If no upsampling is desired, supply a TSV of length 1.

For example: if the inputs at step are (TL, TSV=[a,b,c,d], H), and the "live"
timestamp is c (i.e. "UpsampleLive" is "2", see Params) then four input packets
are created (where TL0 is an empty track list and Hi is a default (invalid)
homography):

(TL0, TS=a, Hi)
(TL0, TS=b, Hi)
(TL,  TS=c, H)
(TL0, TS=d, Hi)

Each of these packets is run through the state machine described
in process_packet() in the implementation.

 **
 ** Other parameters
 **

The GSD is carried along but not interpolated.  When a track packet
is synthesized, the GSD is obtained from the GSD in its process input
packet.

 **
 ** params
 **

- Enabled: If 1, work proceeds as above.  If 0 (disabled), always copy input
packets to output.

- UpsampleLive: A zero-based index into the TSV, indicating which TS should
be associated with the other step inputs; other TSV values are interplated.
Semantically, other TSV values are timestamps for which upsampling is requested.
The index is clipped to the max index in the TSV at each step, so if you
typically have 5 TSV values but only one on startup, a value of "4" here will
work.

- Debug: an integer; 0 for no output, higher numbers for more output.

*/
namespace vidtk
{

class interpolate_track_state_process_impl;


class interpolate_track_state_process
  : public process
{
public:

  // be friendly so impl can call our push_output()
  friend class interpolate_track_state_process_impl;

  typedef interpolate_track_state_process self_type;

  explicit interpolate_track_state_process( const vcl_string& name );

  ~interpolate_track_state_process();

  virtual config_block params() const;

  virtual bool reset();

  virtual bool set_params( const config_block& blk );

  virtual bool initialize();

  virtual process::step_status step2();

  // Process inputs
  void set_input_tracks( const track_vector_t& tl );
  VIDTK_OPTIONAL_INPUT_PORT( set_input_tracks, const track_vector_t& );

  void set_input_img2ref_homography( const image_to_image_homography& hi );
  VIDTK_OPTIONAL_INPUT_PORT( set_input_img2ref_homography, const image_to_image_homography& );

  void set_input_ref2world_homography( const image_to_utm_homography& hr );
  VIDTK_OPTIONAL_INPUT_PORT( set_input_ref2world_homography, const image_to_utm_homography& );

  void set_input_timestamps( const vcl_vector<timestamp>& tsv );
  VIDTK_OPTIONAL_INPUT_PORT( set_input_timestamps, const vcl_vector<timestamp>& );

  void set_input_gsd( double gsd );
  VIDTK_OPTIONAL_INPUT_PORT( set_input_gsd, double );

  // Process outputs
  const track_vector_t& output_track_list() const;
  VIDTK_OUTPUT_PORT( const track_vector_t&, output_track_list );

  const timestamp& output_timestamp() const;
  VIDTK_OUTPUT_PORT( const timestamp&, output_timestamp );

  const image_to_image_homography&  output_img2ref_homography() const;
  VIDTK_OUTPUT_PORT( const image_to_image_homography&, output_img2ref_homography );

  const image_to_utm_homography&  output_ref2utm_homography() const;
  VIDTK_OUTPUT_PORT( const image_to_utm_homography&, output_ref2utm_homography );

  const double& output_gsd() const;
  VIDTK_OUTPUT_PORT( const double&, output_gsd );

private:
  interpolate_track_state_process_impl* p;

}; // class

} // namespace

#endif
