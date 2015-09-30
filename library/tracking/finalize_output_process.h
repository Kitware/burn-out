/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_finalize_output_process_h_
#define vidtk_finalize_output_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <tracking/track.h>
#include <tracking/track_view.h>
#include <tracking/shot_break_flags.h>

#include <utilities/timestamp.h>
#include <utilities/config_block.h>
#include <utilities/ring_buffer.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>


namespace vidtk
{

class finalize_output_process_impl;

// ----------------------------------------------------------------
/** Finalize track output.
 *
 * This class represents a process that finalized output tracks.
 *
 * Finalizing output tracks is essentially converting the stream of
 * tracks that are being detected to a format expected by a specific
 * consumer.  In this case, we do the following:
 *
 * 1) Delay the output of frames for a fixed (configurable) number of
 * frames.
 *
 * 2) Output tracks that start with one event at the current output
 * timestamp and grow in size with each subsequent pipeline step.
 *
 * 3) Do not output tracks where the last state is in the past.  This
 * happens when no more motion is detected, but the track is not yet
 * closed.
 *
 * The tracker takes a number of frames to start detecting a
 * track. These tracks usually have states in the past. For example, a
 * track is detected at frame 15 but it really started at frame
 * 10. (It took until frame 15 to determine it was a track.) We
 * receive a track with 6 states in this case.
 *
 * The output of the finalizer generates an output track, it will
 * start at frame 10 with a track with one state. Frame 11 will have 2
 * states, and so forth. In order to do this, we buffer a number of
 * frames so when we get a new track with states in the past, we have
 * not yet finalized the frame where the new track starts.
 *
 */
class finalize_output_process
  : public process
{
public:
  typedef finalize_output_process self_type;

  finalize_output_process(vcl_string const &);
  ~finalize_output_process();

  // Process interface
  virtual config_block params() const;
  virtual bool set_params(config_block const& blk);
  virtual bool initialize();
  virtual bool reset();
  virtual vidtk::process::step_status step2();

// This is the list of the data items that have to be synchronized
// over input and output.  Just add another line below to add another
// data value to be synchronized.

// (type, name, initial value, reserved)
#define FINALIZER_DATUM_LIST                                            \
  DATUM_DEF ( vidtk::timestamp,                  timestamp,               vidtk::timestamp(),                 0 ) \
  DATUM_DEF ( vidtk::timestamp::vector_t,        timestamp_vector,        vidtk::timestamp::vector_t(),       0 ) \
  DATUM_DEF ( vidtk::image_to_image_homography,  src_to_ref_homography,   vidtk::image_to_image_homography(), 0 ) \
  DATUM_DEF ( vidtk::image_to_utm_homography,    src_to_utm_homography,   vidtk::image_to_utm_homography(),   0 ) \
  DATUM_DEF ( vidtk::image_to_plane_homography,  ref_to_wld_homography,   vidtk::image_to_plane_homography(), 0 ) \
  DATUM_DEF ( double,                            world_units_per_pixel,   0.0,                                0 ) \
  DATUM_DEF ( vidtk::video_modality,             video_modality,          vidtk::VIDTK_INVALID,               0 ) \
  DATUM_DEF ( vidtk::shot_break_flags,           shot_break_flags,        vidtk::shot_break_flags(),           0 )

  // values that are not yet needed
  // DATUM_DEF ( vidtk::plane_to_utm_homography,    wld_to_utm_homography,   vidtk::plane_to_utm_homography(),   0 )


#define PROCESS_INPUT(T,N) void set_input_ ## N (T); VIDTK_OPTIONAL_INPUT_PORT (set_input_ ## N, T)
#define PROCESS_OUTPUT(T,N) T get_output_ ## N() const; VIDTK_OUTPUT_PORT (T, get_output_ ## N)
#define PROCESS_PASS_THRU(T,N) PROCESS_INPUT(T,N); PROCESS_OUTPUT(T,N)

  // ---- Process Inputs ----
  PROCESS_INPUT ( vidtk::track_vector_t  const&, active_tracks );
  PROCESS_INPUT ( vidtk::track_vector_t  const&, filtered_tracks );
  PROCESS_INPUT ( vidtk::track_vector_t  const&, linked_tracks );

  // ---- Process pass through ----
#define DATUM_DEF(T, N, I, R) PROCESS_PASS_THRU(T const& ,N);
  FINALIZER_DATUM_LIST // macro magic
#undef DATUM_DEF

  // ---- Process Outputs ----
  // The semantics of the track output ports going through finalize_output_process are 
  // unfortunately nontrivial. Here we go, 
  //
  // If the finalize_output_process is enabled, then
  //    - finalized_tracks() is usable and will publish active and finalized tracks.
  //    - terminated_tracks() is usable and will publish terminated and finalized tracks. 
  //    - filtered_tracks() is NOT usable and will publish terminated but not finalized tracks.
  //    - linked_tracks() is NOT usable and will publish active but not finalized tracks.
  // On the other hand, if the finalize_output_process is NOT enabled, then
  //    - finalized_tracks() is NOT usable and will publish active tracks.
  //    - terminated_tracks() is NOT usable and will publish ONLY EMPTY set of tracks. 
  //    - filtered_tracks() is usable and will publish terminated tracks.
  //    - linked_tracks() is usable and will publish active tracks.

  PROCESS_OUTPUT ( vidtk::track_vector_t const&, finalized_tracks );
  PROCESS_OUTPUT ( vidtk::track_view::vector_t const&, finalized_track_views );
  PROCESS_OUTPUT ( vidtk::track_vector_t const&, terminated_tracks );

  PROCESS_OUTPUT ( vidtk::track_vector_t  const&, filtered_tracks );
  PROCESS_OUTPUT ( vidtk::track_vector_t  const&, linked_tracks );

#undef PROCESS_INPUT
#undef PROCESS_OUTPUT
#undef PROCESS_PASS_THRU


  vcl_ostream& to_stream(vcl_ostream& str) const;


private:
  finalize_output_process_impl * m_impl;

}; // class finalize_output_process

} // end namespace vidtk

#endif // vidtk_finalize_output_process_h_

// Local Variables:
// mode: c++
// fill-column: 70
// c-tab-width: 2
// c-basic-offset: 2
// c-basic-indent: 2
// c-indent-tabs-mode: nil
// end:
