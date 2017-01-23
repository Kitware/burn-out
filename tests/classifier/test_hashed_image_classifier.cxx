/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_plane.h>

#include <vil/algo/vil_threshold.h>

#include <testlib/testlib_test.h>

#include <classifier/hashed_image_classifier.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_hashed_image_classifier( const std::string& data_dir )
{
  hashed_image_classifier< vxl_byte, double > classifier;

  // Loading
  bool success = classifier.load_from_file( data_dir + "/simple_hashed_img_classifier.clfr" );

  TEST( "File reading", success, true );
  TEST( "Loaded feature count", classifier.feature_count(), 3 );
  TEST( "Is valid flag", classifier.is_valid(), true );

  // Simple application
  std::string mask_path = data_dir + "/obstruction_img_mask.png";
  std::string img_path = data_dir + "/obstruction_img_1.png";
  vil_image_view< vxl_byte > mask_input = vil_load( mask_path.c_str() );
  vil_image_view< vxl_byte > test_input = vil_load( img_path.c_str() );

  vil_image_view< bool > mask_image;
  vil_threshold_above< vxl_byte >( mask_input, mask_image, 1 );

  vil_image_view< double > classified_img;

  std::vector< vil_image_view< vxl_byte > > features;
  features.push_back( vil_plane( test_input, 0 ) );
  features.push_back( vil_plane( test_input, 1 ) );
  features.push_back( vil_plane( test_input, 2 ) );

  // Process without mask
  classifier.classify_images( features, classified_img );

  TEST( "Classified image wo mask value 1", classified_img( 4, 4 ) > 0, true );
  TEST( "Classified image wo mask value 2", classified_img( 15, 15 ) > 0, false );

  classified_img.fill( -0.01 );
  classifier.classify_images( features, mask_image, classified_img );

  TEST( "Classified image w mask value 1", classified_img( 4, 4 ) > 0, false );
  TEST( "Classified image w mask value 2", classified_img( 46, 46 ) > 0, true );
}

}

int test_hashed_image_classifier( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument" << std::endl;
    return EXIT_FAILURE;
  }

  std::string dir = argv[1];

  testlib_test_start( "hashed_image_classifier" );

  test_hashed_image_classifier( dir );

  return testlib_test_summary();
}
