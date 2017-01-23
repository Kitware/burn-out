/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/ring_buffer_process.h>
#include <utilities/ring_buffer.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_ring_buffer()
{
  ring_buffer<double> rb;
  rb.set_capacity(5);
  TEST_EQUAL("correct capacity", rb.capacity(), 5);
  TEST_EQUAL("correct size", rb.size(), 0);
  double v;
  for(unsigned i = 0; i < 5; ++i)
  {
    v = i;
    rb.insert(v);
    TEST_EQUAL("correct size", rb.size(), i+1);
    TEST_EQUAL("correct head", rb.head(), (i+1)%5);
    TEST("has data at the current datum", rb.has_datum_at(i), true);
    TEST_EQUAL("correct data", rb.datum_at(0), v);
    TEST_EQUAL("correct capacity", rb.capacity(), 5);
    TEST_EQUAL("offset of is not implemented", rb.offset_of(v), unsigned(-1));
  }
  v = 6;
  rb.insert(v);
  TEST_EQUAL("correct size", rb.size(), 5);
  TEST_EQUAL("correct capacity", rb.capacity(), 5);
  TEST_EQUAL("correct head", rb.head(), 1);
  TEST("has data at the current datum", rb.has_datum_at(4), true);
  TEST_EQUAL("correct data", rb.datum_at(0), v);
  rb.clear();
  TEST_EQUAL("correct size", rb.size(), 0);
}

void
push_back( ring_buffer_process<timestamp>& buf, timestamp const& t )
{
  std::cout << "Insert " << t.time() << "\n";
  buf.set_next_datum( t );
  if( ! buf.step() )
  {
    TEST( "Push back element", false, true );
  }
}

void
test_datum_nearest_to()
{
  std::cout << "\n\nTest datum nearest to\n\n\n";

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

void test_buffer_process()
{
  ring_buffer_process< timestamp > buffer( "buf" );
  config_block blk = buffer.params();
  blk.set( "length", 4 );
  blk.set( "disable_capacity_error", "true" );
  TEST( "Configure buffer", buffer.set_params( blk ), true );
  TEST( "Initialize buffer", buffer.initialize(), true );
  push_back( buffer, timestamp(0) );
  TEST("correct data at 0", buffer.datum_at(0), timestamp(0));
  TEST_EQUAL("correct size", buffer.size(), 1);

  push_back( buffer, timestamp(1) );
  TEST("correct data at 0", buffer.datum_at(0), timestamp(1));
  TEST("correct data at 1", buffer.datum_at(1), timestamp(0));
  TEST_EQUAL("correct size", buffer.size(), 2);

  push_back( buffer, timestamp(2) );
  TEST("correct data at 0", buffer.datum_at(0), timestamp(2));
  TEST("correct data at 1", buffer.datum_at(1), timestamp(1));
  TEST("correct data at 2", buffer.datum_at(2), timestamp(0));
  TEST_EQUAL("correct size", buffer.size(), 3);

  push_back( buffer, timestamp(3) );
  TEST("correct data at 0", buffer.datum_at(0), timestamp(3));
  TEST("correct data at 1", buffer.datum_at(1), timestamp(2));
  TEST("correct data at 2", buffer.datum_at(2), timestamp(1));
  TEST("correct data at 3", buffer.datum_at(3), timestamp(0));
  TEST_EQUAL("correct size", buffer.size(), 4);

  push_back( buffer, timestamp(4) );
  TEST("correct data at 0", buffer.datum_at(0), timestamp(4));
  TEST("correct data at 1", buffer.datum_at(1), timestamp(3));
  TEST("correct data at 2", buffer.datum_at(2), timestamp(2));
  TEST("correct data at 3", buffer.datum_at(3), timestamp(1));
  TEST_EQUAL("correct size", buffer.size(), 4);

  push_back( buffer, timestamp(5) );
  TEST("correct data at 0", buffer.datum_at(0), timestamp(5));
  TEST("correct data at 1", buffer.datum_at(1), timestamp(4));
  TEST("correct data at 2", buffer.datum_at(2), timestamp(3));
  TEST("correct data at 3", buffer.datum_at(3), timestamp(2));
  TEST_EQUAL("correct size", buffer.size(), 4);

  TEST_EQUAL("gets corrent offset", buffer.offset_of(timestamp(3)), 2);
  TEST_EQUAL("gets corrent offset", buffer.offset_of(timestamp(5)), 0);
  TEST_EQUAL("gets corrent offset", buffer.offset_of(timestamp(2)), 3);
  TEST_EQUAL("gets corrent offset", buffer.offset_of(timestamp(4)), 1);
  TEST_EQUAL("Time is not present", buffer.offset_of(timestamp(1)), unsigned(-1));

  TEST("No new data step", buffer.step(), false);
}

void test_bad_config()
{
  ring_buffer_process< timestamp > buffer( "buf_bad_config" );
  config_block blk = buffer.params();
  blk.set( "length", "0" );
  TEST( "Configure buffer (bad)", buffer.set_params( blk ), false );
}

void test_disable()
{
  ring_buffer_process< timestamp > buffer( "buf_bad_config" );
  config_block blk = buffer.params();
  blk.set( "disabled", "true" );
  TEST( "Configure buffer (disable)", buffer.set_params( blk ), true );
  TEST( "Initialize buffer (disable)", buffer.initialize(), true );
  TEST( "Can step (disable)", buffer.step(), true);
}


} // end anonymous namespace

int test_ring_buffer_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "ring_buffer_process" );

  test_datum_nearest_to();
  test_ring_buffer();
  test_bad_config();
  test_disable();
  test_buffer_process();

  return testlib_test_summary();
}
