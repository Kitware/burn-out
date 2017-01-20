/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_convert_color_space_h_
#define vidtk_convert_color_space_h_

#include <vil/vil_image_view.h>
#include <string>


namespace vidtk
{

/// \brief Known color spaces.
///
/// Just because a color space is defined here, doesn't necessarily means that
/// the convert_color_space function supports it.
enum color_space
{
  INVALID_CS = 0,

  RGB, BGR,

  HSV, HSL, HLS,

  XYZ, Lab, Luv,

  CMYK,

  YCrCb, YCbCr
};


/// \brief Converts a string to a known color space if possible.
color_space string_to_color_space( const std::string& str );


/// \brief With the current build options and included libraries, is a
/// certain color space conversion between two types supported?
template< typename PixType >
bool is_conversion_supported( const color_space src,
                              const color_space dst );


/// \brief Convert a 3-channel image between 2 seperate color spaces.
///
/// For conversions which require it, the default white point for the
/// specified type is used. The input will be deep copied.
///
/// The default white points are defined in file automatic_white_balancing.h
/// and are additionally listed below:
///
/// vxl_byte: 255
/// vxl_uint_16, int, unsigned int: 65535
/// float: 1.0
template< typename PixType >
bool convert_color_space( const vil_image_view< PixType >& src,
                          vil_image_view< PixType >& dst,
                          const color_space src_space,
                          const color_space dst_space );


/// \brief Deep copy the input and swap the two specified planes.
template< typename PixType >
void swap_planes( const vil_image_view< PixType >& src,
                  vil_image_view< PixType >& dst,
                  const unsigned plane1,
                  const unsigned plane2 );

}

#endif // vidtk_convert_color_space_h_
