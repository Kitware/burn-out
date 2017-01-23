/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <iomanip>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>
#include <vil/vil_convert.h>
#include <testlib/testlib_test.h>

#include <video_transforms/ndi.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

// #       tests/data/ssd_test_area.png
// #       tests/data/ssd_test_car.png

void test_get_ndi_surf(std::string const& dir)
{
  {

    vil_image_view<vxl_byte> img =  vil_load((dir + "/ssd_test_area.png").c_str());
    vil_image_view<vxl_byte> kernel = vil_load((dir + "/ssd_test_car.png").c_str());
    vil_image_view<float> di_im(img.ni(),img.nj());
    vil_image_view<float> ndi_im(img.ni(),img.nj());

    TEST("Can get ndi surface", get_ndi_surf(img, kernel, di_im, ndi_im), true);

    TEST_NEAR("Test di surface values", di_im(92, 37), 3.18213, 1e-5);
    TEST_NEAR("Test di surface values", di_im(33, 53), 3.25233, 1e-5);
    TEST_NEAR("Test di surface values", di_im(116, 118), 3.26555, 1e-5);
    TEST_NEAR("Test di surface values", di_im(119, 28), 3.33096, 1e-5);
    TEST_NEAR("Test di surface values", di_im(73, 109), 3.42849, 1e-5);
    TEST_NEAR("Test di surface values", di_im(67, 51), 3.6939, 1e-5);
    TEST_NEAR("Test di surface values", di_im(108, 114), 3.75321, 1e-5);
    TEST_NEAR("Test di surface values", di_im(100, 100), 4.41432, 1e-5);
    TEST_NEAR("Test di surface values", di_im(150, 150), 5.90245, 1e-5);

    std::vector< unsigned int > mis, mjs;
    find_local_minima_ndi( di_im, 1000, kernel.ni(), kernel.nj(), mis, mjs);
    TEST_EQUAL( "Expected number of minimum", mis.size(), 10 );

    TEST_NEAR("Test ndi surface values", ndi_im(33, 53), 0.379552, 1e-5);
    TEST_NEAR("Test ndi surface values", ndi_im(116, 118), 0.379545, 1e-5);
    TEST_NEAR("Test ndi surface values", ndi_im(119, 28), 0.388275, 1e-5);
    TEST_NEAR("Test ndi surface values", ndi_im(73, 109), 0.399411, 1e-5);
    TEST_NEAR("Test ndi surface values", ndi_im(67, 51), 0.433044, 1e-5);
    TEST_NEAR("Test ndi surface values", ndi_im(108, 114), 0.434708, 1e-5);
    TEST_NEAR("Test ndi surface values", ndi_im(100, 100), 0.515874, 1e-5);
    TEST_NEAR("Test ndi surface values", ndi_im(150, 150), 0.702981, 1e-5);

    mis.clear();
    mjs.clear();
    find_local_minima_ndi( ndi_im, 1000, kernel.ni(), kernel.nj(), mis, mjs);
    TEST_EQUAL( "Expected number of minimum", mis.size(), 10 );
  }
}


} // end anonymous namespace

int test_ndi( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "ndi" );

  test_get_ndi_surf(argv[1]);

  return testlib_test_summary();
}
