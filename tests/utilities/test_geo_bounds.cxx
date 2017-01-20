/*ckwg +5
 * Copyright 2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/geo_coordinate.h>

int test_geo_bounds( int, char*[] )
{
  testlib_test_start( "test_geo_bounds" );

  double l_easting = 200100;
  double r_easting = 200000;
  double u_northing = 2000100;
  double l_northing = 2000000;
  double eps = 0.0001;

  vidtk::geo_coord::geo_coordinate ul( 17, true, l_easting, u_northing );
  vidtk::geo_coord::geo_coordinate ll( 17, true, l_easting, l_northing );
  vidtk::geo_coord::geo_coordinate lr( 17, true, r_easting, l_northing );
  vidtk::geo_coord::geo_coordinate ur( 17, true, r_easting, u_northing );

  vidtk::geo_coord::geo_bounds bounds;
  bounds.add( ul );
  bounds.add( ll );
  bounds.add( lr );
  bounds.add( ur );

  vidtk::geo_coord::geo_coordinate ul_coord = bounds.get_upper_left();
  vidtk::geo_coord::geo_UTM ul_utm = ul_coord.get_utm();
  vidtk::geo_coord::geo_coordinate ur_coord = bounds.get_upper_right();
  vidtk::geo_coord::geo_UTM ur_utm = ur_coord.get_utm();
  vidtk::geo_coord::geo_coordinate ll_coord = bounds.get_lower_left();
  vidtk::geo_coord::geo_UTM ll_utm = ll_coord.get_utm();
  vidtk::geo_coord::geo_coordinate lr_coord = bounds.get_lower_right();
  vidtk::geo_coord::geo_UTM lr_utm = lr_coord.get_utm();
  TEST_NEAR( "upper left easting returned from bounds is same as added upper left",
             ul_utm.get_easting(), l_easting, eps );
  TEST_NEAR( "upper left northing returned from bounds is same as added upper left",
             ul_utm.get_northing(), u_northing, eps );
  TEST_NEAR( "lower left easting returned from bounds is same as added lower left",
             ll_utm.get_easting(), l_easting, eps );
  TEST_NEAR( "lower left northing returned from bounds is same as added lower left",
             ll_utm.get_northing(), l_northing, eps );
  TEST_NEAR( "lower right easting returned from bounds is same as added lower right",
             lr_utm.get_easting(), r_easting, eps );
  TEST_NEAR( "lower right northing returned from bounds is same as added lower right",
             lr_utm.get_northing(), l_northing, eps );
  TEST_NEAR( "upper right easting returned from bounds is same as added upper right",
             ur_utm.get_easting(), r_easting, eps );
  TEST_NEAR( "upper right northing returned from bounds is same as added upper right",
             ur_utm.get_northing(), u_northing, eps );

  return testlib_test_summary();
}
