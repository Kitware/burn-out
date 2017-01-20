/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sample_nodes.h"

#include <pipeline_framework/sync_pipeline.h>
#include <pipeline_framework/async_pipeline.h>
#include <testlib/testlib_test.h>
#include <iostream>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

#define TEST_VALUE(idx)               \
  if (idx <= collect->values_.size()) \
    TEST_EQUAL( "Result", collect->values_[idx - 1], idx * idx)
#define CHECK_TEST(count)                                          \
  TEST_EQUAL( "Number of steps", collect->values_.size(), count ); \
  for (size_t i = 1; i <= count; ++i)                              \
    TEST_VALUE(i)
#define EXEC_TEST()                                    \
  TEST( "Pipeline initialize", p.initialize(), true ); \
  bool run_status = p.run();                           \
  TEST( "Run", run_status, true)
#define RUN_TEST() \
  EXEC_TEST();     \
  CHECK_TEST(7)
#define RUN_FAIL_TEST() \
  EXEC_TEST();          \
  CHECK_TEST(0)

void
test_sync_in_sync()
{
  sync_pipeline p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", 7 ) );
  process_smart_pointer< product_of_increments< sync_pipeline > > prod( new product_of_increments< sync_pipeline >( "sync" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( prod );
  p.add( collect );

  p.connect( nums->value_port(),
             prod->set_value1_port() );
  p.connect( nums->value_port(),
             prod->set_value2_port() );
  p.connect( prod->value_port(),
             collect->set_value_port() );

  std::cout << "-------------------------------------------------\n"
           << "Testing sync_pipeline nested within sync_pipeline\n"
           << "-------------------------------------------------" << std::endl;
  RUN_TEST();
}


void
test_sync_in_async()
{
  async_pipeline p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", 7 ) );
  process_smart_pointer< product_of_increments< sync_pipeline > > prod( new product_of_increments< sync_pipeline >( "sync" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( prod );
  p.add( collect );

  p.connect( nums->value_port(),
             prod->set_value1_port() );
  p.connect( nums->value_port(),
             prod->set_value2_port() );
  p.connect( prod->value_port(),
             collect->set_value_port() );

  std::cout << "--------------------------------------------------\n"
           << "Testing sync_pipeline nested within async_pipeline\n"
           << "--------------------------------------------------" << std::endl;
  RUN_TEST();
}


void
test_async_in_sync()
{
  sync_pipeline p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", 7 ) );
  process_smart_pointer< product_of_increments< async_pipeline > > prod( new product_of_increments< async_pipeline >( "async" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( prod );
  p.add( collect );

  p.connect( nums->value_port(),
             prod->set_value1_port() );
  p.connect( nums->value_port(),
             prod->set_value2_port() );
  p.connect( prod->value_port(),
             collect->set_value_port() );

  std::cout << "--------------------------------------------------\n"
           << "Testing async_pipeline nested within sync_pipeline\n"
           << "--------------------------------------------------" << std::endl;
  RUN_FAIL_TEST();
}


void
test_async_in_async()
{
  async_pipeline p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", 7 ) );
  process_smart_pointer< product_of_increments< async_pipeline > > prod( new product_of_increments< async_pipeline >( "async" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( prod );
  p.add( collect );

  p.connect( nums->value_port(),
             prod->set_value1_port() );
  p.connect( nums->value_port(),
             prod->set_value2_port() );
  p.connect( prod->value_port(),
             collect->set_value_port() );

  std::cout << "---------------------------------------------------\n"
           << "Testing async_pipeline nested within async_pipeline\n"
           << "---------------------------------------------------" << std::endl;
  RUN_TEST();
}


#undef RUN_FAIL_TEST
#undef RUN_TEST
#undef EXEC_TEST
#undef CHECK_TEST
#undef TEST_VALUE


} // end anonymous namespace

int test_nested_pipelines( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_nested_pipelines" );

  test_sync_in_sync();
  test_sync_in_async();
  test_async_in_async();
  test_async_in_sync();

  return testlib_test_summary();
}
