/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <utilities/property_map.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
test_get_ref()
{
  vcl_cout << "Test get ref\n";

  vidtk::property_map p;

  TEST( "key didn't exist before", p.has( "key" ), false );

  int& v = p.get_or_create_ref<int>( "key" );
  v = 5;

  TEST( "key exists after", p.has( "key" ), true );

  int w = 1;
  TEST( "get value", p.get( "key", w ), true );
  TEST( "reference was really a reference", w, 5 );
}


void
test_is_set()
{
  vcl_cout << "Test is_set\n";

  vidtk::property_map p;

  p.set( "a", true );
  p.set( "b", false );

  TEST( "Non-existent value is false", p.is_set( "c" ), false );
  TEST( "true value is true", p.is_set( "a" ), true );
  TEST( "false value is false", p.is_set( "b" ), false );
}


void
test_get_if_avail()
{
  vcl_cout << "Test get_if_avail\n";

  vidtk::property_map p;

  p.set( "a", 5.6 );

  TEST( "Non-existent value returns NULL", p.get_if_avail<double>( "c" ), NULL );
  double* d = p.get_if_avail<double>( "a" );
  TEST( "Returns pointer to correct value", d != NULL && *d==5.6, true );
  *d = 2.0;
  double x = -1.0;
  TEST( "Get new value normally", p.get( "a", x ), true );
  TEST( "Value was updated", x, 2.0 );

  vidtk::property_map const& q = p;
  TEST( "Non-existent value returns NULL (const)", q.get_if_avail<double>( "c" ), NULL );
  double const* e = q.get_if_avail<double>( "a" );
  TEST( "Returns pointer to correct value (const)", e != NULL && *e==2.0, true );
}


} // end anonymous namespace

int test_property_map( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "property map" );

  test_get_ref();
  test_is_set();
  test_get_if_avail();

  return testlib_test_summary();
}
