/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "metadata_inpainting_process.h"

#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <vil/vil_convert.h>

#include <opencv/cxcore.h>
#include <opencv/cvaux.h>


namespace vidtk
{


metadata_inpainting_process
::metadata_inpainting_process( vcl_string const& name )
  : process( name, "metadata_inpainting_process" ),
    disabled_( false )
{
  config_.add_parameter(
    "disabled",
    "false",
    "Disable this process, causing the image to pass through unmodified." );

  config_.add_parameter(
    "radius",
    "8",
    "Radius around the inpainted area that is considered for the algorithm." );

  config_.add_parameter(
    "method",
    "telea",
    "The algorithm to use when inpainting." );
}


metadata_inpainting_process
::~metadata_inpainting_process()
{
}


config_block
metadata_inpainting_process
::params() const
{
  return config_;
}


bool
metadata_inpainting_process
::set_params( config_block const& blk )
{
  vcl_string method;

  try
  {
    blk.get( "disabled", disabled_ );
    blk.get( "radius", radius_ );
    blk.get( "method", method );
  }
  catch( unchecked_return_value& e )
  {
    log_error( this->name() << ": set_params failed: "
               << e.what() << "\n" );
    return false;
  }

  if( method == "telea" )
  {
    this->method_ = cv::INPAINT_TELEA;
  }
  else if( method == "ns" )
  {
    this->method_ = cv::INPAINT_NS;
  }
  else
  {
    log_error( this->name() << ": Invalid inpaint method\n" );
    return false;
  }

  config_.update( blk );
  return true;
}


bool
metadata_inpainting_process
::initialize()
{
  this->invalid_image_ = false;
  return true;
}


bool
metadata_inpainting_process
::step()
{
  if( this->disabled_ )
  {
    this->inpainted_image_ = this->input_image_;
    return true;
  }

  if( this->invalid_image_ )
  {
    this->invalid_image_ = false;
    return false;
  }

  this->invalid_image_ = false;

  if( this->input_image_.ni() != this->mask_.ni() )
  {
    log_error( "Input image and mask do not have the same width.\n" );
    return false;
  }

  if( this->input_image_.nj() != this->mask_.nj() )
  {
    log_error( "Input image and mask do not have the same height.\n" );
    return false;
  }

  vil_image_view<vxl_byte> byte_mask;

  vil_convert_stretch_range< bool >( this->mask_, byte_mask );

  cv::Mat input( this->input_image_.nj(), this->input_image_.ni(),
                 ( this->input_image_.nplanes() == 1 ) ? CV_8UC1 : CV_8UC3,
                 this->input_image_.top_left_ptr() );

  cv::Mat mask( byte_mask.nj(), byte_mask.ni(),
                CV_8UC1, byte_mask.top_left_ptr() );

  this->inpainted_image_ = vil_image_view<vxl_byte>( this->input_image_.ni(),
                                                     this->input_image_.nj(),
                                                     1,
                                                     this->input_image_.nplanes() );

  cv::Mat inpaint( this->inpainted_image_.nj(), this->inpainted_image_.ni(),
                   input.type(), this->inpainted_image_.top_left_ptr() );

  cv::inpaint( input, mask, inpaint, this->radius_, this->method_ );

  return true;
}


void
metadata_inpainting_process
::set_source_image( vil_image_view<vxl_byte> const& img )
{
  if( img.nplanes() != 1 && img.nplanes() != 3 )
  {
    log_error( "Input image does not have either 1 or 3 channels.\n" );
    invalid_image_ = true;
    return;
  }

  this->input_image_ = vil_image_view<vxl_byte>( img.ni(), img.nj(), 1, img.nplanes() );
  vil_copy_reformat( img, this->input_image_ );

  if( ( this->input_image_.nplanes() != 1 && this->input_image_.planestep() != 1 ) ||
      this->input_image_.istep() != this->input_image_.nplanes() ||
      this->input_image_.jstep() != ( this->input_image_.ni() * this->input_image_.nplanes() ) )
  {
    log_error( "Input image memory is not aligned for OpenCV.\n" );
    invalid_image_ = true;
    return;
  }

  return;
}


void
metadata_inpainting_process
::set_mask( vil_image_view<bool> const& img )
{
  if( img.nplanes() != 1 )
  {
    log_error( "Input mask does not have exactly 1 channel.\n" );
    invalid_image_ = true;
    return;
  }

  this->mask_ = vil_image_view<bool>();
  this->mask_.deep_copy( img );

  if( this->mask_.istep() != 1 )
  {
    log_error( "Input mask memory is not aligned for OpenCV.\n" );
    invalid_image_ = true;
    return;
  }

  if( this->mask_.jstep() != this->mask_.ni() )
  {
    log_error( "Input mask memory is not aligned for OpenCV.\n" );
    return;
    invalid_image_ = true;
    return;
  }

  return;
}


vil_image_view<vxl_byte> const&
metadata_inpainting_process
::inpainted_image() const
{
  return this->inpainted_image_;
}

} // end namespace vidtk
