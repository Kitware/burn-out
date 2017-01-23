/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "convert_color_space_process.h"

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER ("convert_color_space_process");

template <typename PixType>
convert_color_space_process<PixType>
::convert_color_space_process( std::string const& _name )
  : process( _name, "convert_color_space_process" ),
    src_space_(RGB),
    dst_space_(HSV)
{
  config_.add_parameter(
    "input_color_space",
    "RGB",
    "The color space of the input image, possible values: "
    "RGB, BGR, HSV, HSL, YCrCb, YCbCr, Luv, Lab, HLS, "
    "CMYK, and XYZ." );
  config_.add_parameter(
    "output_color_space",
    "HSV",
    "The desired color space of the output image." );
}


template <typename PixType>
convert_color_space_process<PixType>
::~convert_color_space_process()
{
}

template <typename PixType>
config_block
convert_color_space_process<PixType>
::params() const
{
  return config_;
}


template <typename PixType>
bool
convert_color_space_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    std::string tmp;

    tmp = blk.get<std::string>("input_color_space");

    src_space_ = string_to_color_space(tmp);

    if( src_space_ == INVALID_CS )
    {
      throw config_block_parse_error( "Invalid color space : " + tmp );
    }

    tmp = blk.get<std::string>("output_color_space");

    dst_space_ = string_to_color_space(tmp);

    if( dst_space_ == INVALID_CS )
    {
      throw config_block_parse_error( "Invalid color space : " + tmp );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: "  << e.what() );
    return false;
  }

  if( !is_conversion_supported<PixType>(src_space_,dst_space_) )
  {
    LOG_ERROR( "Unable to convert color space " << src_space_ << " " <<
               "to color space " << dst_space_ << " " <<
               "for type of size " << sizeof(PixType) );
    return false;
  }

  config_.update( blk );
  return true;
}

template <typename PixType>
bool
convert_color_space_process<PixType>
::initialize()
{
  return true;
}

template <typename PixType>
bool
convert_color_space_process<PixType>
::step()
{
  if( !src_image_ || src_image_->nplanes() != 3 )
  {
    LOG_ERROR( this->name() << ": Invalid input image provided." );
    return false;
  }

  dst_image_ = vil_image_view<PixType>();
  convert_color_space( *src_image_, dst_image_, src_space_, dst_space_ );
  return true;
}

// Input/Output Accessors
template <typename PixType>
void
convert_color_space_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  src_image_ = &img;
}

template <typename PixType>
vil_image_view<PixType>
convert_color_space_process<PixType>
::converted_image() const
{
  return dst_image_;
}

} // end namespace vidtk
