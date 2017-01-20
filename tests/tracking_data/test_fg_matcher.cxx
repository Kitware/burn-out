/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <vul/vul_temp_filename.h>
#include <vul/vul_file.h>
#include <vpl/vpl.h>
#include <testlib/testlib_test.h>

#include <tracking_data/fg_matcher.h>

using namespace vidtk;

// put everything in an anonymous namespace to avoid conflicts with
// other tests
namespace
{

void test_search_bbox()
{
  {//test out of bound predict
    vnl_double_2 p1(-1, 0);
    vnl_double_2 p2(0, -1);
    vnl_double_2 p3(355, 100);
    vnl_double_2 p4(100, 555);
    fg_search_region_box fgsrb;
    TEST("fails to update of bounds", fgsrb.update(p1,p1,vgl_box_2d<unsigned>(), false, false, 354, 554), false);
    TEST("fails to update of bounds", fgsrb.update(p2,p2,vgl_box_2d<unsigned>(), false, false, 354, 554), false);
    TEST("fails to update of bounds", fgsrb.update(p3,p3,vgl_box_2d<unsigned>(), false, false, 354, 554), false);
    TEST("fails to update of bounds", fgsrb.update(p4,p4,vgl_box_2d<unsigned>(), false, false, 354, 554), false);
  }
  { //test an interal box (bottom point)
    vnl_double_2 pred(104.5, 130.6);
    vnl_double_2 curr(90.3, 110.5);
    vgl_box_2d<unsigned> box(0,20,0,40);
    vnl_double_2 cent = (pred + curr)*0.5;
    fg_search_region_box fgsrb;
    TEST("succeeds to update of bounds", fgsrb.update(curr, pred, box, /*bottom*/true, /*both*/false, 354, 454), true);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[0], pred[0], 1e-15);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[1], pred[1], 1e-15);

    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[0], curr[0], 1e-15);
    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[1], curr[1], 1e-15);

    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[0], cent[0], 1e-15);
    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[1], cent[1], 1e-15);

    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_x(), 94);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_x(), 114);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_y(), 90);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_y(), 130);

    TEST("succeeds to update of bounds", fgsrb.update(curr, pred, box, /*bottom*/true, /*both*/true, 354, 454), true);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[0], pred[0], 1e-15);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[1], pred[1], 1e-15);

    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[0], curr[0], 1e-15);
    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[1], curr[1], 1e-15);

    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[0], cent[0], 1e-15);
    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[1], cent[1], 1e-15);

    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_x(), 80);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_x(), 114);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_y(), 70);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_y(), 130);
  }

  { //test an interal box (centroid point)
    vnl_double_2 pred(104.5, 130.6);
    vnl_double_2 curr(90.3, 110.5);
    vgl_box_2d<unsigned> box(0,20,0,40);
    vnl_double_2 cent = (pred + curr)*0.5;
    fg_search_region_box fgsrb;
    TEST("succeeds to update of bounds", fgsrb.update(curr, pred, box, /*bottom*/false, /*both*/false, 354, 454), true);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[0], pred[0], 1e-15);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[1], pred[1], 1e-15);

    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[0], curr[0], 1e-15);
    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[1], curr[1], 1e-15);

    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[0], cent[0], 1e-15);
    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[1], cent[1], 1e-15);

    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_x(), 94);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_x(), 114);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_y(), 110);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_y(), 150);

    TEST("succeeds to update of bounds", fgsrb.update(curr, pred, box, /*bottom*/false, /*both*/true, 354, 454), true);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[0], pred[0], 1e-15);
    TEST_NEAR("Predicted is what is expected", fgsrb.get_predict_loc_img()[1], pred[1], 1e-15);

    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[0], curr[0], 1e-15);
    TEST_NEAR("Current is what is expected", fgsrb.get_current_loc_img()[1], curr[1], 1e-15);

    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[0], cent[0], 1e-15);
    TEST_NEAR("Center is what is expected", fgsrb.get_centroid_loc_img()[1], cent[1], 1e-15);

    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_x(), 80);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_x(), 114);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().min_y(), 90);
    TEST_EQUAL("BBOX is correct", fgsrb.get_bbox().max_y(), 150);
  }

  { //bounds checking
    vnl_double_2 curr(1, 110.5);
    vgl_box_2d<unsigned> box(0,20,0,40);
    fg_search_region_box fgsrb;
    TEST("check too small 1", fgsrb.update(curr, curr, box, /*bottom*/false, /*both*/false, 354, 454), false);
    curr = vnl_double_2 (100, 1);
    TEST("check too small 2", fgsrb.update(curr, curr, box, /*bottom*/false, /*both*/false, 354, 454), false);
    curr = vnl_double_2 (353, 100);
    TEST("check too small 3", fgsrb.update(curr, curr, box, /*bottom*/false, /*both*/false, 354, 454), false);
    curr = vnl_double_2 (100, 453);
    TEST("check too small 4", fgsrb.update(curr, curr, box, /*bottom*/false, /*both*/false, 354, 454), false);
  }
}

}//namepace anon

/** Main test driver.
 *
 *
 */
int test_fg_matcher( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "fg matcher" );

  test_search_bbox();

  return testlib_test_summary();
}
