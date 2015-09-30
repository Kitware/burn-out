/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <utilities/uuid_able.h>

int test_uuid( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "uuid" );
  vidtk::uuid_able<int> u1;
  vidtk::uuid_able<int> u2;
  TEST( "Test uniqueness of ids", u1.get_uuid()==u2.get_uuid(), false );
  vidtk::uuid_able<int> copy(u1);
  TEST( "Test copy", u1.get_uuid()==copy.get_uuid(), true );
  vidtk::uuid_able<int> e = u1;
  TEST( "Test assign", u1.get_uuid()==e.get_uuid(), true );
  vidtk::uuid_t old = u2.get_uuid();
  u2.regenerate_uuid();
  TEST( "Test regenerate", old==u2.get_uuid(), false );

  return testlib_test_summary();
}
