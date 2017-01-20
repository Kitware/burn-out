/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
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

#define CHECK_STEP(step, value) \
  TEST_EQUAL( "Value of step " #step, collect->values_[step - 1], value )


#define RUN_TEST()                                            \
  TEST( "Pipeline initialize", p.initialize(), true );        \
  bool run_status = p.run();                                  \
  TEST( "Run", run_status, true);                             \
  TEST_EQUAL( "Number of steps", collect->values_.size(), 16);\
  CHECK_STEP( 1, 1);                                          \
  CHECK_STEP( 2, 2);                                          \
  CHECK_STEP( 3, 3);                                          \
  CHECK_STEP( 4, 4);                                          \
  CHECK_STEP( 5, 5);                                          \
  CHECK_STEP( 6, 6);                                          \
  CHECK_STEP( 7, 7);                                          \
  CHECK_STEP( 8, 8);                                          \
  CHECK_STEP( 9, 9);                                          \
  CHECK_STEP(10,10);                                          \
  CHECK_STEP(11,11);                                          \
  CHECK_STEP(12,12);                                          \
  CHECK_STEP(13,13);                                          \
  CHECK_STEP(14,14);                                          \
  CHECK_STEP(15,15);                                          \
  CHECK_STEP(16,16);

#ifdef VIDTK_SKIP_IS_PROPOGATED_OVER_OPTIONAL_CONNECTIONS
#define RUN_TEST_OPTIONAL() RUN_TEST()
#else
#define RUN_TEST_OPTIONAL()                                   \
  TEST( "Pipeline initialize", p.initialize(), true );        \
  bool run_status = p.run();                                  \
  TEST( "Run", run_status, true);                             \
  TEST_EQUAL( "Number of steps", collect->values_.size(), 16);\
  CHECK_STEP( 1, 1);                                          \
  CHECK_STEP( 2, 1);                                          \
  CHECK_STEP( 3, 3);                                          \
  CHECK_STEP( 4, 3);                                          \
  CHECK_STEP( 5, 5);                                          \
  CHECK_STEP( 6, 5);                                          \
  CHECK_STEP( 7, 7);                                          \
  CHECK_STEP( 8, 7);                                          \
  CHECK_STEP( 9, 9);                                          \
  CHECK_STEP(10, 9);                                          \
  CHECK_STEP(11,11);                                          \
  CHECK_STEP(12,11);                                          \
  CHECK_STEP(13,13);                                          \
  CHECK_STEP(14,13);                                          \
  CHECK_STEP(15,15);                                          \
  CHECK_STEP(16,15);
#endif


template< class P >
void
test_skip_adder()
{
  P p;
  process_smart_pointer< numbers_pred > nums( new numbers_pred( "numbers", is_odd, 8, 1 ) );
  process_smart_pointer< skip_adder > add( new skip_adder( "add" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( add );
  p.add( collect );

  p.connect( nums->value_port(),
             add->set_value_port() );
  p.connect( add->value_port(),
             collect->set_value_port() );

  RUN_TEST();
}

template< class P >
void
test_skip_adder_optional()
{
  P p;
  process_smart_pointer< const_value > nums1( new const_value( "numbers1", 0, 16 ) );
  process_smart_pointer< numbers_pred > nums2( new numbers_pred( "numbers2", is_odd, 8, 1 ) );
  process_smart_pointer< add > sum( new add( "sum" ) );
  process_smart_pointer< skip_adder > add( new skip_adder( "add" ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums1 );
  p.add( nums2 );
  p.add( sum );
  p.add( add );
  p.add( collect );

  p.connect( nums1->value_port(),
             sum->set_value1_port() );
  p.connect_optional( nums2->value_port(),
                      sum->set_value2_port() );
  p.connect( sum->sum_port(),
             add->set_value_port() );
  p.connect( add->value_port(),
             collect->set_value_port() );

  RUN_TEST_OPTIONAL();
}


#undef RUN_TEST_OPTIONAL
#undef RUN_TEST


} // end anonymous namespace

int test_skip_detection_pipeline( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_skip_detection_pipeline" );

  std::cout << "----------------------------------\n"
           << "Testing a skip adder sync_pipeline\n"
           << "----------------------------------" << std::endl;
  test_skip_adder< sync_pipeline >();


  std::cout << "-----------------------------------\n"
           << "Testing a skip adder async_pipeline\n"
           << "-----------------------------------" << std::endl;
  test_skip_adder< async_pipeline >();

  std::cout << "---------------------------------------------\n"
           << "Testing a skip adder (optional) sync_pipeline\n"
           << "---------------------------------------------" << std::endl;
  test_skip_adder_optional< sync_pipeline >();


  std::cout << "----------------------------------------------\n"
           << "Testing a skip adder (optional) async_pipeline\n"
           << "----------------------------------------------" << std::endl;
  test_skip_adder_optional< async_pipeline >();

  return testlib_test_summary();
}
