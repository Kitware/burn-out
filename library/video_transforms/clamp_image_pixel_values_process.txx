/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//clamp_image_pixel_values_process.txx
#include "clamp_image_pixel_values_process.h"

#include <vil/vil_clamp.h>

#include <limits>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

VIDTK_LOGGER("clamp_image_pixel_values_process_txx");

namespace vidtk
{
template <class PixType>
clamp_image_pixel_values_process<PixType>
::clamp_image_pixel_values_process( std::string const& _name )
  : process( _name, "clamp_image_pixel_values_process" ),
    disable_( false ),
    min_( std::numeric_limits<PixType>::min()+1 ),
    max_( std::numeric_limits<PixType>::max()-1 )
{
  config_.add_parameter( "disable", "false",
                        "Disables the process, which means it just passes the image along with out changing it" );
  //We are doing casts here because vxl_byte is an unsigned char, which is turned into a character
  //in the string instead of a number.  This means reading the values in gives errors.  I know
  //this is ugly and not as future proof as I would like.  I am open/want to another solution
  //that will insure that vxl_byte is turned into a string of a number.
  config_.add_parameter( "min", boost::lexical_cast<std::string,int>( min_ ),
                         "Any pixel value less than min will be replaced by min" );
  config_.add_parameter( "max", boost::lexical_cast<std::string,int>( max_ ),
                         "Any pixel value greater than max will be replaced by max" );
}


template <class PixType>
clamp_image_pixel_values_process<PixType>
::~clamp_image_pixel_values_process()
{
}


template <class PixType>
config_block
clamp_image_pixel_values_process<PixType>
::params() const
{
  return config_;
}


template <class PixType>
bool
clamp_image_pixel_values_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    disable_ = blk.get<bool>( "disable" );
    if(!disable_)
    {
      min_ = blk.get<PixType>( "min" );
      max_ = blk.get<PixType>( "max" );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": Unable to set parameters: " << e.what() );
    return false;
  }

  config_.update( blk );
  return true;
}


template <class PixType>
bool
clamp_image_pixel_values_process<PixType>
::initialize()
{
  in_img_ = NULL;

  return true;
}


template <class PixType>
bool
clamp_image_pixel_values_process<PixType>
::step()
{
  LOG_ASSERT( in_img_ != NULL, "Input image not set" );
  out_img_ = *in_img_;//shallow copy

  if( !disable_ )
  {
    vil_clamp(*in_img_,out_img_,min_,max_);
  }

  // record that we've processed this input image
  in_img_ = NULL;

  return true;
}


template <class PixType>
void
clamp_image_pixel_values_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  in_img_ = &img;
}


template <class PixType>
vil_image_view<PixType>
clamp_image_pixel_values_process<PixType>
::image() const
{
  return out_img_;
}

}
