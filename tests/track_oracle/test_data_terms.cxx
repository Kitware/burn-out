/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <track_oracle/track_oracle.h>
#include <track_oracle/data_terms/data_terms.h>
#include <track_oracle/track_field.h>
#include <sstream>

#include <testlib/testlib_test.h>

using std::ostringstream;

namespace // anon
{
using namespace vidtk;

void test_default_values()
{
  track_field< dt::tracking::world_gcs > gcs;
  track_field< dt::tracking::world_x > world_x;

  oracle_entry_handle_type row = track_oracle::get_next_handle();
  {
    ostringstream oss;
    dt::tracking::world_gcs::Type val = gcs( row );
    dt::tracking::world_gcs::Type d = dt::tracking::world_gcs::get_default_value();

    oss << "gcs default: is " << val << "; should be " << d;
    TEST( oss.str().c_str(), val, d);
  }
  {
    ostringstream oss;
    dt::tracking::world_x::Type val = world_x( row );
    dt::tracking::world_x::Type d = 0.0;

    oss << "world_x default: is " << val << "; should be " << d;
    TEST( oss.str().c_str(), val, d);
  }

  const dt::tracking::world_gcs::Type new_gcs = 51717;
  const dt::tracking::world_x::Type new_x = 361.0;
  gcs( row ) = new_gcs;
  world_x( row ) = new_x;

  {
    ostringstream oss;
    dt::tracking::world_gcs::Type val = gcs( row );

    oss << "gcs mutated: is " << val << "; should be " << new_gcs;
    TEST( oss.str().c_str(), val, new_gcs);
  }
  {
    ostringstream oss;
    dt::tracking::world_x::Type val = world_x( row );

    oss << "world_x mutated: is " << val << "; should be " << new_x;
    TEST( oss.str().c_str(), val, new_x);
  }
}

} // anon

int test_data_terms( int, char *[] )
{
  testlib_test_start( "test_data_terms" );

  test_default_values();

  return testlib_test_summary();
}
