/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "convert_image_process.h"

#include <vil/vil_convert.h>

#include <logger/logger.h>

VIDTK_LOGGER("convert_image_process_txx");

namespace vidtk
{


template <class FromPixType, class ToPixType>
convert_image_process<FromPixType, ToPixType>
::convert_image_process( std::string const& _name )
  : process( _name, "convert_image_process" )
{
}


template <class FromPixType, class ToPixType>
convert_image_process<FromPixType, ToPixType>
::~convert_image_process()
{
}


template <class FromPixType, class ToPixType>
config_block
convert_image_process<FromPixType, ToPixType>
::params() const
{
  return config_;
}


template <class FromPixType, class ToPixType>
bool
convert_image_process<FromPixType, ToPixType>
::set_params( config_block const& blk )
{
  config_.update( blk );
  return true;
}


template <class FromPixType, class ToPixType>
bool
convert_image_process<FromPixType, ToPixType>
::initialize()
{
  input_image_ = vil_image_view<FromPixType>();

  return true;
}

namespace
{

template <class FromPixType, class ToPixType>
class convert_image
{
  public:
    static void convert( vil_image_view<FromPixType> const& from,
                         vil_image_view<ToPixType>& to );
};

template <class PixType>
class convert_image<PixType, PixType>
{
  public:
    static void convert( vil_image_view<PixType> const& from,
                         vil_image_view<PixType>& to );
};

template <class FromPixType, class ToPixType>
inline void
convert_image<FromPixType, ToPixType>
::convert( vil_image_view<FromPixType> const& from,
           vil_image_view<ToPixType>& to )
{
  vil_convert_stretch_range( from, to );
}

template <>
inline void
convert_image<float, double>
::convert( vil_image_view<float> const& from,
           vil_image_view<double>& to )
{
  vil_convert_cast( from, to );
}

template <>
inline void
convert_image<double, float>
::convert( vil_image_view<double> const& from,
           vil_image_view<float>& to )
{
  vil_convert_cast( from, to );
}

template <class PixType>
inline void
convert_image<PixType, PixType>
::convert( vil_image_view<PixType> const& from,
           vil_image_view<PixType>& to )
{
  to = from;
}

}


template <class FromPixType, class ToPixType>
bool
convert_image_process<FromPixType, ToPixType>
::step()
{
  if ( !input_image_ )
  {
    LOG_ERROR( this->name() << ": Input image not set" );
    return false;
  }

  convert_image<FromPixType, ToPixType>::convert( input_image_, converted_image_ );

  input_image_ = vil_image_view<FromPixType>();

  return true;
}


template <class FromPixType, class ToPixType>
void
convert_image_process<FromPixType, ToPixType>
::set_image( vil_image_view<FromPixType> const& img )
{
  input_image_ = img;
}


template <class FromPixType, class ToPixType>
vil_image_view<ToPixType>
convert_image_process<FromPixType, ToPixType>
::converted_image() const
{
  return converted_image_;
}


} // end namespace vidtk
