/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_algorithm.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>
#include <testlib/testlib_test.h>

#include <video/jittered_image_difference.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

vxl_byte img_buf1[10][10] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 3, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 2, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

vxl_byte img_buf2[10][10] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 3, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 2, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};


void test_simple1()
{
  vil_image_view<vxl_byte> img1( &img_buf1[0][0], 10, 10, 1, 1, 10, 1 );
  vil_image_view<vxl_byte> img2( &img_buf2[0][0], 10, 10, 1, 1, 10, 1 );

  {
    vcl_cout << "Difference 1-2 with jitter zero\n";
    vil_image_view<float> diff;
    jittered_image_difference( img1, img2, diff,
                               jittered_image_difference_params()
                               .set_delta( 0, 0 ) );
    TEST( "Pixel (3,3)", diff(3,3), 3 );
    TEST( "Pixel (4,3)", diff(4,3), 3 );
    TEST( "Pixel (6,6)", diff(6,6), 2 );
  }

  {
    vcl_cout << "Difference 1-2 with jitter (1,0)\n";
    vil_image_view<float> diff;
    jittered_image_difference( img1, img2, diff,
                               jittered_image_difference_params()
                               .set_delta( 1, 0 ) );
    TEST( "Pixel (3,3)", diff(3,3), 0 );
    TEST( "Pixel (4,3)", diff(4,3), 0 );
    TEST( "Pixel (6,6)", diff(6,6), 2 );
  }

  {
    vcl_cout << "Difference 2-1 with jitter (1,0)\n";
    vil_image_view<float> diff;
    jittered_image_difference( img2, img1, diff,
                               jittered_image_difference_params()
                               .set_delta( 1, 0 ) );
    TEST( "Pixel (3,3)", diff(3,3), 0 );
    TEST( "Pixel (4,3)", diff(4,3), 0 );
    TEST( "Pixel (6,6)", diff(6,6), 0 );
  }

  {
    vcl_cout << "Difference 1-2 with jitter (0,1)\n";
    vil_image_view<float> diff;
    jittered_image_difference( img1, img2, diff,
                               jittered_image_difference_params()
                               .set_delta( 0, 1 ) );
    TEST( "Pixel (3,3)", diff(3,3), 3 );
    TEST( "Pixel (4,3)", diff(4,3), 0 );
    TEST( "Pixel (6,6)", diff(6,6), 2 );
  }

  {
    vcl_cout << "Difference 1-2 with jitter (2,2)\n";
    vil_image_view<float> diff;
    jittered_image_difference( img1, img2, diff,
                               jittered_image_difference_params()
                               .set_delta( 2, 2 ) );
    TEST( "Pixel (3,3)", diff(3,3), 0 );
    TEST( "Pixel (4,3)", diff(4,3), 0 );
    TEST_NEAR( "Pixel (6,6)", diff(6,6), 0, 0 );
  }
}


} // end anonymous namespace

int test_jittered_image_difference( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "jittered_image_difference" );

  test_simple1();

  return testlib_test_summary();
}
