/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_tracking_process.h"

#include "klt_tracking_process_impl_klt.h"

#include "klt_util.h"

#include <klt/pyramid.h>

#include <utilities/log.h>
#include <utilities/timestamp.h>
#include <utilities/unchecked_return_value.h>

#include <boost/none.hpp>

namespace vidtk
{

klt_tracking_process::klt_tracking_process(vcl_string const& name)
  : process(name, "klt_tracking_process")
  , impl_(NULL)
  , has_first_frame_(false)
{
}

klt_tracking_process::~klt_tracking_process()
{
  delete impl_;
}

config_block klt_tracking_process::params() const
{
  config_block config_ = klt_tracking_process_impl::params();

  config_.add("impl", "klt");

  return config_;
}

bool klt_tracking_process::set_params(config_block const& blk)
{
  try
  {
    vcl_string impl_type;
    blk.get("impl", impl_type);

    if (impl_type == "klt")
    {
      impl_ = new klt_tracking_process_impl_klt;
    }
    else
    {
      log_error( this->name() << ": Unknown klt implementation: "<< impl_type <<"\n" );

      return false;
    }

    if (impl_)
    {
      impl_->set_params(blk);
    }
  }
  catch(unchecked_return_value& e)
  {
    log_error( this->name() << ": couldn't set parameters: "<< e.what() <<"\n" );

    return false;
  }

  return true;
}

bool klt_tracking_process::initialize()
{
  const bool init = impl_->initialize();

  if (!init)
  {
    return false;
  }

  return reinitialize();
}

bool klt_tracking_process::reinitialize()
{
  has_first_frame_ = false;

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

bool klt_tracking_process::reset()
{
  this->reinitialize();

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

vcl_vector<klt_track_ptr> const& klt_tracking_process::active_tracks() const
{
  return impl_->active_;
}

vcl_vector<klt_track_ptr> const& klt_tracking_process::terminated_tracks() const
{
  return impl_->terminated_;
}

vcl_vector<klt_track_ptr> const& klt_tracking_process::created_tracks() const
{
  return impl_->created_;
}

} // end namespace vidtk
