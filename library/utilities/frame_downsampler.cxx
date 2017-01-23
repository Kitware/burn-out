/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "frame_downsampler.h"

#include <limits>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER( "frame_downsampler_cxx" );

frame_downsampler
::frame_downsampler()
{
  this->reset_counters();

  known_frame_rate_ = 0;
  known_frame_count_ = 0;
  target_frame_rate_ = 0;
  output_frame_count_ = 0;
}

void
frame_downsampler
::reset_counters()
{
  initial_ignore_in_progress_ = true;
  current_ds_factor_ = 1.0;
  ds_counter_ = 0.0;
  min_ds_factor_ = 1.0;
  max_ds_factor_ = 10.0;
  locked_frame_counter_ = 0;
  burst_skip_mode_ = false;
  burst_counter_ = 0;
}

bool
frame_downsampler
::configure( const frame_downsampler_settings& settings )
{
  reset_counters();

  if( settings.buffer_length < 3 )
  {
    LOG_ERROR( "Buffer_length must be greater than 2!" );
    return false;
  }

  frame_rate_estimator_settings fr_settings;
  fr_settings.enable_source_fps_estimator = settings.rate_limiter_enabled;
  fr_settings.enable_local_fps_estimator = settings.adaptive_downsampler_enabled;
  fr_settings.enable_lag_estimator = settings.adaptive_downsampler_enabled;
  fr_settings.buffer_length = settings.buffer_length;

  if( settings.rate_estimation_mode == frame_downsampler_settings::MEAN )
  {
    fr_settings.fps_estimator_mode = frame_rate_estimator_settings::MEAN;
  }
  else if( settings.rate_estimation_mode == frame_downsampler_settings::MEDIAN )
  {
    fr_settings.fps_estimator_mode = frame_rate_estimator_settings::MEDIAN;
  }

  if( !fr_estimator_.configure( fr_settings ) )
  {
    LOG_ERROR( "Unable to configure frame rate estimator" );
    return false;
  }

  max_ds_factor_ = settings.max_downsample_factor;
  settings_ = settings;
  recompute_target_fr();
  return true;
}

bool
frame_downsampler
::drop_current_frame( const vidtk::timestamp& ts )
{
  // Update FPS estimate
  frame_rate_estimator_output rate_estimates = fr_estimator_.step( ts );

  // Over-ride computed FPS if known value is set
  if( known_frame_rate_ > 0 )
  {
    rate_estimates.source_fps = known_frame_rate_;
  }
  // Handle initial ignore
  else if( initial_ignore_in_progress_ )
  {
    locked_frame_counter_ += 1;

    if( locked_frame_counter_ > settings_.initial_ignore_count )
    {
      initial_ignore_in_progress_ = false;
      locked_frame_counter_ = 0;
    }
    else
    {
      return true;
    }
  }

  // Record last downsampling rate for logging
  double last_ds_factor = current_ds_factor_;

  // Handle rate limiting
  if( locked_frame_counter_ > 0 )
  {
    locked_frame_counter_--;
  }
  else
  {
    if( settings_.rate_limiter_enabled &&
        rate_estimates.source_fps > target_frame_rate_ )
    {
      if( !settings_.known_fps.empty() )
      {
        double min_dist = std::numeric_limits<double>::max();
        unsigned min_ind = 0;

        for( unsigned i = 0; i < settings_.known_fps.size(); i++ )
        {
          double dist = std::abs( settings_.known_fps[i]-rate_estimates.source_fps );

          if( dist < min_dist )
          {
            min_ind = i;
            min_dist = dist;
          }
        }

        min_ds_factor_ = settings_.known_fps[min_ind] / target_frame_rate_;
      }
      else
      {
        min_ds_factor_ = rate_estimates.source_fps / target_frame_rate_;
      }

      if( min_ds_factor_ < 1.0 )
      {
        min_ds_factor_ = 1.0;
      }
      if( min_ds_factor_ > max_ds_factor_ )
      {
        min_ds_factor_ = max_ds_factor_;
      }

      if( !settings_.adaptive_downsampler_enabled )
      {
        current_ds_factor_ = min_ds_factor_;
      }
    }

    if( settings_.adaptive_downsampler_enabled )
    {
      if( rate_estimates.estimated_lag > settings_.lag_threshold )
      {
        current_ds_factor_++;
      }
      else if( rate_estimates.estimated_lag < settings_.lag_threshold )
      {
        current_ds_factor_--;
      }
      if( current_ds_factor_ > max_ds_factor_ )
      {
        current_ds_factor_ = max_ds_factor_;
      }
      if( current_ds_factor_ < min_ds_factor_ )
      {
        current_ds_factor_ = min_ds_factor_;
      }
    }
  }

  // Perform actual downsampling
  if( current_ds_factor_ != last_ds_factor )
  {
    locked_frame_counter_ = settings_.lock_rate_frame_count;

    LOG_INFO( "Changed downsampling factor from " <<
              last_ds_factor << " to " << current_ds_factor_ );
  }

  ds_counter_ += 1.0;

  if( ds_counter_ < current_ds_factor_ )
  {
    return true;
  }
  else
  {
    ds_counter_ = std::fmod( ds_counter_, current_ds_factor_ );
  }

  if( settings_.burst_frame_break != 0 && settings_.burst_frame_count != 0 )
  {
    burst_counter_++;

    if( burst_skip_mode_ )
    {
      if( burst_counter_ > settings_.burst_frame_break )
      {
        burst_counter_ = 0;
        burst_skip_mode_ = false;
      }

      return true;
    }
    else if( burst_counter_ > settings_.burst_frame_count )
    {
      burst_counter_ = 0;
      burst_skip_mode_ = true;
    }
  }

  return false;
}

bool
frame_downsampler
::in_burst_break() const
{
  return burst_skip_mode_;
}

void
frame_downsampler
::set_frame_rate( const double frame_rate )
{
  known_frame_rate_ = frame_rate;
  recompute_target_fr();
}

void
frame_downsampler
::set_frame_count( const unsigned frame_count )
{
  known_frame_count_ = frame_count;
  recompute_target_fr();
}

double
frame_downsampler
::output_frame_rate() const
{
  return target_frame_rate_;
}

unsigned
frame_downsampler
::output_frame_count() const
{
  return output_frame_count_;
}

void
frame_downsampler
::recompute_target_fr()
{
  // Compute target (output) frame rate
  if( known_frame_rate_ > 0 )
  {
    if( settings_.retain_as_multiple )
    {
      double drop_rate = std::ceil( known_frame_rate_ / settings_.rate_threshold );
      target_frame_rate_ = known_frame_rate_ / drop_rate;
    }
    else
    {
      target_frame_rate_ = settings_.rate_threshold;
    }
  }
  else
  {
    target_frame_rate_ = settings_.rate_threshold;
  }

  if( known_frame_count_ > 0 && known_frame_rate_ > 0 )
  {
    output_frame_count_ = 1 + known_frame_count_ * ( target_frame_rate_ / known_frame_rate_ );
  }
}

}
