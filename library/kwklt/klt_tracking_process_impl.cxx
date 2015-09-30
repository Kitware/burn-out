/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_tracking_process_impl.h"

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>

namespace vidtk
{

klt_tracking_process_impl::klt_tracking_process_impl()
{
}

klt_tracking_process_impl::~klt_tracking_process_impl()
{
}

config_block klt_tracking_process_impl::params()
{
  config_block config_;

  config_.add("feature_count", "100");
  config_.add("min_feature_count_percent", ".80");
  config_.add("window_width", "7");
  config_.add("window_height", "7");
  config_.add("min_distance", "10");
  config_.add("num_skipped_pixels", "0");
  config_.add("search_range", "15");
  config_.add("disabled", "false");

  return config_;
}

bool klt_tracking_process_impl::set_params(config_block const& blk)
{
  try
  {
    double min_feature_count_percent;

    blk.get("feature_count", feature_count_);
    blk.get("min_feature_count_percent", min_feature_count_percent);
    blk.get("window_width", window_width_);
    blk.get("window_height", window_height_);
    blk.get("min_distance", min_distance_);
    blk.get("num_skipped_pixels", num_skipped_pixels_);
    blk.get("search_range", search_range_);
    blk.get("disabled", disabled_);

    if (min_feature_count_percent <= 0 || 1 < min_feature_count_percent)
    {
      log_error("klt_tracking_process: min_feature_count_percent must be within (0, 1].");
      return false;
    }

    min_feature_count_ = min_feature_count_percent * feature_count_;
  }
  catch(unchecked_return_value&)
  {
    return false;
  }

  return true;
}

bool klt_tracking_process_impl::initialize()
{
  reinitialize();

  return true;
}

bool klt_tracking_process_impl::reinitialize()
{
  return true;
}

bool klt_tracking_process_impl::is_ready()
{
  active_.clear();
  terminated_.clear();
  created_.clear();

  return true;
}

void klt_tracking_process_impl::post_step()
{
}

} // end namespace vidtk
