/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_pyramid_process.h"

#include "klt_util.h"
#include <kwklt/klt_mutex.h>

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

namespace vidtk
{

klt_pyramid_process::klt_pyramid_process(vcl_string const& name)
  : process(name, "klt_pyramid_process")
  , disabled_(false)
  , subsampling_(4)
  , levels_(2)
  , init_sigma_(0.7)
  , sigma_factor_(0.9)
  , grad_sigma_(1.0)
{
  config_.add("levels", "2");
  config_.add("subsampling", "4");
  config_.add("init_sigma", "0.7");
  config_.add("sigma_factor", "0.9");
  config_.add("grad_sigma", "1.0");
  config_.add("disabled", "false");
}

klt_pyramid_process::~klt_pyramid_process()
{
}

config_block klt_pyramid_process::params() const
{
  return config_;
}

bool klt_pyramid_process::set_params(config_block const& blk)
{
  try
  {
    blk.get("levels", levels_);
    blk.get("subsampling", subsampling_);
    blk.get("init_sigma", init_sigma_);
    blk.get("sigma_factor", sigma_factor_);
    blk.get("grad_sigma", grad_sigma_);
    blk.get("disabled", disabled_);
  }
  catch(unchecked_return_value& e)
  {
    log_error( this->name() << ": couldn't set parameters: "<< e.what() <<"\n" );

    return false;
  }

  config_.update(blk);
  return true;
}

bool klt_pyramid_process::initialize()
{
  img_ = vil_image_view<vxl_byte>();

  return true;
}

bool klt_pyramid_process::step()
{
  if (disabled_)
  {
    return true;
  }

  if (!img_)
  {
    return false;
  }


  // critical region
  boost::lock_guard<boost::mutex> lock(vidtk::klt_mutex::instance()->get_lock());

  pyramid_ = create_klt_pyramid(img_, levels_, subsampling_, sigma_factor_, init_sigma_);

  pgradx_ = vil_pyramid_image_view<float>();
  pgrady_ = vil_pyramid_image_view<float>();

  double scale = 1;

  for (int i = 0; i < levels_; ++i)
  {
    std::pair<vil_image_view<float>, vil_image_view<float> > const grads = compute_gradients(pyramid_(i), grad_sigma_);

    vil_image_view_base_sptr gradx(new vil_image_view<float>(grads.first));
    pgradx_.add_view(gradx, scale);

    vil_image_view_base_sptr grady(new vil_image_view<float>(grads.second));
    pgrady_.add_view(grady, scale);

    scale /= subsampling_;
  }

  img_ = vil_image_view<vxl_byte>();

  return true;
}

bool klt_pyramid_process::reset()
{
  return true;
}

void klt_pyramid_process::set_image(vil_image_view<vxl_byte> const& img)
{
  img_ = img;
}

vil_pyramid_image_view<float> const& klt_pyramid_process::image_pyramid() const
{
  return pyramid_;
}

vil_pyramid_image_view<float> const& klt_pyramid_process::image_pyramid_gradx() const
{
  return pgradx_;
}

vil_pyramid_image_view<float> const& klt_pyramid_process::image_pyramid_grady() const
{
  return pgrady_;
}

} // end namespace vidtk
