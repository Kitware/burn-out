/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_image_view.h>

#include <vil/vil_math.h>

#include <testlib/testlib_test.h>

#include <video_transforms/invert_image_values.h>

namespace
{

using namespace vidtk;

void test_invert_image_values()
{
  vil_image_view< vxl_byte > byte_image( 2, 2, 3 );
  vil_image_view< int > integer_image( 2, 2, 3 );
  vil_image_view< float > float_image( 2, 2, 3 );

  byte_image.fill( 0 );
  integer_image.fill( 0 );
  float_image.fill( 0.0f );

  byte_image( 1, 1, 1 ) = 234;
  integer_image( 1, 1, 1 ) = 234;
  float_image( 1, 1, 1 ) = 234.0f;

  invert_image( byte_image );
  invert_image( integer_image );
  invert_image( float_image );

  TEST( "Byte Image Value 1", byte_image( 1, 1, 1 ), 21 );
  TEST( "Byte Image Value 2", byte_image( 0, 0, 0 ), 255 );

  TEST( "Integer Image Value 1", integer_image( 1, 1, 1 ), -234 );
  TEST( "Integer Image Value 2", integer_image( 0, 0, 0 ), 0 );

  TEST( "FP Image Value 1", float_image( 1, 1, 1 ), -234.0f );
  TEST( "FP Image Value 2", float_image( 0, 0, 0 ), 0.0f );
}


} // end anonymous namespace

int test_invert_image_values( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "invert_image_values" );

  test_invert_image_values();

  return testlib_test_summary();
}
