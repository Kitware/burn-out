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

#include <video_transforms/color_commonality_filter_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void simple_cc_tests()
{
  vil_image_view<vxl_byte> input_intensity( 20, 20 );
  vil_image_view<vxl_byte> input_color( 20, 20, 3 );

  input_intensity.fill( 100 );
  input_color.fill( 100 );

  input_intensity( 10, 10 ) = 0;
  input_intensity( 10, 11 ) = 255;

  input_color( 10, 10, 0 ) = 0;
  input_color( 10, 11, 0 ) = 255;

  color_commonality_filter_process<vxl_byte,vxl_byte> cc_proc("cc_proc");

  cc_proc.set_source_image( input_intensity );
  cc_proc.step();

  vil_image_view<vxl_byte> output1 = cc_proc.filtered_image();

  cc_proc.set_source_image( input_color );
  cc_proc.step();

  vil_image_view<vxl_byte> output2 = cc_proc.filtered_image();

  TEST( "Common intensity value 1", output1(1,1) > 250, true );
  TEST( "Common intensity value 2", output1(19,19) > 250, true );
  TEST( "Uncommon intensity value 1", output1(10,10) < 5, true );
  TEST( "Uncommon intensity value 2", output1(10,11) < 5, true );

  TEST( "Common color value 1", output2(1,1) > 250, true );
  TEST( "Common color value 2", output2(19,19) > 250, true );
  TEST( "Uncommon color value 1", output2(10,10) < 5, true );
  TEST( "Uncommon color value 2", output2(10,11) < 5, true );
}

} // end anonymous namespace

int test_color_commonality_filter_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "color_commonality_filter_process" );

  simple_cc_tests();

  return testlib_test_summary();
}
