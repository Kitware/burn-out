/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/// \file

#include "crop_image_process.h"

#include <vil/vil_crop.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_crop_image_process_txx__
VIDTK_LOGGER("crop_image_process_txx");


namespace vidtk
{


template <class PixType>
crop_image_process<PixType>
::crop_image_process( std::string const& _name )
  : process( _name, "crop_image_process" )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Disable this process, causing the image to pass through unmodified" );

  config_.add_parameter(
    "upper_left",
    "0 0",
    "The pixel (x,y) coordinates of the top-left corner of the cropping "
    "rectangle. "
    "Negative coordinates are interpreted as offsets from "
    "the right and bottom edges of the image. "
    "E.g. If the image is ni x nj, then the coordinate (-1,-3) is equivalent "
    "to the coordinate (ni-1,nj-3)." );

  config_.add_parameter(
    "lower_right",
    "0 0",
    "The pixel (x,y) coordinates of the bottom-right corner of the cropping "
    "rectangle. These pixels are *not* in the cropped rectangle, so that a "
    "crop with top-left (10,10) and bottom-right (90,90) will be 90x90 pixels "
    "in size, and not 91x91 pixels.  "
    "See also the description of top_left for "
    "the interpretation of negative coordinates.  A coordinate of (0,0) is "
    "interpreted as (ni,nj)." );
}


template <class PixType>
crop_image_process<PixType>
::~crop_image_process()
{
}


template <class PixType>
config_block
crop_image_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
crop_image_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    disabled_ = blk.get<bool>( "disabled" );
    upper_left_ = blk.get<vnl_int_2>( "upper_left" );
    lower_right_ = blk.get<vnl_int_2>( "lower_right" );
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


template <class PixType>
bool
crop_image_process<PixType>
::initialize()
{
  input_image_ = NULL;

  return true;
}


template <class PixType>
bool
crop_image_process<PixType>
::step()
{
  if ( input_image_ == NULL )
  {
    LOG_ERROR( this->name() << ": Input image not set" );
    return false;
  }

  if( disabled_ )
  {
    cropped_image_ = *input_image_;
  }
  else
  {
    unsigned ni = input_image_->ni();
    unsigned nj = input_image_->nj();
    unsigned i0 = static_cast<unsigned>(
      upper_left_[0] >= 0 ? upper_left_[0] : static_cast<int>(ni) + upper_left_[0]
      );
    unsigned j0 = static_cast<unsigned>(
      upper_left_[1] >= 0 ? upper_left_[1] : static_cast<int>(nj) + upper_left_[1]
      );
    unsigned i1 = static_cast<unsigned>(
      lower_right_[0] > 0 ? lower_right_[0] : static_cast<int>(ni) + lower_right_[0]
      );
    unsigned j1 = static_cast<unsigned>(
      lower_right_[1] > 0 ? lower_right_[1] : static_cast<int>(nj) + lower_right_[1]
      );

    if( i0 >= ni || j0 >= nj ||
        i1 >  ni || j1 >  nj ||
        i0 >= i1 || j0 >= j1 )
    {
      LOG_ERROR( this->name() << ": crop from (" << i0 << "," << j0 << ")-("
                 << i1 << "," << j1 << ") is empty or is not within the "
                 << ni << "x" << nj << " source image" );
      return false;
    }

    cropped_image_ = vil_crop( *input_image_, i0, i1-i0, j0, j1-j0 );
  }

  input_image_ = NULL;

  return true;
}


template <class PixType>
void
crop_image_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  input_image_ = &img;
}


template <class PixType>
vil_image_view<PixType>
crop_image_process<PixType>
::cropped_image() const
{
  return cropped_image_;
}


} // end namespace vidtk
