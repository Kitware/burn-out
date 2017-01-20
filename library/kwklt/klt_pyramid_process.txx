/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_klt_pyramid_process_txx_
#define vidtk_klt_pyramid_process_txx_

#include <limits>

#include "klt_pyramid_process.h"

#include "klt_util.h"
#include <kwklt/klt_mutex.h>

#include <vil/vil_convert.h>
#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_klt_pyramid_process_cxx__
VIDTK_LOGGER("klt_pyramid_process_cxx");

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

namespace vidtk
{

template< class PixType >
klt_pyramid_process<PixType>::klt_pyramid_process(std::string const& _name)
  : process(_name, "klt_pyramid_process")
  , disabled_(false)
  , subsampling_(4)
  , levels_(2)
  , init_sigma_(0.7f)
  , sigma_factor_(0.9f)
  , grad_sigma_(1.0f)
{
  config_.add_parameter("levels", "2", "UNDOCUMENTED");
  config_.add_parameter("subsampling", "4", "UNDOCUMENTED");
  config_.add_parameter("init_sigma", "0.7", "UNDOCUMENTED");
  config_.add_parameter("sigma_factor", "0.9", "UNDOCUMENTED");
  config_.add_parameter("grad_sigma", "1.0", "UNDOCUMENTED");
  config_.add_parameter("disabled", "false", "UNDOCUMENTED");
}

template< class PixType >
klt_pyramid_process<PixType>::~klt_pyramid_process()
{
}

template< class PixType >
config_block klt_pyramid_process<PixType>::params() const
{
  return config_;
}

template< class PixType >
bool klt_pyramid_process<PixType>::set_params(config_block const& blk)
{
  try
  {
    disabled_ = blk.get<bool>("disabled");
    if(!disabled_)
    {
      levels_ = blk.get<int>("levels");
      subsampling_ = blk.get<int>("subsampling");
      init_sigma_ = blk.get<float>("init_sigma");
      sigma_factor_ = blk.get<float>("sigma_factor");
      grad_sigma_ = blk.get<float>("grad_sigma");
    }
  }
  catch(config_block_parse_error& e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );

    return false;
  }

  config_.update(blk);
  return true;
}

template< class PixType >
bool klt_pyramid_process<PixType>::initialize()
{
  //Currently, we are using vxl_byte because the KLT code make way too many 8 bit image
  //assumptions.  TODO: This should be changed.
  img_ = vil_image_view<vxl_byte>();

  return true;
}

template< class PixType >
bool klt_pyramid_process<PixType>::step()
{
  if (disabled_)
  {
    img_ = vil_image_view<vxl_byte>();
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

  //Currently, we are using vxl_byte because the KLT code make way too many 8 bit image
  //assumptions.  TODO: This should be changed.
  img_ = vil_image_view<vxl_byte>();

  return true;
}

template< class PixType >
bool klt_pyramid_process<PixType>::reset()
{
  return true;
}

template<>
void klt_pyramid_process<vxl_uint_16>::set_image(vil_image_view<vxl_uint_16> const& img)
{
  //TODO: redo this when we actually properly template the klt code.
  //NOTE: If you are having trouble with 16 bit images stabilizing, it is possible that this stretching is non-optimal.  That said,
  //      you will probably want to avoid vil_convert_stretch_range (img, img_);  It stretches between the min and max pixel
  //      value in img.  These values are most unstable values in the range.  I would recommend a min and max function that
  //      looks at pixels values that describe 90% of the data.  But that might be slower than one wants.  The other possible
  //      suggestion would be to use the range of the very first image as the stretch.  This might have other problems.
  vil_convert_stretch_range_limited(img, img_, std::numeric_limits<vxl_uint_16>::min(), std::numeric_limits<vxl_uint_16>::max());
}

template<>
void klt_pyramid_process<vxl_byte>::set_image(vil_image_view<vxl_byte> const& img)
{
  img_ = img;
}

template< class PixType >
vil_pyramid_image_view<float> klt_pyramid_process<PixType>::image_pyramid() const
{
  return pyramid_;
}

template< class PixType >
vil_pyramid_image_view<float> klt_pyramid_process<PixType>::image_pyramid_gradx() const
{
  return pgradx_;
}

template< class PixType >
vil_pyramid_image_view<float> klt_pyramid_process<PixType>::image_pyramid_grady() const
{
  return pgrady_;
}

} // end namespace vidtk

#endif
