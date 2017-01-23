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

#include <video_transforms/frame_averaging_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


void
test_frame_averaging()
{
  // Fill in some dummy values
  vil_image_view<vxl_byte> image1( 3, 3 );
  image1.fill( 25 );
  image1( 0, 0 ) = 100;
  image1( 1, 1 ) = 100;
  image1( 2, 0 ) = 100;
  vil_image_view<vxl_byte> image2( 3, 3 );
  image2.fill( 255 );
  vil_image_view<vxl_byte> image3( 3, 3 );
  image3.fill( 20 );

  // Create a test pipeline
  frame_averaging_process< vxl_byte > avg_process ("averager");
  config_block default_params = avg_process.params();

  vcl_cout << vcl_endl;
  vcl_cout << "Beginning frame averaging test" << vcl_endl;
  vcl_cout << vcl_endl;

  // Test standard averaging
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "cumulative" );
    avg_process.initialize();
    avg_process.set_params( test_setup );
    avg_process.set_source_image( image1 );
    avg_process.step();
    avg_process.set_source_image( image2 );
    avg_process.step();
    avg_process.set_source_image( image3 );
    avg_process.step();
    vil_image_view< vxl_byte > output = avg_process.averaged_image();
    TEST( "Cumulative Averaging Test: Size", output.ni() == 3 && output.nj() == 3, true );
    TEST( "Cumulative Averaging Test: Value", output(1,1), 125 );
  }

  // Test windowed averaging
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "window" );
    test_setup.set( "window_size", "2" );
    test_setup.set( "round", "false" );
    avg_process.initialize();
    avg_process.set_params( test_setup );
    avg_process.set_source_image( image1 );
    avg_process.step();
    avg_process.set_source_image( image2 );
    avg_process.step();
    avg_process.set_source_image( image3 );
    avg_process.step();
    vil_image_view< vxl_byte > output = avg_process.averaged_image();
    TEST( "Windowed Averaging Test: Value 1", output(1,1), 137 );
    TEST( "Windowed Averaging Test: Value 2", output(2,1), 137 );
  }

  // Test exponential averaging
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "exponential" );
    test_setup.set( "exp_weight", "0.75" );
    test_setup.set( "round", "false" );
    avg_process.initialize();
    avg_process.set_params( test_setup );
    avg_process.set_source_image( image1 );
    avg_process.step();
    avg_process.set_source_image( image2 );
    avg_process.step();
    avg_process.set_source_image( image3 );
    avg_process.step();
    vil_image_view< vxl_byte > output = avg_process.averaged_image();
    TEST( "Exponential Averaging Test: Value 1", output(1,1), 69 );
  }

  // Test resets
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "exponential" );
    test_setup.set( "exp_weight", "0.75" );
    test_setup.set( "round", "false" );
    avg_process.initialize();
    avg_process.set_params( test_setup );
    avg_process.set_reset_flag( false );
    avg_process.set_source_image( image1 );
    avg_process.step();
    avg_process.set_reset_flag( false );
    avg_process.set_source_image( image2 );
    avg_process.step();
    avg_process.set_reset_flag( true );
    avg_process.set_source_image( image3 );
    avg_process.step();
    vil_image_view< vxl_byte > output = avg_process.averaged_image();
    TEST( "Averaging Test w/ Resets: Value 1", output(1,1), 20 );
    avg_process.initialize();
    avg_process.set_params( test_setup );
    avg_process.set_reset_flag( false );
    avg_process.set_source_image( image1 );
    avg_process.step();
    avg_process.set_reset_flag( true );
    avg_process.set_source_image( image2 );
    avg_process.step();
    avg_process.set_reset_flag( false );
    avg_process.set_source_image( image3 );
    avg_process.step();
    output = avg_process.averaged_image();
    TEST( "Averaging Test w/ Resets: Value 2", output(1,1), 78 );
  }

  // Test variance calculation
  {
    avg_process.initialize();
    config_block test_setup = default_params;
    test_setup.set( "type", "cumulative" );
    test_setup.set( "compute_variance", "true" );
    avg_process.set_params( test_setup );
    avg_process.set_source_image( image1 );
    avg_process.step();
    avg_process.set_source_image( image2 );
    avg_process.step();
    avg_process.set_source_image( image3 );
    avg_process.step();
    vil_image_view< double > var = avg_process.variance_image();
    TEST( "Variance Test: Var Est 1", vcl_sqrt(var(1,1)) > 85.0, true );
    TEST( "Variance Test: Var Est 2", vcl_sqrt(var(1,1)) < 155.0, true );
  }
}

} // end anonymous namespace

int test_frame_averaging_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "frame_averaging" );
  test_frame_averaging();
  return testlib_test_summary();
}
