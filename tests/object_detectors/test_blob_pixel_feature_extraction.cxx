/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <testlib/testlib_test.h>
#include <boost/lexical_cast.hpp>

#include <object_detectors/blob_pixel_feature_extraction.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;


void test_blob_pixel_feature_extraction()
{
  vil_image_view<bool> input_mask( 20, 20, 3 );
  vil_image_view<vxl_byte> output_features;

  // Fill in some default values
  input_mask.fill( false );
  for( unsigned i = 2; i < 12; i++ )
  {
    input_mask( 5, i, 0 ) = true;
  }
  input_mask( 10, 1, 0 ) = true;
  input_mask( 10, 2, 0 ) = true;

  blob_pixel_features_settings settings;
  settings.minimum_size_pixels = 5;

  extract_blob_pixel_features( input_mask, settings, output_features );

  TEST( "Output Image Size", output_features.nplanes(), 9 );
  TEST( "Output Image Value 1", output_features( 10, 1, 0 ), 0 );
  TEST( "Output Image Value 2", output_features( 10, 1, 0 ), 0 );
  TEST( "Output Image Value 3", output_features( 5, 7, 0 ) > 0, true );
  TEST( "Output Image Value 4", output_features( 5, 11, 0 ) > 0, true );
}

} // end anonymous namespace

int test_blob_pixel_feature_extraction( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "blob_pixel_feature_extraction" );

  test_blob_pixel_feature_extraction();

  return testlib_test_summary();
}
