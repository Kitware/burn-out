/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_algorithm.h>

#include <vil/vil_image_view.h>

#include <testlib/testlib_test.h>

#include <video_transforms/kmeans_segmentation.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void simple_kmeans_tests()
{
  vil_image_view<vxl_byte> color_image( 20, 20, 1 ), label_image;
  color_image.fill( 0 );
  color_image( 10, 10 ) = 255;
  color_image( 10, 11 ) = 254;
  color_image( 10, 12 ) = 253;
  color_image( 10, 13 ) = 51;
  color_image( 10, 14 ) = 50;
  color_image( 10, 15 ) = 52;

  bool success = segment_image_kmeans( color_image, label_image, 3 );

#if defined USE_OPENCV || defined USE_MUL

  TEST( "Clustering Success", success, true );

#endif

  if( success )
  {
    vxl_byte low_value = label_image( 0, 0 );
    vxl_byte mid_value = label_image( 10, 13 );
    vxl_byte high_value = label_image( 10, 11 );

    TEST( "Kmeans Label Value 1", low_value != high_value, true );
    TEST( "Kmeans Label Value 2", label_image( 10, 15 ), mid_value );
    TEST( "Kmeans Label Value 3", label_image( 10, 12 ), high_value );
    TEST( "Kmeans Label Value 4", label_image( 10, 10 ), high_value );
    TEST( "Kmeans Label Value 5", label_image( 10, 14 ), mid_value );
    TEST( "Kmeans Label Value 6", label_image( 10, 1 ), low_value );
  }
}

} // end anonymous namespace

int test_kmeans_segmentation( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "kmeans_segmentation" );

  simple_kmeans_tests();

  return testlib_test_summary();
}
