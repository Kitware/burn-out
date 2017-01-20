/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_rate_estimator_h_
#define vidtk_frame_rate_estimator_h_

#include <utilities/timestamp.h>

#include <boost/circular_buffer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace vidtk
{


/// Settings for the frame rate estimator.
struct frame_rate_estimator_settings
{
  /// Should we enable the source frames per second (fps) estimator which
  /// looks at the input timestamps to try and estimate the source rate?
  bool enable_source_fps_estimator;

  /// Should we enabled the local frames per second (fps) estimator which
  /// looks at the local wall clock to try and estimate the source rate?
  bool enable_local_fps_estimator;

  /// Should we enabled the local frames per second (fps) estimator which
  /// looks at the local wall clock to try and estimate the source rate?
  bool enable_lag_estimator;

  /// Buffer length used for frame rate estimation.
  unsigned buffer_length;

  /// Percentage of entries in our buffer [0,0.5] to remove from consideration
  /// for estimating the current frame rate. Used for noise suppression.
  double cull_percentage;

  /// Source FPS estimator mode.
  enum { MEAN, MEDIAN } fps_estimator_mode;

  // Set defaults.
  frame_rate_estimator_settings()
  : enable_source_fps_estimator( true ),
    enable_local_fps_estimator( false ),
    enable_lag_estimator( false ),
    buffer_length( 50 ),
    cull_percentage( 0.05 ),
    fps_estimator_mode( MEAN )
  {}
};


/// Possible outputs for the frame_rate_estimator step function.
struct frame_rate_estimator_output
{
  /// The estimated source fps.
  double source_fps;

  /// The estimated local fps.
  double local_fps;

  /// Estimated lag (in milliseconds) of the local stream behind the source.
  double estimated_lag;

  frame_rate_estimator_output()
  : source_fps( -1.0 ),
    local_fps( -1.0 ),
    estimated_lag( -1.0 )
  {}
};


/// A class used for estimating input and processing frame rates. It's useful
/// for optimally determining if we should downsample the current input stream
/// to meet some criteria.
class frame_rate_estimator
{

public:

  typedef frame_rate_estimator_output output_t;
  typedef double local_time_t;
  typedef boost::circular_buffer< local_time_t > time_buffer_t;

  frame_rate_estimator();
  virtual ~frame_rate_estimator() {}

  bool configure( const frame_rate_estimator_settings& settings );

  output_t step( const vidtk::timestamp& ts );

private:

  // Internal buffers
  time_buffer_t source_time_diff_hist_;
  time_buffer_t local_time_diff_hist_;

  // Stored times from the last step call
  local_time_t last_source_time_;
  local_time_t last_local_time_;

  // The minimum estimated difference between local time and device time
  // ever received by this process.
  local_time_t min_source_to_local_offset_;

  // Is this the first frame observed?
  bool is_first_frame_;

  // Start time computed when we receive the first frame
  local_time_t start_source_time_;
  boost::posix_time::ptime start_local_time_;

  // Copy of externally set settings
  frame_rate_estimator_settings settings_;

  // Compute the average temporal difference between frames
  local_time_t average_temporal_diff( const time_buffer_t& tbuf );
};


} // namespace vidtk

#endif // vidtk_frame_rate_estimator_h_
