/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sample_nodes.h"

#include <pipeline_framework/async_pipeline.h>

#include <testlib/testlib_test.h>

namespace
{

using namespace vidtk;

void run_test()
{
  async_pipeline p;

  process_smart_pointer< special_output > source(
    new special_output( "no_output_sender", process::NO_OUTPUT, 7, 13 ) );
  process_smart_pointer< step_counter > sink(
    new step_counter( "counter" ) );

  p.add( source );
  p.add( sink );

  p.connect( source->value_port(),
    sink->set_value_port() );

  p.initialize();
  bool run_status = p.run();

  TEST( "Run status", run_status, true );
  TEST( "Correct step count", sink->get_count(), 7 );
}

} // end anonymous namespace

int test_no_output_signal( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_no_output_signal" );

  run_test();

  return testlib_test_summary();
}
