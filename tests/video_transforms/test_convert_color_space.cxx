/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_algorithm.h>

#include <vil/vil_image_view.h>

#include <testlib/testlib_test.h>

#include <video_transforms/convert_color_space.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void simple_color_tests()
{
  vil_image_view<vxl_byte> color_image( 20, 20, 3 ), converted_image;
  color_image.fill( 0 );
  color_image( 10, 10, 0 ) = 255;

  bool bgr_support = is_conversion_supported<vxl_byte>( RGB, BGR );

  TEST( "BGR Supported", bgr_support, true );

  convert_color_space( color_image, converted_image, RGB, BGR );

  TEST( "BGR Value 1", converted_image( 10, 10, 0 ), 0 );
  TEST( "BGR Value 2", converted_image( 10, 10, 2 ), 255 );

  bool hsv_support = is_conversion_supported<vxl_byte>( RGB, HSV );

  if( hsv_support )
  {
    TEST( "HSV Supported", hsv_support, true );

    convert_color_space( color_image, converted_image, RGB, HSV );

    TEST( "HSV Value 1", converted_image( 10, 10, 0 ), 0 );
    TEST( "HSV Value 2", converted_image( 10, 10, 2 ), 255 );
  }

  bool lab_support = is_conversion_supported<vxl_byte>( RGB, Lab );

  if( lab_support )
  {
    TEST( "Lab Supported", lab_support, true );

    convert_color_space( color_image, converted_image, RGB, Lab );

    TEST( "Lab Value 1", converted_image( 10, 10, 0 ), 136 );
    TEST( "Lab Value 2", converted_image( 10, 10, 2 ), 195 );
  }

  {
    vil_image_view<vxl_byte> original_image = color_image;

    convert_color_space( color_image, color_image, RGB, BGR );

    TEST( "Same Input Value 1", color_image( 10, 10, 0 ), 0 );
    TEST( "Same Input Value 2", color_image( 10, 10, 2 ), 255 );
    TEST( "Same Input Value 3", original_image( 10, 10, 0 ), 255 );
    TEST( "Same Input Value 4", original_image( 10, 10, 2 ), 0 );

    color_image = original_image;
  }
}

} // end anonymous namespace

int test_convert_color_space( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "convert_color_space" );

  simple_color_tests();

  return testlib_test_summary();
}
