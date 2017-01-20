/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_rgb.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <testlib/testlib_test.h>
#include <pipeline_framework/async_pipeline.h>

#include <video_transforms/video_enhancement_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_config()
{
  video_enhancement_process< vxl_byte > cleaner("enhancement_proc");
  config_block default_params = cleaner.params();
  default_params.set( "disabled", "bob" );
  TEST("Invalid value", cleaner.set_params( default_params ), false);
  default_params.set( "disabled", "true" );
  TEST("Disabled true", cleaner.set_params( default_params ), true);
  TEST("Disabled init", cleaner.initialize(), true);
  vil_image_view< vxl_byte > temp;
  cleaner.set_source_image( temp );
  TEST("Disabled step", cleaner.step(), true);
}

void
test_simple_balancing()
{
  // Fill in some dummy values
  vil_image_view<vxl_byte> dark( 100, 100, 3 );
  vil_image_view<vxl_byte> light( 100, 100, 3 );

  dark.fill( 25 );
  light.fill( 200 );

  // Create a test pipeline
  video_enhancement_process< vxl_byte > cleaner("enhancement_proc");
  config_block default_params = cleaner.params();

  // Simple brightness normalization test
  {
    config_block test_setup = default_params;
    test_setup.set( "smoothing_enabled", "true" );
    test_setup.set( "auto_white_balance", "false" );
    test_setup.set( "normalize_brightness", "true" );
    test_setup.set( "brightness_history_length", "3" );
    test_setup.set( "min_percent_brightness", "0" );

    cleaner.initialize();
    cleaner.set_params( test_setup );

    cleaner.set_source_image( dark );
    cleaner.step();
    cleaner.set_source_image( dark );
    cleaner.step();
    cleaner.set_source_image( dark );
    cleaner.step();

    vil_image_view< vxl_byte > out1 = cleaner.copied_output_image();

    cleaner.set_source_image( dark );
    cleaner.step();
    cleaner.set_source_image( dark );
    cleaner.step();
    cleaner.set_source_image( light );
    cleaner.step();

    vil_image_view< vxl_byte > out2 = cleaner.copied_output_image();

    cleaner.set_source_image( light );
    cleaner.step();
    cleaner.set_source_image( light );
    cleaner.step();
    cleaner.set_source_image( light );
    cleaner.step();

    vil_image_view< vxl_byte > out3 = cleaner.copied_output_image();

    TEST( "Brightness Norm Test: Output Resolution", out1.ni(), 100 );
    TEST( "Brightness Norm Test: Output Planes", out1.nplanes(), 3 );
    TEST( "Brightness Norm Test: Initial Value", out1(50,50,2), 25 );
    TEST( "Brightness Norm Test: Intermediate Range 1", out2(50,50,2) > 25, true );
    TEST( "Brightness Norm Test: Intermediate Range 2", out2(50,50,2) < 200, true );
    TEST( "Brightness Norm Test: Final Value", out3(50,50,2), 200 );
  }

  vil_image_view<vxl_byte> blue_tint( 2, 2, 3 );

  // Perceptually red, green, blue, and light blue pixels with a blue tint
  blue_tint(0,0,0) = 104; blue_tint(0,0,1) = 147; blue_tint(0,0,2) = 215;
  blue_tint(0,1,0) = 47; blue_tint(0,1,1) = 90; blue_tint(0,1,2) = 147;
  blue_tint(1,0,0) = 90; blue_tint(1,0,1) = 90; blue_tint(1,0,2) = 147;
  blue_tint(1,1,0) = 40; blue_tint(1,1,1) = 106; blue_tint(1,1,2) = 147;

  // Rough auto-white balancing test
  {
    config_block test_setup = default_params;
    test_setup.set( "smoothing_enabled", "false" );
    test_setup.set( "auto_white_balance", "true" );
    test_setup.set( "normalize_brightness", "false" );
    test_setup.set( "sampling_rate", "1" );

    cleaner.initialize();
    cleaner.set_params( test_setup );

    cleaner.set_source_image( blue_tint );
    cleaner.step();

    vil_image_view< vxl_byte > out = cleaner.output_image();

    // The brightest pixel should be closer to pure white
    double dist1 = 255-blue_tint(0,0,0)+255-blue_tint(0,0,1)+255-blue_tint(0,0,2);
    double dist2 = 255-out(0,0,0)+255-out(0,0,1)+255-out(0,0,2);
    TEST( "AWB Test: White Reference Point", dist1 > dist2, true );

    // The green pixel should be greener
    double rat1 = static_cast<double>(blue_tint(1,1,1))/blue_tint(1,1,2);
    double rat2 = static_cast<double>(out(1,1,1))/out(1,1,2);
    TEST( "AWB Test: Green Reference Point", rat2 > rat1, true );

    // The red pixel should be redder
    rat1 = static_cast<double>(blue_tint(1,0,1))/blue_tint(1,0,2);
    rat2 = static_cast<double>(out(1,0,1))/out(1,0,2);
    TEST( "AWB Test: Red Reference Point", rat2 > rat1, true );
  }
}

void
test_simple_median_illumination()
{
  vil_image_view<vxl_uint_16> dark( 3, 3, 1 );
  vil_image_view<vxl_uint_16> light( 3, 3, 1 );

  dark(0,0) = 25;
  dark(0,1) = 30;
  dark(0,2) = 31;
  dark(1,0) = 10;
  dark(1,1) = 25;
  dark(1,2) = 30;
  dark(2,0) = 10;
  dark(2,1) = 15;
  dark(2,2) = 25;

  light(0,0) = 50;
  light(0,1) = 60;
  light(0,2) = 62;
  light(1,0) = 20;
  light(1,1) = 50;
  light(1,2) = 60;
  light(2,0) = 20;
  light(2,1) = 30;
  light(2,2) = 50;

  // Create a test pipeline
  video_enhancement_process< vxl_uint_16 > cleaner("enhancement_proc");
  config_block default_params = cleaner.params();
  {
    config_block test_setup = default_params;
    test_setup.set( "disabled", "false" );
    test_setup.set( "smoothing_enabled", "false" );
    test_setup.set( "normalize_brightness", "true" );
    test_setup.set( "normalize_brightness_mode" , "median");
    test_setup.set( "reference_median_mode", "fixed" );
    test_setup.set( "fixed_reference_median", "50");

    cleaner.set_params( test_setup );
    cleaner.initialize();

    cleaner.set_source_image( dark );
    cleaner.step();
    vil_image_view< vxl_uint_16 > out1 = cleaner.copied_output_image();

    cleaner.set_source_image( light );
    cleaner.step();

    vil_image_view< vxl_uint_16 > out2 = cleaner.copied_output_image();

    TEST( "Brightness Median Norm Test: Output Resolution", out1.ni(), 3 );
    TEST( "Brightness Median Norm Test: Output Planes", out1.nplanes(), 1 );
    TEST( "Brightness Median Norm Test: Initial Value", out1(1,1,0), 50 );
    TEST( "Brightness Median Norm Test: Initial Value", out2(1,1,0), 50 );
    for(unsigned int i = 0; i < 3; ++i)
    {
      for(unsigned int j = 0; j < 3; ++j)
      {
        TEST( "Same pixel value", out1(i,j,0), out2(i,j,0) );
      }
    }
  }

  dark(0,0) = 25;
  dark(0,1) = 30;
  dark(0,2) = 31;
  dark(1,0) = 10;
  dark(1,1) = 25;
  dark(1,2) = 30;
  dark(2,0) = 10;
  dark(2,1) = 15;
  dark(2,2) = 25;

  light(0,0) = 50;
  light(0,1) = 60;
  light(0,2) = 62;
  light(1,0) = 20;
  light(1,1) = 50;
  light(1,2) = 60;
  light(2,0) = 20;
  light(2,1) = 30;
  light(2,2) = 50;

  {
    config_block test_setup = default_params;
    test_setup.set( "disabled", "false" );
    test_setup.set( "normalize_brightness", "true" );
    test_setup.set( "normalize_brightness_mode" , "median");
    test_setup.set( "reference_median_mode", "first" );
    test_setup.set( "fixed_reference_median", "50");

    cleaner.set_params( test_setup );
    cleaner.initialize();

    cleaner.set_source_image( dark );
    cleaner.step();
    vil_image_view< vxl_uint_16 > out1 = cleaner.copied_output_image();

    cleaner.set_source_image( light );
    cleaner.step();

    vil_image_view< vxl_uint_16 > out2 = cleaner.copied_output_image();

    TEST( "Brightness Median Norm Test: Output Resolution", out1.ni(), 3 );
    TEST( "Brightness Median Norm Test: Output Planes", out1.nplanes(), 1 );
    TEST( "Brightness Median Norm Test: Initial Value", out1(1,1,0), 25 );
    TEST( "Brightness Median Norm Test: Initial Value", out2(1,1,0), 25 );
    for(unsigned int i = 0; i < 3; ++i)
    {
      for(unsigned int j = 0; j < 3; ++j)
      {
        TEST( "Same pixel value", out1(i,j,0), out2(i,j,0) );
      }
    }
  }
}

} // end anonymous namespace

int test_video_enhancement_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "video_enhancement" );

  test_config();
  test_simple_balancing();
  test_simple_median_illumination();

  return testlib_test_summary();
}
