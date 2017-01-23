/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sample_nodes.h"

#include <pipeline_framework/sync_pipeline.h>
#include <pipeline_framework/async_pipeline.h>

#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;



#ifdef VIDTK_SKIP_IS_PROPOGATED_OVER_OPTIONAL_CONNECTIONS
#define RUN_TEST()                                      \
  TEST( "Pipeline initialize", p.initialize(), true );  \
  bool run_status = p.run();                            \
  TEST( "Run", run_status, true);                       \
  TEST( "Number of steps", collect->values_.size(), 4 );\
  TEST( "Value of step 1", collect->values_[0], 1 );    \
  TEST( "Value of step 2", collect->values_[1], 11 );   \
  TEST( "Value of step 3", collect->values_[2], 29 );   \
  TEST( "Value of step 4", collect->values_[3], 55 );
#else
#define RUN_TEST()                                      \
  TEST( "Pipeline initialize", p.initialize(), true );  \
  bool run_status = p.run();                            \
  TEST( "Run", run_status, true);                       \
  TEST( "Number of steps", collect->values_.size(), 7 );\
  TEST( "Value of step 1", collect->values_[0], 1 );    \
  TEST( "Value of step 2", collect->values_[1], 3 );    \
  TEST( "Value of step 3", collect->values_[2], 11 );   \
  TEST( "Value of step 4", collect->values_[3], 15 );   \
  TEST( "Value of step 5", collect->values_[4], 29 );   \
  TEST( "Value of step 6", collect->values_[5], 35 );   \
  TEST( "Value of step 7", collect->values_[6], 55 );
#endif


template< class P >
void
test_two_crossed_sources()
{
  P p;
  process_smart_pointer< numbers > nums1( new numbers( "numbers1", 7 ) );
  process_smart_pointer< numbers_pred > nums2( new numbers_pred( "numbers2", is_odd, 8, 1 ) );
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
  p.connect_optional( nums2->value_port(),
                      sum->set_value2_port() );
  p.connect( nums1->value_port(),
             product->set_value1_port() );
  p.connect_optional( nums2->value_port(),
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

int test_skip_optional_pipeline( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_skip_optional_pipeline" );

  std::cout << "----------------------------------------\n"
            << "Testing two source crossed sync_pipeline\n"
            << "----------------------------------------" << std::endl;
  test_two_crossed_sources< sync_pipeline >();


  std::cout << "-----------------------------------------\n"
            << "Testing two source crossed async_pipeline\n"
            << "-----------------------------------------" << std::endl;
  test_two_crossed_sources< async_pipeline >();

  return testlib_test_summary();
}
