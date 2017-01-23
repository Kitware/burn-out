/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/pixel_feature_extractor_super_process.h>

#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <testlib/testlib_test.h>

#include <pipeline_framework/async_pipeline.h>

#include <iostream>
#include <algorithm>

#include <boost/lexical_cast.hpp>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;


void test_pixel_feature_extraction()
{
  vil_image_view<vxl_byte> input_intensity( 20, 20, 1 );
  vil_image_view<vxl_byte> input_color( 20, 20, 3 );
  image_border input_border( 1, 19, 1, 19 );

  input_intensity.fill( 100 );
  input_color.fill( 100 );

  input_intensity( 10, 10 ) = 0;
  input_intensity( 10, 11 ) = 255;

  input_color( 10, 10, 0 ) = 0;
  input_color( 10, 11, 0 ) = 255;

  // Test standard case
  pixel_feature_extractor_super_process<vxl_byte,vxl_byte> feature_proc("test_proc");

  config_block blk = feature_proc.params();
  blk.set( "run_async", "false" );
  blk.set( "enable_raw_color_image", "true" );
  blk.set( "enable_color_normalization_filter", "true" );
  blk.set( "enable_high_pass_filter_1", "true" );

  TEST( "Configure Standard", feature_proc.set_params( blk ), true );

  feature_proc.set_source_color_image( input_color );
  feature_proc.set_source_grey_image( input_intensity );
  feature_proc.set_border( input_border );

  feature_proc.step2();

  std::vector< vil_image_view<vxl_byte> > output = feature_proc.feature_array();

  TEST( "Output Array Total Size Standard", output.size() > 0, true );

  for( unsigned i = 0; i < output.size(); i++ )
  {
    std::string info = "Output Index " + boost::lexical_cast<std::string>(i) + " Size";
    TEST( info.c_str(), output[i].ni() == 20 && output[i].nj() == 20, true );
  }

  // Test disabled flag
  blk.set( "disabled", "true" );

  TEST( "Configure Disabled", feature_proc.set_params( blk ), true );

  feature_proc.set_source_color_image( input_color );
  feature_proc.set_source_grey_image( input_intensity );
  feature_proc.set_border( input_border );

  feature_proc.step2();

  output = feature_proc.feature_array();

  TEST( "Output Array Total Size Disabled", output.size(), 0 );
}

} // end anonymous namespace

int test_pixel_feature_extraction_super_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "pixel_feature_extraction_super_process" );

  test_pixel_feature_extraction();

  return testlib_test_summary();
}
