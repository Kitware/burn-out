/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "mask_overlay_process.h"

#include <vxl_config.h>
#include <vil/vil_plane.h>

#include <logger/logger.h>

VIDTK_LOGGER("mask_overlay_process");



namespace vidtk
{


/** Constructor.
 *
 *
 */
mask_overlay_process
::mask_overlay_process( std::string const& _name )
  : process( _name, "mask_overlay_process" ),
    transparancy_( 0.0 )
{
  config_.add_parameter(
    "transparency",
    "0.5",
    "Transparency (alpha) for drawing mask" );

  config_.add_parameter(
    "color",
    "1.0  0.0  0.0",
    "RGB color to draw mask (in range [0,1])" );
}


mask_overlay_process
::~mask_overlay_process()
{
}


config_block
mask_overlay_process
::params() const
{
  return config_;
}


bool
mask_overlay_process
::set_params( config_block const& blk )
{

  try
  {
    transparancy_ = blk.get<double>( "transparency" );
    color_ = blk.get<vnl_double_3>( "color" );
    color_ *= 255.0;
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "
               << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
mask_overlay_process
::initialize()
{
  return true;
}



bool
mask_overlay_process
::step()
{
  vil_image_view<vxl_byte> grey_img, box_vert_smooth;

  // force allocation of new image memory
  overlay_ = vil_image_view<vxl_byte>(NULL);
  // if no mask then deep_copy the image
  if( !mask_ )
  {
    overlay_.deep_copy(image_);
    return true;
  }

  const unsigned ni = image_.ni(), nj = image_.nj(), np = image_.nplanes();
  assert(mask_.ni() == ni);
  assert(mask_.nj() == nj);
  assert(mask_.nplanes() == 1);
  assert(np == 1 || np == 3);

  overlay_.set_size(ni,nj,3);
  if (np == 3)
  {
    overlay_.deep_copy(image_);
  }
  else
  {
    vil_plane(overlay_, 0).deep_copy(image_);
    vil_plane(overlay_, 1).deep_copy(image_);
    vil_plane(overlay_, 2).deep_copy(image_);
  }
  for (unsigned p=0; p<3; ++p)
  {
    double color_term_p = (1 - transparancy_) * color_[p];
    for (unsigned j=0; j<nj; ++j)
    {
      for (unsigned i=0; i<ni; ++i)
      {
        if (mask_(i,j))
        {
          vxl_byte& pixel = overlay_(i,j,p);
          pixel = static_cast<vxl_byte>(transparancy_ * pixel + color_term_p);
        }
      }
    }
  }

  return true;
}


void
mask_overlay_process
::set_source_image( vil_image_view<vxl_byte> const& img )
{
  image_ = img;
}

void
mask_overlay_process
::set_mask( vil_image_view<bool> const& mask )
{
  mask_ = mask;
}


vil_image_view<vxl_byte>
mask_overlay_process
::image() const
{
  return overlay_;
}


} // end namespace vidtk
