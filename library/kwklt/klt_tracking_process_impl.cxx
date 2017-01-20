/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_tracking_process_impl.h"

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_klt_tracking_process_impl_cxx__
VIDTK_LOGGER("klt_tracking_process_impl_cxx");


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

  config_.add_parameter("feature_count", "100", "The maximum number of feature points to track");
  config_.add_parameter("min_feature_count_percent", ".80",
      "The minimum percentage of feature points, below which new feature points to fill up to feature_count will be found");
  config_.add_parameter("window_width", "7", "The width of the search window for correspondence (must be odd)");
  config_.add_parameter("window_height", "7", "The height of the search window for correspondence (must be odd)");
  config_.add_parameter("min_distance", "10", "The minimum distance allowed between two feature points");
  config_.add_parameter("num_skipped_pixels", "0", "The number of pixels to skip when searching for feature points");
  config_.add_parameter("disabled", "false", "Self-explanitory");

  return config_;
}

bool klt_tracking_process_impl::set_params(config_block const& blk)
{
  //Not doing a try catch block here so the exception will be passed up
  //to the klt_tracking_process.  That we will have the name of the process.
  disabled_ = blk.get<bool>("disabled");
  if(!disabled_)
  {
    feature_count_ = blk.get<int>("feature_count");
    double min_feature_count_percent = blk.get<double>("min_feature_count_percent");
    window_width_ = blk.get<int>("window_width");
    window_height_ = blk.get<int>("window_height");
    min_distance_ = blk.get<int>("min_distance");
    num_skipped_pixels_ = blk.get<int>("num_skipped_pixels");

    if (min_feature_count_percent <= 0 || 1 < min_feature_count_percent)
    {
      throw config_block_parse_error(" min_feature_count_percent must be within (0, 1].");
    }
    min_feature_count_ = static_cast<int>(min_feature_count_percent * feature_count_);
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
