/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <vul/vul_temp_filename.h>
#include <vpl/vpl.h>
#include <vsl/vsl_binary_io.h>
#include <testlib/testlib_test.h>

#include <utilities/horizon_detection_functions.h>
#include <utilities/vsl/timestamp_io.h>

namespace //annon
{

void test_identity()
{
  vgl_h_matrix_2d<double> H2d;
  H2d.set_identity();

  TEST("Does not have a horizon line", vidtk::contains_horizon_line( H2d ), false  );
  TEST("Does not have a horizon line", vidtk::is_region_above_horizon( vgl_box_2d<unsigned>(1,10,2,20), H2d ), false  );
  TEST("Does not have a horizon line", vidtk::is_point_above_horizon( vgl_point_2d<unsigned>(1,10), H2d ), false  );
  TEST("Does not have a horizon line", vidtk::does_horizon_intersect( 1000, 1200, H2d ), false  );
}

void test_homography()
{
  vnl_double_3x3 H;
  H(0,0) = -0.00029320106;  H(0,1) = 0.0012073065;   H(0,2) = -0.59184241;
  H(1,0) =  0.00050597447;  H(1,1) = 0.00096643864;  H(1,2) = -0.80604973;
  H(2,0) =  7.7224635e-008; H(2,1) = -4.584715e-005; H(2,2) =  0.0020273329;
  vgl_h_matrix_2d<double> H2d;
  H2d.set(H);

  TEST("Has a horizon line", vidtk::contains_horizon_line( H2d ), true  );
  TEST("box above", vidtk::is_region_above_horizon( vgl_box_2d<unsigned>(1,10,2,10), H2d ), true  );
  TEST("box below", vidtk::is_region_above_horizon( vgl_box_2d<unsigned>(1,10,200,210), H2d ), false  );
  TEST("point above", vidtk::is_point_above_horizon( vgl_point_2d<unsigned>(1,10), H2d ), true  );
  TEST("point below", vidtk::is_point_above_horizon( vgl_point_2d<unsigned>(201,210), H2d ), false  );
  TEST("Does not intersect", vidtk::does_horizon_intersect( 10, 10, H2d ), false  );
  TEST("intersects", vidtk::does_horizon_intersect( 2000, 1000, H2d ), true  );
}

}//namespace annon

int test_horizon_detection_functions( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "horizon detection functions" );

  test_identity();
  test_homography();

  return testlib_test_summary();
}
