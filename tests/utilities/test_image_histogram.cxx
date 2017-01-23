/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "utilities/image_histogram.h"
#include <iostream>
#include "vil/vil_load.h"
#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


void
test_histogram_pair( vidtk::image_histogram const & h1,
                     vidtk::image_histogram const & h2 )
{
  //std::cout << "masses: " << h1.mass() << ", " << h2.mass() << std::endl;

  double score_12 = h1.compare( h2.get_h() );
  double score_21 = h2.compare( h1.get_h() );
  //std::cout << "Diff: 1->2: " << score_12  << "; 2->1: " << score_12 << std::endl;
  TEST( "Histogram compare() symmetry.", score_21, score_12 );
}

void
test_simple( vil_image_view< vxl_byte > & img1,
             vil_image_view< vxl_byte > & img2 )
{
  vidtk::image_histogram h1 =
    vidtk::image_histogram_functions<vxl_byte, bool>::generate( img1,
                                                                std::vector<unsigned>( img1.nplanes(), 8 ) );
  vidtk::image_histogram h2 =
    vidtk::image_histogram_functions<vxl_byte, bool>::generate( img2,
                                                                std::vector<unsigned>( img2.nplanes(), 8 ) );

  std::cout << " First test...\n";

  test_histogram_pair( h1, h2 );
}

void
test_masks( vil_image_view< vxl_byte > & img1,
            vil_image_view< vxl_byte > & img2 )
{
  vidtk::image_histogram h3;
  vidtk::image_histogram h4;

  if( img1.nplanes() == 3 )
  {
    h3.set_type( vidtk::VIDTK_RGB );
    h4.set_type( vidtk::VIDTK_RGB );
    vidtk::image_histogram_functions<vxl_byte, float>::init( h3, img1, std::vector< unsigned >( 3, 4 ) );
    vidtk::image_histogram_functions<vxl_byte, bool>::init( h4, img2, std::vector< unsigned >( 3, 4 ) );
  }
  else
  {
    h3.set_type( vidtk::VIDTK_GRAYSCALE );
    h4.set_type( vidtk::VIDTK_GRAYSCALE );
    vidtk::image_histogram_functions<vxl_byte, float>::init( h3, img1, std::vector< unsigned >( 1, 4 ) );
    vidtk::image_histogram_functions<vxl_byte, bool>::init( h4, img2, std::vector< unsigned >( 1, 4 ) );
  }

  vil_image_view< float > h3_mask( img1.ni(), img1.nj() );
  h3_mask.fill( 1.0 );
  vidtk::image_histogram_functions<vxl_byte, float>::add( h3, img1, &h3_mask );

  vil_image_view< bool > h4_mask( img2.ni(), img2.nj() );
  h4_mask.fill( true );
  vidtk::image_histogram_functions<vxl_byte, bool>::add( h4, img2 );
  vidtk::image_histogram_functions<vxl_byte, bool>::add( h4, img2, &h4_mask );

  std::cout << " Second test...\n";

  test_histogram_pair( h3, h4 );
}

void test_img_pair( std::string img1_name, std::string img2_name )
{
  vil_image_view<vxl_byte> img1 = vil_load( img1_name.c_str() );
  TEST( "Loading first input image file.", img1 != NULL, true );

  vil_image_view<vxl_byte> img2 = vil_load( img2_name.c_str() );
  TEST( "Loading second input image file.", img2 != NULL, true );

  TEST( "Input images have inconsistent number of planes",
    img1.nplanes(), img2.nplanes() );

  // Test 1
  test_simple( img1, img2 );

  // Test 2
  test_masks( img1, img2 );

  return;
}

void test_shifted_imgs( std::string img1_name, std::string img2_name )
{
  vil_image_view<vxl_byte> img1 = vil_load( img1_name.c_str() );
  TEST( "Loading first input image file.", img1 != NULL, true );

  vil_image_view<vxl_byte> img2 = vil_load( img2_name.c_str() );
  TEST( "Loading second input image file.", img2 != NULL, true );

  TEST( "Input images have inconsistent number of planes",
    img1.nplanes(), img2.nplanes() );

  vidtk::image_histogram h1 =
    vidtk::image_histogram_functions<vxl_byte, bool>::generate( img1,
                                                                std::vector<unsigned>( img1.nplanes(), 8 ) );

  vidtk::image_histogram h2 =
    vidtk::image_histogram_functions<vxl_byte, bool>::generate( img2,
                                                                std::vector<unsigned>( img2.nplanes(), 8 ) );

  TEST( "Histograms are identical.", h1.compare( h2 ) == 1.0, true );

  return;
}

} // anonymous namespace

int test_image_histogram(int argc, char *argv[])
{
  testlib_test_start( "image_histogram" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    std::string data_dir( argv[1] );

    std::cout<< "Testing binary images\n";
    test_shifted_imgs( data_dir + "/homog_test_mask_0.pgm",
                       data_dir + "/homog_test_mask_1.pgm" );

    std::cout<< "Testing grayscale images\n";
    test_img_pair( data_dir + "/foursquare.png",
                   data_dir + "/foursquare_trans+7+4.png" );

    std::cout<< "Testing colored images\n";
    test_img_pair( data_dir + "/aph_imgs00001.png",
                   data_dir + "/aph_imgs00002.png" );
  }

  return testlib_test_summary();
}
