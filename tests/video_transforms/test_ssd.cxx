/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_iomanip.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>
#include <vil/vil_convert.h>
#include <testlib/testlib_test.h>

#include <video_transforms/ssd.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_fast_square_of_diff()
{
  vxl_byte i = 25;
  vxl_byte j = 23;
  TEST_EQUAL("vxl_byte fast_square_of_diff", fast_square_of_diff(j,i), 4.0f);
  TEST_EQUAL("vxl_byte fast_square_of_diff", fast_square_of_diff(i,j), 4.0f);
  TEST_EQUAL("float fast_square_of_diff", fast_square_of_diff(25.0,23.0), 4.0f);
  TEST_EQUAL("float fast_square_of_diff", fast_square_of_diff(23.0,25.0), 4.0f);
}

void test_find_global_min()
{
  vil_image_view<float> test(1,2);
  test(0,0) = 25.;
  test(0,1) = 23.;
  unsigned mi = 0;
  unsigned mj = 0;
  find_global_minimum(test, mi, mj);
  TEST_EQUAL( "For dim of one, corect min value x", mi, 0 );
  TEST_EQUAL( "For dim of one, corect min value y", mj, 1 );
  test = vil_image_view<float>(2,2);
  test(0,0) = 25.;
  test(0,1) = 23.;
  test(1,0) = 26.;
  test(1,1) = 21.;
  find_global_minimum(test, mi, mj);
  TEST_EQUAL( "For dim of one, corect min value x", mi, 1 );
  TEST_EQUAL( "For dim of one, corect min value y", mj, 1 );
}

// #       tests/data/ssd_test_area.png
// #       tests/data/ssd_test_car.png
// #       tests/data/ssd_test_car_mask.png


void test_get_ssd_surf(vcl_string const& dir)
{
  {
    vil_image_view<vxl_byte> img(30,30);
    vil_image_view<vxl_byte> kernel(33,33);
    vil_image_view<float> dest(30,30);
    TEST("Failed get ssd surf", get_ssd_surf(img, dest, kernel), false);
    vcl_vector< unsigned int > mis, mjs;
    find_local_minima( dest, 2000, 400, 29, 41, mis, mjs);
  }
  {
    vil_image_view<vxl_byte> img =  vil_load((dir + "/ssd_test_area.png").c_str());
    vil_image_view<vxl_byte> kernel = vil_load((dir + "/ssd_test_car.png").c_str());
    vil_image_view<float> dest(1+img.ni()-kernel.ni(),1+img.nj()-kernel.nj());
    TEST("Can get ssd surface", get_ssd_surf(img, dest, kernel), true);
    TEST_NEAR("Test a couple surface values", dest(10, 30),  8707.83203125, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(20, 50),  3281.4951171875, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(117, 121), 227.30499267578125, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(30, 53),   524.02752685546875, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(119, 29), 858.71002197265625, 1e-6);
    vcl_vector< unsigned int > mis, mjs;
    find_local_minima( dest, 900, 1000, kernel.ni(), kernel.nj(), mis, mjs);
    TEST_EQUAL( "Expected number of minimum", mis.size(), 3 );
  }
}

void test_get_ssd_weighted_surf(vcl_string const& dir)
{
  {
    vil_image_view<vxl_byte> img(30,30);
    vil_image_view<vxl_byte> kernel(33,33);
    vil_image_view<float> dest(30,30);
    vil_image_view<float> weight(22,22);
    TEST("Failed get ssd surf", get_ssd_surf(img, dest, kernel, weight), false);
    img = vil_image_view<vxl_byte>(40,40);
    TEST("Failed get ssd surf", get_ssd_surf(img, dest, kernel, weight), false);
  }
  {
    vil_image_view<vxl_byte> img =  vil_load((dir + "/ssd_test_area.png").c_str());
    vil_image_view<vxl_byte> kernel = vil_load((dir + "/ssd_test_car.png").c_str());
    vil_image_view<float> weight;
    {
      vil_image_view<vxl_byte> t = vil_load((dir + "/ssd_test_car_mask.png").c_str());
      weight = vil_image_view<float>(t.ni(), t.nj());
      for(unsigned int i = 0; i < t.ni(); ++i)
      {
        for(unsigned int j = 0; j < t.nj(); ++j)
        {
          weight(i,j) = t(i,j)/255.0f;
        }
      }
    }
    vil_image_view<float> dest(1+img.ni()-kernel.ni(),1+img.nj()-kernel.nj());
    TEST("Can get ssd surface", get_ssd_surf(img, dest, kernel, weight), true);
    TEST_NEAR("Test a couple surface values", dest(10, 30),  7308.205078125, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(20, 50),  2049.199951171875, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(117, 121), 172.7924957275390625, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(30, 53),   248.625, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(119, 29), 565.98748779296875, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(79, 102),  640.25750732421875, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(92, 22),  444.102508544921875, 1e-6);
    TEST_NEAR("Test a couple surface values", dest(117, 121),  172.7924957275390625, 1e-6);

    vcl_cout << vcl_setprecision(25) << dest(10, 30) << " " << dest(20, 50) << " " << dest(117, 121) << " " << dest(30, 53) << " " << dest(119, 29) << vcl_endl;
    vcl_vector< unsigned int > mis, mjs;
    find_local_minima( dest, 900, 1000, kernel.ni(), kernel.nj(), mis, mjs);
    TEST_EQUAL( "Expected number of minimum", mis.size(), 5 );
  }
}


} // end anonymous namespace

int test_ssd( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    vcl_cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "ssd" );

  test_find_global_min();
  test_fast_square_of_diff();
  test_get_ssd_surf(argv[1]);
  test_get_ssd_weighted_surf(argv[1]);

  return testlib_test_summary();
}
