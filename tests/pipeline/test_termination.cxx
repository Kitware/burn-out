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



template< class P >
void
test_infinite_source()
{
  P p;
  process_smart_pointer< const_value > const_val( new const_value( "const", 1 ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect", 20) );

  p.add( const_val );
  p.add( collect );

  p.connect( const_val->value_port(),
             collect->set_value_port() );

  TEST( "Pipeline initialize", p.initialize(), true );
  bool run_status = p.run();
  TEST( "Run", run_status, true);
  TEST( "Number of steps", collect->values_.size(), 20 );
}



} // end anonymous namespace

int test_termination( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_termination" );

  vcl_cout << "-------------------------------------\n"
           << "Testing infinite source sync_pipeline\n"
           << "-------------------------------------" << vcl_endl;
  test_infinite_source< sync_pipeline >();

  vcl_cout << "--------------------------------------\n"
           << "Testing infinite source async_pipeline\n"
           << "--------------------------------------" << vcl_endl;
  test_infinite_source< async_pipeline >();

  return testlib_test_summary();
}
