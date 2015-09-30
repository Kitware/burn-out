/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sample_nodes.h"

#include <pipeline/sync_pipeline.h>
#include <pipeline/async_pipeline.h>
#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


#define RUN_TEST()                                      \
  TEST( "Pipeline initialize", p.initialize(), true );  \
  bool run_status = p.run();                            \
  TEST( "Run", run_status, true);                       \
  TEST( "Number of steps", collect->values_.size(), 7 );\
  TEST( "Value of step 1", collect->values_[0], 1 );    \
  TEST( "Value of step 2", collect->values_[1], 4 );    \
  TEST( "Value of step 3", collect->values_[2], 9 );    \
  TEST( "Value of step 4", collect->values_[3], 16 );   \
  TEST( "Value of step 5", collect->values_[4], 25 );   \
  TEST( "Value of step 6", collect->values_[5], 36 );   \
  TEST( "Value of step 7", collect->values_[6], 49 );

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

  vcl_cout << "-------------------------------------------------\n"
           << "Testing sync_pipeline nested within sync_pipeline\n"
           << "-------------------------------------------------" << vcl_endl;
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

  vcl_cout << "--------------------------------------------------\n"
           << "Testing sync_pipeline nested within async_pipeline\n"
           << "--------------------------------------------------" << vcl_endl;
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

  vcl_cout << "--------------------------------------------------\n"
           << "Testing async_pipeline nested within sync_pipeline\n"
           << "--------------------------------------------------" << vcl_endl;
  RUN_TEST();
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

  vcl_cout << "---------------------------------------------------\n"
           << "Testing async_pipeline nested within async_pipeline\n"
           << "---------------------------------------------------" << vcl_endl;
  RUN_TEST();
}


#undef RUN_TEST


} // end anonymous namespace

int test_nested_pipelines( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_nested_pipelines" );

  test_sync_in_sync();
  test_sync_in_async();
  test_async_in_async();
  //test_async_in_sync();

  return testlib_test_summary();
}
