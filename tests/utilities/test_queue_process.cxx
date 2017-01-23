/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/queue_process.h>
#include <utilities/queue_process.txx>
#include <utilities/config_block.h>

#include <iostream>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_bad_value()
{
  queue_process<double> t("test");
  config_block blk = t.params();
  blk.set("disable_read", "BOB");
  TEST("cannot set bad value", t.set_params(blk), false);
}

void test_simple_queue()
{
  queue_process<double> t("test");
  config_block blk = t.params();
  blk.set("disable_read", "true");
  blk.set("max_length", "5");
  TEST("can set config", t.set_params(blk), true);
  TEST("can init", t.initialize(), true);
  TEST_EQUAL("Max size is correct", t.get_max_length(), 5);
  double v = 0;
  for(unsigned int i =0; i < 7; ++i, ++v)
  {
    t.set_input_datum(v);
    TEST("can step", t.step2(), process::SKIP);
    if(i == 3)
    {
      TEST("close to full", t.is_close_to_full(), true);
    }
    else
    {
      TEST("not close to full", t.is_close_to_full(), false);
    }
    if(i < 5)
    {
      TEST_EQUAL("length", t.length(), i+1);
    }
    else
    {
      TEST_EQUAL("length", t.length(), 5);
    }
  }
  TEST_EQUAL("index to nearest return large number", t.index_nearest_to( 3 ), unsigned(-1));
  for(unsigned int i = 0; i < 5; ++i)
  {
    TEST_EQUAL("Is expected value", t.datum_at(i), i + 2.0);
  }
  std::vector< double > & data = t.get_data_store();
  for(unsigned int i = 0; i < 5; ++i)
  {
    TEST_EQUAL("Is expected value", data[i], i + 2.0);
  }
  t.set_disable_read( false );
  for(unsigned int i = 0; i < 5; ++i)
  {
    TEST("can step", t.step2(), process::SUCCESS);
    TEST_EQUAL("Is expected value", t.get_output_datum(), i + 2.0);
    TEST_EQUAL("length", t.length(), 4-i);

  }
  TEST("can step", t.step2(), process::FAILURE);
  t.set_disable_read( true );
  t.set_input_datum(4);
  TEST("can step", t.step2(), process::SKIP);
  TEST_EQUAL("length", t.length(), 1);
  t.clear();
  TEST_EQUAL("length", t.length(), 0);
}

void test_timestamp_queue()
{
  queue_process<timestamp> t("test");
  config_block blk = t.params();
  blk.set("disable_read", "true");
  blk.set("max_length", "5");
  TEST("can set config", t.set_params(blk), true);
  TEST("can init", t.initialize(), true);
  timestamp ts_wt[] = {timestamp(100,10), timestamp(150,15), timestamp(200,20), timestamp(250,25)};
  timestamp nearest(210, 24);
  TEST_EQUAL("Test nearest on empty queue", t.index_nearest_to( nearest ), unsigned(-1));
  for(size_t i = 0; i < 4; ++i)
  {
    t.set_input_datum(ts_wt[i]);
    TEST("can step", t.step2(), process::SKIP);
  }
  TEST_EQUAL("Test nearest valid time", t.index_nearest_to( nearest ), 2);
  TEST_EQUAL("Test nearest invalid time", t.index_nearest_to( timestamp() ), unsigned(-1));
}

} // end anonymous namespace

int test_queue_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "queue_process" );

  test_bad_value();
  test_simple_queue();
  test_timestamp_queue();

  return testlib_test_summary();
}
