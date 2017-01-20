/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_downsampler_h_
#define vidtk_frame_downsampler_h_

#include <utilities/frame_rate_estimator.h>
#include <utilities/external_settings.h>

namespace vidtk
{

#define settings_macro( add_param, add_array, add_enumr ) \
  add_param( \
    rate_limiter_enabled, \
    bool, \
    false, \
    "Should we enable the pipeline rate limiter? This option will " \
    "attempt to make the source FPS as close as possible to the " \
    "given threshold, using only values in supplied timestamps." ); \
  add_param( \
    rate_threshold, \
    double, \
    10.0, \
    "If our apparent source fps is above this threshold, we will " \
    "attempt to drop frames to match this target rate as close as " \
    "possible." ); \
  add_param( \
    retain_as_multiple, \
    bool, \
    false, \
    "If this flag is set, then we will only drop frames at a certain " \
    "multiple of the input FPS, e.g. every 2 frames, every 3 frames, " \
    "etc." ); \
  add_param( \
    adaptive_downsampler_enabled, \
    bool, \
    false, \
    "Should we enable the adaptive frame downsampler which attempts " \
    "to measure the current lag of this node behind the head of the " \
    "stream, and engages in more aggressive downsampling if we lag " \
    "behind some threshold?" ); \
  add_param( \
    lag_threshold, \
    double, \
    2000.0, \
    "Time threshold (in ms) to start more aggressively downsampling " \
    "if this process lags this many milliseconds behind the input " \
    "stream." ); \
  add_param( \
    buffer_length, \
    unsigned, \
    50, \
    "Buffer length used to estimate the current frame rate." ); \
  add_param( \
    max_downsample_factor, \
    double, \
    10.0, \
    "Maximum downsampling factor to use no matter what." ); \
  add_param( \
    lock_rate_frame_count, \
    unsigned, \
    100, \
    "After a change in the downsampling factor, lock the downsampling " \
    "factor for at least this many frames before changing it again." ); \
  add_enumr( \
    rate_estimation_mode, \
    (MEAN) (MEDIAN), \
    MEAN, \
    "Whether or not we should take the mean or median of the buffer " \
    "to estimate various frame rates" ); \
  add_param( \
    initial_ignore_count, \
    unsigned, \
    5, \
    "Number of frames to wait before starting to downsample." ); \
  add_param( \
    burst_frame_break, \
    unsigned, \
    0, \
    "If non-zero, burst mode will be enabled which sends frames downstream " \
    "in groups with gaps between them. This parameter represents the gap in " \
    "terms of number of frames, after any other downsampling." ); \
  add_param( \
    burst_frame_count, \
    unsigned, \
    0, \
    "Number of frames in a single frame burst to keep, if in burst mode." ); \
  add_array( \
    known_fps, \
    double, \
    0, \
    "", \
    "A vector of known possible FPS priors. If this vector is not " \
    "empty, the measured frame rate will be mapped to one of these " \
    "categories and the mapped value will be used as the FPS." ); \

init_external_settings3( frame_downsampler_settings, settings_macro );

#undef settings_macro


/// A class which uses an estimation for the input frame to determine
class frame_downsampler
{

public:

  frame_downsampler();
  virtual ~frame_downsampler() {}

  /// Set any external options for this class.
  bool configure( const frame_downsampler_settings& settings );

  /// Should we drop the current frame, given it's timestamp?
  bool drop_current_frame( const vidtk::timestamp& ts );

  /// Are we currently dropping frames due to a burst mode break?
  bool in_burst_break() const;

  /// Set a known input frame rate, if known from an external source.
  void set_frame_rate( const double frame_rate );

  /// Set a known total frame count, if known from an extenal source.
  void set_frame_count( const unsigned frame_count );

  /// Estimated output frame rate produced by this process.
  double output_frame_rate() const;

  /// Estimated total number of frames which will be passed by this
  /// process if a total frame count is set. This is an estimate.
  unsigned output_frame_count() const;

private:

  // Any external settings
  frame_downsampler_settings settings_;

  // Internal parameters and counters
  bool initial_ignore_in_progress_;

  // Counters related to the current downsampling rate for adaptive ds mode
  double current_ds_factor_;
  double ds_counter_;
  double min_ds_factor_;
  double max_ds_factor_;
  unsigned locked_frame_counter_;
  bool burst_skip_mode_;
  unsigned burst_counter_;

  // Externally set input properties, if known
  double known_frame_rate_;
  unsigned known_frame_count_;

  // Output frame rate and total estimated frame count
  double target_frame_rate_;
  unsigned output_frame_count_;

  // The internal frame estimator class
  frame_rate_estimator fr_estimator_;

  // Reset all internal counters
  void reset_counters();

  // Recompute estimate target and output frame rates, output frame count
  void recompute_target_fr();
};

}

#endif // vidtk_downsampling_process_h_
