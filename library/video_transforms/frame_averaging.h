/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_averaging_helpers_h_
#define vidtk_frame_averaging_helpers_h_

#include <utilities/ring_buffer.h>
#include <vil/vil_image_view.h>

/// @file frame_averaging.h
/// @brief Functions for performing frame averaging in a variety of ways

namespace vidtk
{


/// Base class for all online frame averagers instances
template <typename PixType>
class online_frame_averager
{

public:

  online_frame_averager() : should_round_(false) {}
  virtual ~online_frame_averager() {}

  /// Process a new frame, returning the current frame average.
  virtual void process_frame( const vil_image_view< PixType >& input,
                              vil_image_view< PixType >& average ) = 0;

  /// Process a new frame, and additionally compute a per-pixel instantaneous
  /// variance estimation, which can be further average to estimate the per-pixel
  /// variance over x frames.
  void process_frame( const vil_image_view< PixType >& input,
                      vil_image_view< PixType >& average,
                      vil_image_view< double >& variance );

  /// Reset the internal average.
  virtual void reset() = 0;

protected:

  /// Should we spend a little bit of extra time rounding outputs?
  bool should_round_;

  /// The last average in double form
  vil_image_view<double> last_average_;

  /// Is the resolution of the input image different from prior inputs?
  bool has_resolution_changed( const vil_image_view< PixType >& input );

private:

  /// Temporary buffers used for variance calculations if they're enabled
  vil_image_view<double> dev1_tmp_space_;
  vil_image_view<double> dev2_tmp_space_;

};

/// A cumulative frame averager
template <typename PixType>
class cumulative_frame_averager : public online_frame_averager<PixType>
{

public:

  cumulative_frame_averager( const bool should_round = false );
  virtual ~cumulative_frame_averager() {}

  /// Process a new frame, returning the current frame average.
  virtual void process_frame( const vil_image_view< PixType >& input,
                              vil_image_view< PixType >& average );

  /// Reset the internal average.
  virtual void reset();

protected:

  /// The number of observed frames since the last reset
  unsigned frame_count_;

};

/// An exponential frame averager
template <typename PixType>
class exponential_frame_averager : public online_frame_averager<PixType>
{

public:

  exponential_frame_averager( const bool should_round = false,
                              const double new_frame_weight = 0.5 );

  virtual ~exponential_frame_averager() {}

  /// Process a new frame, returning the current frame average.
  virtual void process_frame( const vil_image_view< PixType >& input,
                              vil_image_view< PixType >& average );

  /// Reset the internal average.
  virtual void reset();

protected:

  /// The exponential averaging coefficient
  double new_frame_weight_;

  /// The number of observed frames since the last reset
  unsigned frame_count_;
};

/// A windowed frame averager
template <typename PixType>
class windowed_frame_averager : public online_frame_averager<PixType>
{

public:

  typedef vil_image_view< PixType > input_type;

  windowed_frame_averager( const bool should_round = false,
                           const unsigned window_length = 20 );

  virtual ~windowed_frame_averager() {}

  /// Process a new frame, returning the current frame average.
  virtual void process_frame( const vil_image_view< PixType >& input,
                              vil_image_view< PixType >& average );

  /// Reset the internal average.
  virtual void reset();

  /// Get number of frames used in the current window
  virtual unsigned frame_count() const;

protected:

  /// Buffer containing pointers to last window_length frames
  ring_buffer< vil_image_view< PixType > > window_buffer_;

};

} // end namespace vidtk


#endif // vidtk_frame_averaging_process_h_
