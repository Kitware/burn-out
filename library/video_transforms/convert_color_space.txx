/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "convert_color_space.h"

#include <vil/vil_plane.h>
#include <vil/vil_copy.h>

#ifdef USE_OPENCV
#include <opencv/cxcore.h>
#include <opencv/cv.h>

#include <utilities/vxl_to_cv_converters.h>
#endif

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER( "convert_color_space" );


// Helper templated typedef for defining a function which translates between
// two color spaces, that only takes two images as arguments.
template< typename PixType >
struct converter_func
{
public:
  typedef typename boost::function2<void,
                                    const vil_image_view<PixType>&,
                                    vil_image_view<PixType>& > type;

  static type swap_planes_func( unsigned plane1, unsigned plane2 )
  {
    return boost::bind( swap_planes<PixType>, _1, _2, plane1, plane2 );
  }
};

// This implementation uses OpenCV for formats VXL doesn't support by itself.
#ifdef USE_OPENCV

typedef int cv_convert_code;

static const cv_convert_code CV_Invalid = -1;

cv_convert_code lookup_cv_conversion_code( color_space space1, color_space space2 )
{
  switch( space1 )
  {
    case RGB:
      switch( space2 )
      {
        case XYZ:
          return CV_RGB2XYZ;
        case YCrCb:
          return CV_RGB2YCrCb;
        case HSV:
          return CV_RGB2HSV;
        case HLS:
          return CV_RGB2HLS;
        case Lab:
          return CV_RGB2Lab;
        case Luv:
          return CV_RGB2Luv;
        default:
          return CV_Invalid;
      }
    case BGR:
      switch( space2 )
      {
        case XYZ:
          return CV_BGR2XYZ;
        case YCrCb:
          return CV_BGR2YCrCb;
        case HSV:
          return CV_BGR2HSV;
        case HLS:
          return CV_BGR2HLS;
        case Lab:
          return CV_BGR2Lab;
        case Luv:
          return CV_BGR2Luv;
        default:
          return CV_Invalid;
      }
    case HSV:
      switch( space2 )
      {
        case RGB:
          return CV_HSV2RGB;
        case BGR:
          return CV_HSV2BGR;
        default:
          return CV_Invalid;
      }
    case HLS:
      switch( space2 )
      {
        case RGB:
          return CV_HLS2RGB;
        case BGR:
          return CV_HLS2BGR;
        default:
          return CV_Invalid;
      }
    case XYZ:
      switch( space2 )
      {
        case RGB:
          return CV_XYZ2RGB;
        case BGR:
          return CV_XYZ2BGR;
        default:
          return CV_Invalid;
      }
    case Lab:
      switch( space2 )
      {
        case RGB:
          return CV_Lab2RGB;
        case BGR:
          return CV_Lab2BGR;
        default:
          return CV_Invalid;
      }
    case Luv:
      switch( space2 )
      {
        case RGB:
          return CV_Luv2RGB;
        case BGR:
          return CV_Luv2BGR;
        default:
          return CV_Invalid;
      }
    case YCrCb:
      switch( space2 )
      {
        case RGB:
          return CV_YCrCb2RGB;
        case BGR:
          return CV_YCrCb2BGR;
        default:
          return CV_Invalid;
      }
    default:
      return CV_Invalid;
  }
  return CV_Invalid;
}

template< typename PixType >
bool convert_colors_cv( const vil_image_view< PixType >& src,
                        vil_image_view< PixType >& dst,
                        const cv_convert_code cv_conversion_code )
{
  LOG_ASSERT( src.nplanes() == 3, "Input must contain 3 planes" );

  vil_image_view< PixType > tmp = vil_image_view<PixType>( src.ni(), src.nj(), 1, src.nplanes() );

  vil_copy_reformat( src, tmp );

  cv::Mat cv_image_wrapper;

  if( !shallow_cv_conversion( tmp, cv_image_wrapper ) )
  {
    LOG_FATAL( "Shallow wrapper failed; has OpenCV changed image representations?" );
    return false;
  }

  cv::cvtColor( cv_image_wrapper,
                cv_image_wrapper,
                cv_conversion_code );

  dst = tmp;
  return true;
}

#endif

// If possible, returns function pointer for the specified conversion.
// Returns whether or not a valid function was found.
template< typename PixType >
bool lookup_functor( const color_space src,
                     const color_space dst,
                     typename converter_func<PixType>::type& func )
{
  // Check for VXL supported reformats
  switch( src )
  {
    case RGB:
      switch( dst )
      {
        case BGR:
          func = converter_func<PixType>::swap_planes_func( 0, 2 );
          return true;
        default:
          break;
      }
    case BGR:
      switch( dst )
      {
        case RGB:
          func = converter_func<PixType>::swap_planes_func( 0, 2 );
          return true;
        default:
          break;
      }
    case HSL:
      switch( dst )
      {
        case HLS:
          func = converter_func<PixType>::swap_planes_func( 1, 2 );
          return true;
        default:
          break;
      }
    case HLS:
      switch( dst )
      {
        case HSL:
          func = converter_func<PixType>::swap_planes_func( 1, 2 );
          return true;
        default:
          break;
      }
    case YCrCb:
      switch( dst )
      {
        case YCbCr:
          func = converter_func<PixType>::swap_planes_func( 1, 2 );
          return true;
        default:
          break;
      }
    case YCbCr:
      switch( dst )
      {
        case YCrCb:
          func = converter_func<PixType>::swap_planes_func( 1, 2 );
          return true;
        default:
          break;
      }
    default:
      break;
  }

#ifdef USE_OPENCV
  // Check for OpenCV supported reformats
  const vil_pixel_format type_code = vil_pixel_format_of( PixType() );

  cv_convert_code cv_code = lookup_cv_conversion_code( src, dst );

  bool ocv_supported = ( type_code == vil_pixel_format_of( vxl_byte() ) ||
                         type_code == vil_pixel_format_of( vxl_uint_16() ) ||
                         type_code == vil_pixel_format_of( float() ) )
                       && ( cv_code != CV_Invalid );

  if( ocv_supported )
  {
    func = boost::bind( convert_colors_cv<PixType>, _1, _2, cv_code );
    return true;
  }
#endif
  return false;
}


template< typename PixType >
bool is_conversion_supported( const color_space src,
                              const color_space dst )
{
  typename converter_func<PixType>::type func;
  return lookup_functor<PixType>( src, dst, func );
}

template< typename PixType >
bool convert_color_space( const vil_image_view< PixType >& src,
                          vil_image_view< PixType >& dst,
                          const color_space src_space,
                          const color_space dst_space )
{
  LOG_ASSERT( src.nplanes() == 3, "Input must contain 3 planes" );

  if( src_space == dst_space )
  {
    dst.deep_copy( src );
  }

  // Check for support and retrieve functor
  typename converter_func<PixType>::type func;
  if( !lookup_functor<PixType>( src_space, dst_space, func ) )
  {
    return false;
  }

  // Perform conversion
  func( src, dst );
  return true;
}

template< typename PixType >
void swap_planes( const vil_image_view< PixType >& src,
                  vil_image_view< PixType >& dst,
                  const unsigned plane1,
                  const unsigned plane2 )
{
  // Validate inputs
  LOG_ASSERT( plane1 < src.nplanes(), "Input plane1 value invalid" );
  LOG_ASSERT( plane2 < src.nplanes(), "Input plane2 value invalid" );
  LOG_ASSERT( plane1 != plane2, "Input planes must be different" );

  vil_image_view< PixType > tmp = vil_image_view< PixType >( src.ni(), src.nj(), 1, src.nplanes() );

  // Handle optimized special case of reversing all channel orders
  if( src.nplanes() == 3 && ( (plane1 == 0 && plane2 == 2) || (plane1 == 2 && plane2 == 0) ) )
  {
    vil_image_view<PixType> rev_tmp( tmp.top_left_ptr()+2,
                                     tmp.ni(),
                                     tmp.nj(),
                                     tmp.nplanes(),
                                     tmp.istep(),
                                     tmp.jstep(),
                                     std::ptrdiff_t(-1) );
    rev_tmp.deep_copy( src );
  }
  // Handle general case
  else
  {
    for( unsigned i = 0; i < src.nplanes(); i++ )
    {
      unsigned src_plane_id = i;
      unsigned tmp_plane_id = i;

      if( i == plane1 )
      {
        src_plane_id = plane2;
      }
      else if( i == plane2 )
      {
        src_plane_id = plane1;
      }

      vil_image_view< PixType > src_plane = vil_plane( src, src_plane_id );
      vil_image_view< PixType > tmp_plane = vil_plane( tmp, tmp_plane_id );
      tmp_plane.deep_copy( src_plane );
    }
  }

  dst = tmp;
}

}
