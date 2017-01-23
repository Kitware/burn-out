/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sample_nodes.h"

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/pipeline_queue_monitor.h>

#include <testlib/testlib_test.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>
namespace
{

using namespace vidtk;

void run_test()
{
  async_pipeline p;
  pipeline_queue_monitor qm;

  process_smart_pointer< special_output > source(
    new special_output( "source", process::FLUSH, 7, 1, true ) );
  process_smart_pointer< infinite_looper > sink(
    new infinite_looper( "looper" ) );

  p.add( source );
  p.add( sink );

  p.connect( source->value_port(),
    sink->set_value_port() );

  p.initialize();

  p.run_async();

  TEST( "Monitor node initialize", qm.monitor_node( "source", &p ), true );

  while( qm.current_max_queue_length() < 6 );

  TEST( "Initial queue size reached", qm.current_max_queue_length() >= 6, true );

  source->send_status_signals();

  while( qm.current_max_queue_length() > 2 );

  TEST( "All edges flushed", qm.current_max_queue_length() <= 2, true );

  sink->exit_loop();
}

} // end anonymous namespace

int test_flush_signal( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_flush_signal" );

  run_test();

  return testlib_test_summary();
}
