/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/geo_coordinate.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

} // end anonymous

using namespace ::vidtk::geo_coord;


int test_geo_coord( int, char*[] )
{
  testlib_test_start( "geo_coord" );

  geo_lat_lon ll_1;

  TEST( "ll is empty", ll_1.is_empty(), true );
  TEST( "ll is valid", ll_1.is_valid(), false );

  ll_1.set_latitude( 35.123 );
  TEST( "ll is empty 2", ll_1.is_empty(), false );
  TEST( "ll is valid 2", ll_1.is_valid(), false );

  ll_1.set_longitude( 456.7 );
  TEST( "ll is valid 2a", ll_1.is_valid(), false );

  ll_1.set_longitude( 45.67 );
  TEST( "ll is empty 3", ll_1.is_empty(), false );
  TEST( "ll is valid 3", ll_1.is_valid(), true );

  geo_coordinate ll_coord = geo_coordinate( ll_1 );
  geo_UTM utm_1 = ll_coord.get_utm();
  TEST( "utm is valid 3", utm_1.is_valid(), true );

  geo_MGRS mgrs_1 = ll_coord.get_mgrs();
  TEST( "mgrs is valid 3", mgrs_1.is_valid(), true );

  geo_coordinate utm_coord = geo_coordinate( utm_1 );
  geo_lat_lon ll_2 = utm_coord.get_lat_lon();
  std::cout << "ll-s: " << ll_1 << " ll-2: " << ll_2 << std::endl;
  TEST_NEAR( "ll - utm - ll", ll_1.get_latitude(), ll_2.get_latitude(), 1e-5 );

  geo_coordinate mgrs_coord = geo_coordinate( mgrs_1 );
  ll_2 = mgrs_coord.get_lat_lon();
  std::cout << "ll-s: " << ll_1 << " ll-2: " << ll_2 << std::endl;
  TEST_NEAR( "ll - mgrs - ll", ll_1.get_latitude(), ll_2.get_latitude(), 1e-5 );

  geo_coordinate ll_2_coord = geo_coordinate( ll_2 );
  geo_MGRS mgrs_2 = ll_2_coord.get_mgrs();
  std::cout << "mgrs-1: " << mgrs_1 << " mgrs-2: " << mgrs_2 << std::endl;
  TEST( "mgrs-mgrs", mgrs_1 == mgrs_2, true );

  geo_lat_lon ll_3(42.565, -89.4883);
  TEST( "ll not equal", ll_1 != ll_3, true );
  TEST( "ll not equal", ll_1 == ll_3, false );

  { //test sets
    geo_UTM test1(18, true, 596658, 4746691), test2, test3(16, true, 295750, 4715512);
    TEST( "test valid", test2.is_valid(), false );
    test2.set_zone(0);
    TEST( "test valid", test2.is_valid(), false );
    test2.set_zone(18);
    TEST( "test valid", test2.is_valid(), false );
    test2.set_is_north(true);
    test2.set_easting(596658);
    TEST( "test valid", test2.is_valid(), false );
    test2.set_northing(4746691);
    TEST( "test valid", test2.is_valid(), true );
    TEST( "not equal same", test1 != test2, false);
    TEST( "not equal different", test1 != test3, true);
    TEST( "equal same", test1 == test2, true);
    TEST( "equal different", test1 == test3, false);
    geo_coordinate test1_coord = geo_coordinate( test1 );
    TEST( "converting utm to mgrs", test1_coord.get_mgrs().get_coord(), "18TWN9665846691");
  }

  {
    geo_MGRS test_1("18TWN9665846691"), test_2, test_3("16TBN9575015512");
    TEST( "test valid", test_2.is_valid(), false );
    test_2.set_coord("18TWN9665846691");
    TEST( "test valid", test_2.is_valid(), true );
    TEST( "not equal same", test_1 != test_2, false);
    TEST( "not equal different", test_1 != test_3, true);
    TEST( "equal same", test_1 == test_2, true);
    TEST( "equal different", test_1 == test_3, false);

    geo_coordinate test_1_coord = geo_coordinate( test_1 );
    geo_UTM r = test_1_coord.get_utm();
    TEST_EQUAL( "converting mgrs to utm", r.get_zone(), 18 );
    TEST( "converting mgrs to utm", r.is_north(), true );
    TEST_NEAR( "converting mgrs to utm", r.get_easting(), 596658.5, 1e-6 );
    TEST_NEAR( "converting mgrs to utm", r.get_northing(), 4746691.5, 1e-6 );
  }

  return testlib_test_summary();
}
