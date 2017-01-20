/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/frame_data.h>
#include <testlib/testlib_test.h>

using namespace vidtk;

int test_frame_data( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "frame_data" );

  {
    frame_data test_frame;

    vil_image_view< vxl_byte > test_image( 20, 20, 3 );
    test_image.fill( 125 );

    test_frame.set_image( test_image );

    vil_image_view< vxl_byte > mc_image_8u = test_frame.image_8u();
    vil_image_view< vxl_uint_16 > mc_image_16u = test_frame.image_16u();
    vil_image_view< vxl_byte > sc_image_8u = test_frame.image_8u_gray();
    vil_image_view< vxl_uint_16 > sc_image_16u = test_frame.image_16u_gray();

    TEST( "Color image, 8 bit", mc_image_8u.nplanes(), 3 );
    TEST( "Color image, 16 bit", mc_image_16u.nplanes(), 3 );
    TEST( "Gray image, 8 bit", sc_image_8u.nplanes(), 1 );
    TEST( "Gray image, 16 bit", sc_image_16u.nplanes(), 1 );
  }

  return testlib_test_summary();
}
