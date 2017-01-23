/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_tracking_process.h"

#include "klt_tracking_process_impl_klt.h"

#include "klt_util.h"

#include <klt/pyramid.h>

#include <utilities/timestamp.h>

#include <boost/none.hpp>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_klt_tracking_process_cxx__
VIDTK_LOGGER("klt_tracking_process_cxx");


namespace vidtk
{

klt_tracking_process::klt_tracking_process(std::string const& _name)
  : process(_name, "klt_tracking_process")
  , impl_(NULL)
  , has_first_frame_(false)
{
}

klt_tracking_process::~klt_tracking_process()
{
}

config_block klt_tracking_process::params() const
{
  config_block config_ = klt_tracking_process_impl::params();

  config_.add_parameter("impl", "klt", "UNDOCUMENTED");

  return config_;
}

bool klt_tracking_process::set_params(config_block const& blk)
{
  try
  {
    std::string impl_type = blk.get<std::string>("impl");

    if (impl_type == "klt")
    {
      impl_.reset(new klt_tracking_process_impl_klt);
    }
    else
    {
      throw config_block_parse_error( " Unknown klt implementation: " + impl_type );
    }

    if (impl_)
    {
      impl_->set_params(blk);
    }
  }
  catch(config_block_parse_error& e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );
    return false;
  }

  return true;
}

bool klt_tracking_process::initialize()
{
  if (impl_->disabled_)
  {
    return true;
  }

  const bool init = impl_->initialize();

  if (!init)
  {
    return false;
  }

  return this->reset();
}

bool klt_tracking_process::reset()
{
  has_first_frame_ = false;

  if (impl_->disabled_)
  {
    return true;
  }

  return impl_->reinitialize();
}

bool klt_tracking_process::step()
{
  if (impl_->disabled_)
  {
    return false;
  }

  if (!impl_->is_ready())
  {
    return false;
  }

  if (has_first_frame_)
  {
    const int count = impl_->track_features();

    if (count < impl_->min_feature_count_)
    {
      impl_->replace_features();
    }
  }
  else
  {
    impl_->select_features();

    has_first_frame_ = true;
  }

  impl_->post_step();

  return true;
}

void klt_tracking_process::set_image_pyramid(vil_pyramid_image_view<float> const& img)
{
  impl_->set_image_pyramid(img);
}

void klt_tracking_process::set_image_pyramid_gradx(vil_pyramid_image_view<float> const& img)
{
  impl_->set_image_pyramid_gradx(img);
}

void klt_tracking_process::set_image_pyramid_grady(vil_pyramid_image_view<float> const& img)
{
  impl_->set_image_pyramid_grady(img);
}

void klt_tracking_process::set_timestamp(vidtk::timestamp const& ts)
{
  impl_->set_timestamp(ts);
}

void klt_tracking_process::set_homog_predict(vgl_h_matrix_2d<double> const& h)
{
  impl_->set_homog_predict(h);
}

std::vector<klt_track_ptr> klt_tracking_process::active_tracks() const
{
  return impl_->active_;
}

std::vector<klt_track_ptr> klt_tracking_process::terminated_tracks() const
{
  return impl_->terminated_;
}

std::vector<klt_track_ptr> klt_tracking_process::created_tracks() const
{
  return impl_->created_;
}

} // end namespace vidtk
