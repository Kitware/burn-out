/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <testlib/testlib_test.h>
#include <utilities/aoi.h>

using namespace vidtk;

namespace //anon
{

#define TEST_LAT_LON(POINT, LAT, LON) \
  TEST_NEAR("TEST LAT " #POINT, a->get_latitude(POINT), LAT, 1e-6);\
  TEST_NEAR("TEST LON " #POINT, a->get_longitude(POINT), LON, 1e-6)

void test_aoi_positive_xy()
{
  aoi_sptr a = new aoi();
  TEST("Does not have geo points", a->has_geo_points(), false);
  a->set_aoi_id(10);
  a->set_aoi_name("aoi 10 1");
  a->set_mission_id( 1 );

  TEST("Setting aoi string", a->set_aoi_crop_string("100x110+1200+1000"), true);
  a->set_geo_point_lat_lon( aoi::UPPER_LEFT, 42.850196, -73.759802);
  a->set_geo_point_lat_lon( aoi::UPPER_RIGHT, 42.849277, -73.759802);
  a->set_geo_point_lat_lon( aoi::LOWER_LEFT, 42.850196, -73.758132);
  a->set_geo_point_lat_lon( aoi::LOWER_RIGHT, 42.849277, -73.758132);
  TEST("Does not have geo points", a->has_geo_points(), true);
  timestamp st_t(1, 3);
  a->set_start_time( st_t );
  timestamp ed_t(1300, 500000);
  a->set_end_time(ed_t);
  a->set_description( "Sample AOI 1");

  TEST_EQUAL("correct id", a->get_aoi_id(), 10);
  TEST_EQUAL("correct mission id", a->get_mission_id(), 1);
  TEST("correct aoi name", a->get_aoi_name(), "aoi 10 1");
  TEST("correct crop string", a->get_aoi_crop_string(), "100x110+1200+1000");
  {
    vnl_vector_fixed<int, 2> wh(100, 110);
    std::cout << a->get_aoi_crop_wh() << std::endl;
    TEST("correct width and height", a->get_aoi_crop_wh(), wh );
    vnl_vector_fixed<int, 2> xy(1200, 1000);
    std::cout <<  a->get_aoi_crop_xy() << std::endl;
    TEST("correct x y", a->get_aoi_crop_xy(), xy);
  }
  TEST_EQUAL("correct start time", a->get_start_time(), 1);
  TEST_EQUAL("correct end time", a->get_end_time(), 1300);
  TEST("correct start timestamp", a->get_start_timestamp(),  st_t);
  TEST("correct start timestamp", a->get_end_timestamp(),  ed_t);
  TEST_EQUAL("correct start frame", a->get_start_frame(), 3);
  TEST_EQUAL("correct end frame", a->get_end_frame(), 500000);
  TEST("correct descriptor", a->get_description(), "Sample AOI 1");

  TEST_LAT_LON(aoi::UPPER_LEFT, 42.850196, -73.759802);
  TEST_LAT_LON(aoi::UPPER_RIGHT, 42.849277, -73.759802);
  TEST_LAT_LON(aoi::LOWER_LEFT, 42.850196, -73.758132);
  TEST_LAT_LON(aoi::LOWER_RIGHT, 42.849277, -73.758132);

  geographic::geo_coords geo_inside(42.849817, -73.758964);
  geographic::geo_coords geo_outside(80.0003, 120.00001);
  int zone = geo_inside.zone();
  bool is_north = geo_inside.is_north();
  TEST("Point inside", a->point_inside( aoi::UTM, geo_inside.easting(), geo_inside.northing(), &zone, &is_north), true );
  TEST("Point inside", a->point_inside( aoi::UTM, geo_inside.easting(), geo_inside.northing()), true );
  TEST("Point inside", a->point_inside( aoi::LAT_LONG, geo_inside.latitude(), geo_inside.longitude()), true );
  TEST("Point inside", a->point_inside( aoi::PIXEL, 1250, 1050), true );
  zone = geo_outside.zone();
  is_north = geo_outside.is_north();
  TEST("Point outside", a->point_inside( aoi::UTM, geo_outside.easting(), geo_outside.northing(), &zone, &is_north), false );
  TEST("Point outside", a->point_inside( aoi::UTM, geo_outside.easting(), geo_outside.northing()), false );
  TEST("Point outside", a->point_inside( aoi::LAT_LONG, geo_outside.latitude(), geo_outside.longitude()), false );
  TEST("Point outside", a->point_inside( aoi::PIXEL, 1350, 1150), false );

  zone = geo_inside.zone();
  is_north = geo_inside.is_north();
  aoi::intersection_mode l = static_cast<aoi::intersection_mode>(int(aoi::UTM)+int(aoi::LAT_LONG)+int(aoi::PIXEL));
  TEST("bad point lable", a->point_inside( l, geo_inside.easting(), geo_inside.northing(), &zone, &is_north), false);
}

void test_bad_crop()
{
  //bad crop string
  aoi_sptr a = new aoi();
  TEST("Setting aoi string", a->set_aoi_crop_string("100x110+1200+1000"), true);
  //Bad crop strings clears out the previous's pixel x, y, w and h with a set values -1 0 0 0.
  TEST("Setting aoi string captital x", a->set_aoi_crop_string("100X110+1200+1000"), false); //capital x does not match
  {
    vnl_vector_fixed<int, 2> wh(0, 0);
    std::cout << a->get_aoi_crop_wh() << std::endl;
    TEST("correct width and height", a->get_aoi_crop_wh(), wh );
    vnl_vector_fixed<int, 2> xy(-1, 0);
    std::cout <<  a->get_aoi_crop_xy() << std::endl;
    TEST("correct x y", a->get_aoi_crop_xy(), xy);
  }
  TEST("Setting aoi string +- translate", a->set_aoi_crop_string("100x110+-1200+-1000"), false);
  TEST("Setting aoi string negative size", a->set_aoi_crop_string("-100x-110+1200+1000"), false);
}

void test_negative_crop()
{
  aoi_sptr a = new aoi();
  TEST("Does not have geo points (NEG)", a->has_geo_points(), false);
  a->set_aoi_id(10);
  a->set_aoi_name("aoi 10 1");
  a->set_mission_id( 1 );

  TEST("Setting aoi string neg translate (NEG)", a->set_aoi_crop_string("100x110-1200-1000"), true);
  a->set_geo_point_lat_lon( aoi::UPPER_LEFT, 42.850196, -73.759802);
  a->set_geo_point_lat_lon( aoi::UPPER_RIGHT, 42.849277, -73.759802);
  a->set_geo_point_lat_lon( aoi::LOWER_LEFT, 42.850196, -73.758132);
  a->set_geo_point_lat_lon( aoi::LOWER_RIGHT, 42.849277, -73.758132);
  TEST("Does have geo points (NEG)", a->has_geo_points(), true);
  timestamp st_t(1, 3);
  a->set_start_time( st_t );
  timestamp ed_t(1300, 500000);
  a->set_end_time(ed_t);
  a->set_description( "Sample AOI 1");

  TEST_EQUAL("correct id (NEG)", a->get_aoi_id(), 10);
  TEST_EQUAL("correct mission id (NEG)", a->get_mission_id(), 1);
  TEST("correct aoi name (NEG)", a->get_aoi_name(), "aoi 10 1");
  TEST("correct crop string (NEG)", a->get_aoi_crop_string(), "100x110-1200-1000");
  {
    vnl_vector_fixed<int, 2> wh(100, 110);
    std::cout << a->get_aoi_crop_wh() << std::endl;
    TEST("correct width and height (NEG)", a->get_aoi_crop_wh(), wh );
    vnl_vector_fixed<int, 2> xy(-1200, -1000);
    std::cout <<  a->get_aoi_crop_xy() << std::endl;
    TEST("correct x y (NEG)", a->get_aoi_crop_xy(), xy);
  }
}

}//namespace anon

int test_aoi( int, char*[] )
{
  testlib_test_start( "test aoi" );


  test_aoi_positive_xy();
  test_bad_crop();
  test_negative_crop();

  return testlib_test_summary();
}
