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
  TEST( "Value of step 1", collect->values_[0], 0 );    \
  TEST( "Value of step 2", collect->values_[1], 3 );    \
  TEST( "Value of step 3", collect->values_[2], 8 );    \
  TEST( "Value of step 4", collect->values_[3], 15 );   \
  TEST( "Value of step 5", collect->values_[4], 24 );   \
  TEST( "Value of step 6", collect->values_[5], 35 );   \
  TEST( "Value of step 7", collect->values_[6], 48 );


template< class P >
void
test_one_source()
{
  P p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", 7 ) );
  process_smart_pointer< add > sum( new add( "sum" ) );
  process_smart_pointer< multiply > product( new multiply( "product" ) );
  process_smart_pointer< add > total( new add( "total" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( sum );
  p.add( product );
  p.add( total );
  p.add( collect );

  p.connect( nums->value_port(),
             sum->set_value1_port() );
  p.connect( nums->value_port(),
             sum->set_value2_port() );
  p.connect( nums->value_port(),
             product->set_value1_port() );
  p.connect( nums->value_port(),
             product->set_value2_port() );
  p.connect( sum->sum_port(),
             total->set_value1_port() );
  p.connect( product->product_port(),
             total->set_value2_port() );
  p.connect( total->sum_port(),
             collect->set_value_port() );

  RUN_TEST();
}


template< class P >
void
test_two_sources()
{
  P p;
  process_smart_pointer< numbers > nums1( new numbers( "numbers1", 7 ) );
  process_smart_pointer< numbers > nums2( new numbers( "numbers2", 8 ) );
  process_smart_pointer< add > sum( new add( "sum" ) );
  process_smart_pointer< multiply > product( new multiply( "product" ) );
  process_smart_pointer< add > total( new add( "total" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums1 );
  p.add( nums2 );
  p.add( sum );
  p.add( product );
  p.add( total );
  p.add( collect );

  p.connect( nums1->value_port(),
             sum->set_value1_port() );
  p.connect( nums1->value_port(),
             sum->set_value2_port() );
  p.connect( nums2->value_port(),
             product->set_value1_port() );
  p.connect( nums2->value_port(),
             product->set_value2_port() );
  p.connect( sum->sum_port(),
             total->set_value1_port() );
  p.connect( product->product_port(),
             total->set_value2_port() );
  p.connect( total->sum_port(),
             collect->set_value_port() );

  RUN_TEST();
}


template< class P >
void
test_two_crossed_sources()
{
  P p;
  process_smart_pointer< numbers > nums1( new numbers( "numbers1", 7 ) );
  process_smart_pointer< numbers > nums2( new numbers( "numbers2", 8 ) );
  process_smart_pointer< add > sum( new add( "sum" ) );
  process_smart_pointer< multiply > product( new multiply( "product" ) );
  process_smart_pointer< add > total( new add( "total" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums1 );
  p.add( nums2 );
  p.add( sum );
  p.add( product );
  p.add( total );
  p.add( collect );

  p.connect( nums1->value_port(),
             sum->set_value1_port() );
  p.connect( nums2->value_port(),
             sum->set_value2_port() );
  p.connect( nums1->value_port(),
             product->set_value1_port() );
  p.connect( nums2->value_port(),
             product->set_value2_port() );
  p.connect( sum->sum_port(),
             total->set_value1_port() );
  p.connect( product->product_port(),
             total->set_value2_port() );
  p.connect( total->sum_port(),
             collect->set_value_port() );

  RUN_TEST();
}


#undef RUN_TEST


} // end anonymous namespace

int test_simple_pipelines( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_simple_pipelines" );

  vcl_cout << "--------------------------------\n"
           << "Testing one source sync_pipeline\n"
           << "--------------------------------" << vcl_endl;
  test_one_source< sync_pipeline >();

  vcl_cout << "--------------------------------\n"
           << "Testing two source sync_pipeline\n"
           << "--------------------------------" << vcl_endl;
  test_two_sources< sync_pipeline >();

  vcl_cout << "----------------------------------------\n"
           << "Testing two source crossed sync_pipeline\n"
           << "----------------------------------------" << vcl_endl;
  test_two_crossed_sources< sync_pipeline >();


  vcl_cout << "---------------------------------\n"
           << "Testing one source async_pipeline\n"
           << "---------------------------------" << vcl_endl;
  test_one_source< async_pipeline >();

  vcl_cout << "---------------------------------\n"
           << "Testing two source async_pipeline\n"
           << "---------------------------------" << vcl_endl;
  test_two_sources< async_pipeline >();

  vcl_cout << "-----------------------------------------\n"
           << "Testing two source crossed async_pipeline\n"
           << "-----------------------------------------" << vcl_endl;
  test_two_crossed_sources< async_pipeline >();

  return testlib_test_summary();
}
