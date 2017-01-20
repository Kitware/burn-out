/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "rescale_image_process.h"

#include <boost/math/special_functions/round.hpp>
#include <vil/vil_decimate.h>
#include <vil/vil_resample_bilin.h>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_rescale_image_process_txx__
VIDTK_LOGGER( "rescale_image_process_txx" );


namespace vidtk
{


template <class PixType>
rescale_image_process<PixType>
::rescale_image_process( std::string const& _name )
  : process( _name, "rescale_image_process" )
{
  config_.add_parameter(
      "algorithm",
      "pass",
      "The algorithm to downsample with. Valid values"
        ": pass"
        ", bilinear"
        ", decimate");
  config_.add_parameter(
      "scale_factor",
      "1.0",
      "The percentage of the input image's size to resize to.");
}


template <class PixType>
rescale_image_process<PixType>
::~rescale_image_process()
{
}


template <class PixType>
config_block
rescale_image_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
rescale_image_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    scale_factor_ = blk.get<double>("scale_factor");

    if (scale_factor_ <= 0)
    {
      std::string const scale_str = boost::lexical_cast<std::string>(scale_factor_);

      throw config_block_parse_error("Invalid scale value (must be > 0): " + scale_str);
    }

    std::string const algo = blk.get<std::string>("algorithm");

    if (algo == "pass")
    {
      if (scale_factor_ != 1.0)
      {
        LOG_WARN(this->name() << ": Passing image through when scale factor is " << scale_factor_);
      }

      algo_ = rescale_pass;
    }
    else if (algo == "bilinear")
    {
      algo_ = rescale_bilinear;
    }
    else if (algo == "decimate")
    {
      if (scale_factor_ > 1.0)
      {
        std::string const scale_str = boost::lexical_cast<std::string>(scale_factor_);

        throw config_block_parse_error("Invalid scale value (must be <= 1 when using \'decimate\'): " + scale_str);
      }

      // Decimation uses the inverse value;
      decimation_ = static_cast<unsigned>(boost::math::round(1.0 / scale_factor_));

      LOG_INFO(this->name() << ": Using " << decimation_ << " when decimating");

      algo_ = rescale_decimate;
    }
    else
    {
      throw config_block_parse_error("Invalid scaling algorithm: " + algo);
    }
  }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
rescale_image_process<PixType>
::initialize()
{
  in_img_ = vil_image_view<PixType>();
  return true;
}


template <class PixType>
bool
rescale_image_process<PixType>
::reset()
{
  in_img_ = vil_image_view<PixType>();
  return true;
}

template <class PixType>
void
resample_bilin( const vil_image_view< PixType >& src,
                vil_image_view< PixType >& dest,
                int n1, int n2 )
{
  vil_resample_bilin( src, dest, n1, n2 );
}

void
resample_bilin( const vil_image_view<bool>& /*src*/,
                vil_image_view<bool>& /*dest*/,
                int /*n1*/, int /*n2*/ )
{
  LOG_AND_DIE( "Boolean resample bilin not yet supported" );
}

template <class PixType>
bool
rescale_image_process<PixType>
::step()
{
  if (!in_img_)
  {
    LOG_ERROR(this->name() << ": No input image given");
    return false;
  }

  out_img_ = vil_image_view<PixType>();

  switch (algo_)
  {
    case rescale_pass:
      out_img_ = in_img_;
      break;
    case rescale_bilinear:
      resample_bilin(in_img_, out_img_, scale_factor_ * in_img_.ni(), scale_factor_ * in_img_.nj());
      break;
    case rescale_decimate:
      out_img_ = vil_decimate(in_img_, decimation_);
      break;
    default:
      LOG_ERROR(this->name() << ": Unrecognized algorithm: " << algo_);

      return false;
  }

  in_img_ = vil_image_view<PixType>();

  return true;
}


template <class PixType>
void
rescale_image_process<PixType>
::set_image( vil_image_view<PixType> const& img )
{
  in_img_ = img;
}


template <class PixType>
vil_image_view<PixType>
rescale_image_process<PixType>
::image() const
{
  return out_img_;
}


} // end namespace vidtk
