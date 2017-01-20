/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "frame_rate_estimator.h"

#include <vector>
#include <algorithm>
#include <limits>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "frame_rate_estimator" );

frame_rate_estimator
::frame_rate_estimator()
{
  this->configure( frame_rate_estimator_settings() );
}

bool
frame_rate_estimator
::configure( const frame_rate_estimator_settings& settings )
{
  local_time_diff_hist_.clear();
  source_time_diff_hist_.clear();

  last_local_time_ = 0;
  last_source_time_ = 0;

  if( settings_.buffer_length < 3 )
  {
    LOG_ERROR( "Buffer length must be greater than 2." );
    return false;
  }

  local_time_diff_hist_.set_capacity( settings.buffer_length );
  source_time_diff_hist_.set_capacity( settings.buffer_length );

  is_first_frame_ = true;

  settings_ = settings;

  if( settings_.enable_lag_estimator )
  {
    settings_.enable_source_fps_estimator = true;
    settings_.enable_local_fps_estimator = true;
  }

  if( settings_.cull_percentage >= 0.5 && settings_.cull_percentage < 0 )
  {
    LOG_ERROR( "Cull percentage must be between 0.0 and 0.5" );
    return false;
  }

  min_source_to_local_offset_ = std::numeric_limits< local_time_t >::max();
  return true;
}

// Note: starts to do a quick select, then switches to sort so still
// technically O( nlog(n) ), so while it could be modified to run in
// O( n ) time, it's generally run on a small number of samples.
frame_rate_estimator::local_time_t
compute_approx_median( const frame_rate_estimator::time_buffer_t& buffer )
{
  std::vector< frame_rate_estimator::local_time_t > left, right;

  const size_t pivot = buffer.size() / 2;
  const frame_rate_estimator::local_time_t pivot_value = buffer[ pivot ];

  for( unsigned i = 0; i < buffer.size(); i++ )
  {
    if( i != pivot )
    {
      if( buffer[i] < pivot_value )
      {
        left.push_back( buffer[i] );
      }
      else
      {
        right.push_back( buffer[i] );
      }
    }
  }

  if( left.size() > pivot )
  {
    std::sort( left.begin(), left.end() );
    return left[ pivot ];
  }
  else if( right.size() > pivot )
  {
    std::sort( right.begin(), right.end() );
    return right[ pivot - left.size() ];
  }
  return pivot_value;
}

frame_rate_estimator::local_time_t
frame_rate_estimator
::average_temporal_diff( const time_buffer_t& tbuf )
{
  std::vector< frame_rate_estimator::local_time_t > sorted_times;
  sorted_times.insert( sorted_times.begin(), tbuf.begin(), tbuf.end() );
  std::sort( sorted_times.begin(), sorted_times.end() );

  local_time_t summed_difference = 0;

  unsigned diff_ignore_count = static_cast<unsigned>(
    settings_.cull_percentage * sorted_times.size() );

  unsigned start_index = diff_ignore_count;
  size_t end_index = sorted_times.size() - diff_ignore_count;

  for( size_t i = start_index; i < end_index; ++i )
  {
    summed_difference += sorted_times[i];
  }

  if( end_index > start_index )
  {
    return summed_difference / ( end_index - start_index );
  }

  return 0;
}

frame_rate_estimator_output
frame_rate_estimator
::step( const vidtk::timestamp& ts )
{
  frame_rate_estimator_output output;

  local_time_t source_time_ms = 0.0;
  local_time_t local_time_ms = 0.0;

  if( !ts.has_time() )
  {
    LOG_ERROR( "Input contains an invalid time, skipping." );
    return output;
  }

  if( is_first_frame_ )
  {
    start_local_time_ = boost::posix_time::microsec_clock::local_time();
    start_source_time_ = ts.time() / 1000;
    is_first_frame_ = false;
    return output;
  }

  if( settings_.enable_source_fps_estimator )
  {
    source_time_ms = ( ts.time() / 1000 ) - start_source_time_;

    source_time_diff_hist_.push_back( source_time_ms - last_source_time_ );
    last_source_time_ = source_time_ms;

    local_time_t est_frame_diff = 0;

    if( settings_.fps_estimator_mode == frame_rate_estimator_settings::MEDIAN )
    {
      est_frame_diff = compute_approx_median( source_time_diff_hist_ );
    }
    else
    {
      est_frame_diff = average_temporal_diff( source_time_diff_hist_ );
    }
    if( est_frame_diff > 0 )
    {
      output.source_fps = 1000.0 / est_frame_diff;
    }
  }

  if( settings_.enable_local_fps_estimator )
  {
    boost::posix_time::ptime current_time = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration diff = current_time - start_local_time_;
    local_time_ms = static_cast<double>( diff.total_milliseconds() );

    local_time_diff_hist_.push_back( local_time_ms - last_local_time_ );
    last_local_time_ = local_time_ms;

    double est_frame_diff = 0;

    if( settings_.fps_estimator_mode == frame_rate_estimator_settings::MEDIAN )
    {
      est_frame_diff = compute_approx_median( local_time_diff_hist_ );
    }
    else
    {
      est_frame_diff = average_temporal_diff( local_time_diff_hist_ );
    }

    if( est_frame_diff > 0 )
    {
      output.local_fps = 1000 / est_frame_diff;
    }
  }

  if( settings_.enable_lag_estimator )
  {
    double current_diff = local_time_ms - source_time_ms;

    if( current_diff < min_source_to_local_offset_ )
    {
      min_source_to_local_offset_ = current_diff;
    }

    output.estimated_lag = current_diff - min_source_to_local_offset_;
  }

  return output;
}

}
