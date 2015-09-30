/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/ring_buffer_process.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;



void
push_back( ring_buffer_process<timestamp>& buf, timestamp const& t )
{
  vcl_cout << "Insert " << t.time() << "\n";
  buf.set_next_datum( t );
  if( ! buf.step() )
  {
    TEST( "Push back element", false, true );
  }
}

void
test_datum_nearest_to()
{
  vcl_cout << "\n\nTest datum nearest to\n\n\n";

  ring_buffer_process< timestamp > buffer( "buf" );
  config_block blk = buffer.params();
  blk.set( "length", 4 );
  blk.set( "disable_capacity_error", "true" );
  TEST( "Configure buffer", buffer.set_params( blk ), true );
  TEST( "Initialize buffer", buffer.initialize(), true );

  push_back( buffer, timestamp(0) );
  TEST( "Nearest to 2 -> 0", buffer.datum_nearest_to(2), timestamp(0) );
  TEST( "Nearest to 6 -> 0", buffer.datum_nearest_to(6), timestamp(0) );

  push_back( buffer, timestamp(1) );
  TEST( "Nearest to 2 -> 0", buffer.datum_nearest_to(2), timestamp(0) );
  TEST( "Nearest to 6 -> 0", buffer.datum_nearest_to(6), timestamp(0) );

  push_back( buffer, timestamp(2) );
  TEST( "Nearest to 2 -> 0", buffer.datum_nearest_to(2), timestamp(0) );
  TEST( "Nearest to 6 -> 0", buffer.datum_nearest_to(6), timestamp(0) );

  push_back( buffer, timestamp(3) );
  TEST( "Nearest to 2 -> 1", buffer.datum_nearest_to(2), timestamp(1) );
  TEST( "Nearest to 6 -> 0", buffer.datum_nearest_to(6), timestamp(0) );

  push_back( buffer, timestamp(4) );
  TEST( "Nearest to 2 -> 2", buffer.datum_nearest_to(2), timestamp(2) );
  TEST( "Nearest to 6 -> 1", buffer.datum_nearest_to(6), timestamp(1) );

  push_back( buffer, timestamp(5) );
  TEST( "Nearest to 2 -> 3", buffer.datum_nearest_to(2), timestamp(3) );
  TEST( "Nearest to 6 -> 2", buffer.datum_nearest_to(6), timestamp(2) );
}


} // end anonymous namespace

int test_ring_buffer_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "ring_buffer_process" );

  test_datum_nearest_to();

  return testlib_test_summary();
}
