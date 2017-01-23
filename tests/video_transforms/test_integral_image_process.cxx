/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_algorithm.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <testlib/testlib_test.h>
#include <pipeline_framework/async_pipeline.h>

#include <video_transforms/integral_image_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


void
test_integral_imaging()
{
  // Fill in some dummy values
  vil_image_view<vxl_byte> image1( 3, 3 );
  image1.fill( 0 );
  image1( 0, 0 ) = 1;
  image1( 1, 1 ) = 1;

  vil_image_view<vxl_byte> image2( 3, 3 );
  image2.fill( 200 );

  // Create a test pipeline
  integral_image_process< vxl_byte, int > ii_process ("integrator");
  config_block default_params = ii_process.params();

  vcl_cout << vcl_endl;
  vcl_cout << "Beginning integral image process test" << vcl_endl;
  vcl_cout << vcl_endl;

  // Test image 1
  {
    config_block test_setup = default_params;
    ii_process.initialize();
    ii_process.set_params( test_setup );
    ii_process.set_source_image( image1 );
    ii_process.step();
    vil_image_view< int > output = ii_process.integral_image();
    TEST( "Output Image Size", output.ni() == 4 && output.nj() == 4, true );
    TEST( "Value 1", output(0,0), 0 );
    TEST( "Value 1", output(1,1), 1 );
    TEST( "Value 2", output(2,3), 2 );
    TEST( "Value 3", output(3,3), 2 );
  }

  // Test image 2
  {
    config_block test_setup = default_params;
    ii_process.initialize();
    ii_process.set_params( test_setup );
    ii_process.set_source_image( image2 );
    ii_process.step();
    vil_image_view< int > output = ii_process.integral_image();
    TEST( "Value 4", output(1,1), 200 );
    TEST( "Value 5", output(2,3), 1200 );
    TEST( "Value 6", output(3,3), 1800 );
  }
}

} // end anonymous namespace

int test_integral_image_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "frame_averaging" );

  test_integral_imaging();

  return testlib_test_summary();
}
