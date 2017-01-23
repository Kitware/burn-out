/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gui_frame_info_h_
#define vidtk_gui_frame_info_h_

#include <utilities/timestamp.h>

#include <tracking_data/gui_track_signal.h>

#include <ostream>

namespace vidtk
{

/// \brief Any GUI related information we want to propogate down the pipeline
/// for each individual frame.
///
/// In order for processes to adapt their behavior accordingly, this class
/// contains 3 pieces of information. First, a list of all GUI track-related
/// signals with a timestamp approximately matching the current frame is provided.
/// Secondly, a flag is specified which states whether or not to it would be
/// beneficial to fully process the given frame based on the last signal, or
/// whether or not only buffering operations should be performed (for example,
/// maybe we should only buffer frames for later image differencing but not detect
/// anything). Lastly, a couple other temporal properties are contained including
/// if on this frame a temporal discontinuity was created in order to process
/// a GUI signal received in the past.
class gui_frame_info
{
public:

  enum recommended_action { BUFFER = 0,
                            PROCESS,
                            RESET,
                            RESET_AND_BUFFER,
                            RESET_AND_PROCESS };

  gui_frame_info();
  gui_frame_info( const gui_track_signal& signal );

  /// Any GUI signals which have a timestamp matching the current frame.
  gui_track_signal_list const& frame_signals() const;

  /// In order to process the last GUI signal, do we need to fully process
  /// and track on this frame, or just perform any buffering operations?
  recommended_action action() const;

  /// Does this frame contain any track signals corresponding to this frame?
  bool has_frame_signals() const;

  /// A flag indicating that due to the contents of some particular gui signal
  /// we were required to start reprocessing frames from the past, resulting
  /// in a temporal discontinuity starting on the current frame for this tracker.
  /// This flag will be set for the first frame after this discontinuity only.
  bool temporal_discontinuity_flag() const;

  /// If known, this value indicates the number of frames before a frame with
  /// a GUI click will be provided. Often, when processing GUI signals, we will
  /// send a few frames before a frame with a signal to help perform related
  /// buffering operations which require past frames.
  unsigned frames_until_next_signal() const;

  /// If known, this flag indicates the number of frames in the past this message
  /// was formulated on. Useful if we have a buffer of recent frames, and we
  /// receive a message in the past. Downstream processes can then respond
  /// to this accordingly to speed up processing and reach real-time.
  unsigned frames_under_realtime() const;

  /// Helper functions to set internal state
  void add_signal( const gui_track_signal& sig );
  void set_action( gui_frame_info::recommended_action rec_action );
  void set_temporal_discontinuity_flag( const bool flag = true );
  void set_frames_until_signal( const unsigned count );
  void set_frames_under_realtime( const unsigned count );

protected:

  void reset_variables();

  gui_track_signal_list frame_signals_;
  recommended_action recommended_action_;
  bool temporal_discontinuity_flag_;
  unsigned frames_until_signal_;
  unsigned frames_under_realtime_;

};

/// Output stream operator for the gui_frame_info class.
std::ostream& operator<<( std::ostream& str, const gui_frame_info& obj );

}

#endif
