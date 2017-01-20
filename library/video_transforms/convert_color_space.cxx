/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "convert_color_space.h"

#include <boost/algorithm/string.hpp>

namespace vidtk
{

color_space string_to_color_space( const std::string& str )
{
  std::string lowercase_str = str;
  boost::algorithm::to_lower( lowercase_str );

  // Manually create a short branch tree. Another common way to create
  // a string to enum correspondance would be to use a static std::map,
  // but that would incur a tiny amount of extra run-time memory.
  if( lowercase_str.size() < 3 )
  {
    return INVALID_CS;
  }

  if( lowercase_str[0] < 'k' )
  {
    if( lowercase_str == "bgr" )
    {
      return BGR;
    }
    if( lowercase_str == "cmyk" )
    {
      return CMYK;
    }
    if( lowercase_str == "hsv" )
    {
      return HSV;
    }
    if( lowercase_str == "hsl" )
    {
      return HSL;
    }
    if( lowercase_str == "hls" )
    {
      return HLS;
    }
  }
  else
  {
    if( lowercase_str == "lab" )
    {
      return Lab;
    }
    if( lowercase_str == "luv" )
    {
      return Luv;
    }
    if( lowercase_str == "rgb" )
    {
      return RGB;
    }
    if( lowercase_str == "xyz" )
    {
      return XYZ;
    }
    if( lowercase_str == "ycrcb" )
    {
      return YCrCb;
    }
    if( lowercase_str == "ycbcr" )
    {
      return YCbCr;
    }
  }

  return INVALID_CS;
}

}
