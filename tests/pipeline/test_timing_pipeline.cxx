/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "sample_nodes.h"

#include <vcl_cstdlib.h>
#include <pipeline/sync_pipeline.h>
#include <pipeline/async_pipeline.h>
#include <testlib/testlib_test.h>
#include <boost/thread/thread.hpp>

#include <vul/vul_timer.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


#define NUM_BUSY_LOOPS 1000000
#define NUM_PIPELINE_LOOPS 100


template< class P >
long
test_parallel_processes( unsigned num_procs )
{
  P p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", NUM_PIPELINE_LOOPS ) );
  vcl_vector< process_smart_pointer< busy_loop > > busy_loops;
  vcl_vector< process_smart_pointer< collect_value > > collectors;

  p.add( nums );

  for( unsigned i = 0; i < num_procs; ++i )
  {
    process_smart_pointer< busy_loop > busy( new busy_loop( vcl_string("busy_loop_") + char('A' + i), NUM_BUSY_LOOPS ) );
    process_smart_pointer< collect_value > collect( new collect_value( vcl_string("collect_") + char('A' + i) ) );

    p.add( busy );
    p.add( collect );

    p.connect( nums->value_port(),
               busy->set_value_port() );
    p.connect( busy->value_port(),
               collect->set_value_port() );

    busy_loops.push_back( busy );
    collectors.push_back( collect );
  }

  TEST( "Pipeline initialize", p.initialize(), true );
  vul_timer timer;
  bool run_status = p.run();
  long time = timer.real();
  TEST( "Run", run_status, true);
  for( unsigned i = 0; i < num_procs; ++i )
  {
    TEST( "Number of steps", collectors[i]->values_.size(), NUM_PIPELINE_LOOPS );
  }
  return time;
}


template< class P1, class P2 >
long
test_nested_sequential_processes()
{
  P1 p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", NUM_PIPELINE_LOOPS) );
  process_smart_pointer< busy_sequence<P2> > busy_seq( new busy_sequence<P2>( "busy_seq", NUM_BUSY_LOOPS, NUM_BUSY_LOOPS ) );
  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( nums );
  p.add( busy_seq );
  p.add( collect );

  p.connect( nums->value_port(),
             busy_seq->set_value_port() );
  p.connect( busy_seq->value_port(),
             collect->set_value_port() );

  TEST( "Pipeline initialize", p.initialize(), true );
  vul_timer timer;
  bool run_status = p.run();
  long time = timer.real();
  TEST( "Run", run_status, true);
  TEST( "Number of steps", collect->values_.size(), NUM_PIPELINE_LOOPS );
  return time;
}


template< class P >
long
test_sequential_processes( unsigned num_procs )
{
  P p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", NUM_PIPELINE_LOOPS) );
  vcl_vector< process_smart_pointer< busy_loop > > busy_loops;

  p.add( nums );

  process_smart_pointer< busy_loop > busy( new busy_loop( "busy_loop_A", NUM_BUSY_LOOPS ) );

  p.add( busy );

  p.connect( nums->value_port(),
             busy->set_value_port() );

  busy_loops.push_back( busy );

  for( unsigned i = 1; i < num_procs; ++i )
  {
    busy = new busy_loop( vcl_string("busy_loop_") + char('A' + i), NUM_BUSY_LOOPS  );

    p.add( busy );

    p.connect( (*busy_loops.rbegin())->value_port(),
               busy->set_value_port() );

    busy_loops.push_back( busy );
  }

  process_smart_pointer< collect_value > collect( new collect_value( "collect" ) );

  p.add( collect );

  p.connect( (*busy_loops.rbegin())->value_port(),
             collect->set_value_port() );

  TEST( "Pipeline initialize", p.initialize(), true );
  vul_timer timer;
  bool run_status = p.run();
  long time = timer.real();
  TEST( "Run", run_status, true);
  TEST( "Number of steps", collect->values_.size(), NUM_PIPELINE_LOOPS );
  return time;
}


template< class P >
long
test_parallel_nonuniform_processes( unsigned num_procs )
{
  P p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", NUM_PIPELINE_LOOPS ) );
  vcl_vector< process_smart_pointer< busy_loop > > busy_loops;
  vcl_vector< process_smart_pointer< collect_value > > collectors;

  p.add( nums );

  for( unsigned i = 0; i < num_procs; ++i )
  {
    unsigned loop_multiple = (i==1) + 1;
    process_smart_pointer< busy_loop > busy( new busy_loop( vcl_string("busy_loop_") + char('A' + i),  loop_multiple * NUM_BUSY_LOOPS  ) );
    process_smart_pointer< collect_value > collect( new collect_value( vcl_string("collect_") + char('A' + i) ) );

    p.add( busy );
    p.add( collect );

    p.connect( nums->value_port(),
               busy->set_value_port() );
    p.connect( busy->value_port(),
               collect->set_value_port() );

    busy_loops.push_back( busy );
    collectors.push_back( collect );
  }

  TEST( "Pipeline initialize", p.initialize(), true );
  vul_timer timer;
  bool run_status = p.run();
  long time = timer.real();
  TEST( "Run", run_status, true);
  for( unsigned i = 0; i < num_procs; ++i )
  {
    TEST( "Number of steps", collectors[i]->values_.size(), NUM_PIPELINE_LOOPS );
  }
  return time;
}


template< class P >
long
test_parallel_nonuniform_processes_sink()
{
  P p;
  process_smart_pointer< numbers > nums( new numbers( "numbers", NUM_PIPELINE_LOOPS ) );
  vcl_vector< process_smart_pointer< busy_loop > > busy_loops;
  process_smart_pointer< collect_value_community > collect( new collect_value_community( "collect" ) );

  p.add( nums );

  for( unsigned i = 0; i < 2; ++i )
  {
    unsigned loop_multiple = (i==1) + 1;
    process_smart_pointer< busy_loop > busy( new busy_loop( vcl_string("busy_loop_") + char('A' + i), loop_multiple * NUM_BUSY_LOOPS ) );

    p.add( busy );

    p.connect( nums->value_port(),
               busy->set_value_port() );

    busy_loops.push_back( busy );
  }

  p.add( collect );
  p.connect( busy_loops[0]->value_port(),
             collect->set_value_port() );
  p.connect( busy_loops[1]->value_port(),
             collect->set_value_dummy1_port() );

  TEST( "Pipeline initialize", p.initialize(), true );
  vul_timer timer;
  bool run_status = p.run();
  long time = timer.real();
  TEST( "Run", run_status, true);
  TEST( "Number of steps", collect->values_.size(), NUM_PIPELINE_LOOPS );
  return time;
}


void busy_job()
{
  volatile unsigned v=0;
  for(unsigned int i=0; i<NUM_BUSY_LOOPS*10; ++i)
  {
    double v2 = static_cast<double>(i);
    v += i + static_cast<unsigned>(vcl_sqrt(v2*v2));
  }
}

// using timing to find the number of processes that actually run in parallel
// this may differ from boost::thread::hardware_concurrency due to hyper-threading
unsigned real_hardware_concurrency()
{
  vul_timer t;
  busy_job();
  double serial_time = t.real();

  unsigned num_procs = boost::thread::hardware_concurrency();
  boost::thread_group threads;
  t.mark();
  for( unsigned i=0; i<num_procs; ++i)
  {
    threads.create_thread(busy_job);
  }
  threads.join_all();
  double parallel_time = t.real();
  double factor = parallel_time / serial_time;
  vcl_cout << "  serial time   "<<serial_time << vcl_endl;
  vcl_cout << "  parallel time "<<parallel_time << vcl_endl;
  // if the parallel tasks take about twice as long to run then it is likely that
  // hyperthreading is enabled and we should scale back the number of threads
  // by a factor of two.
  if( factor > 1.5 )
  {
    num_procs /= 2;
  }
  return num_procs;
}


} // end anonymous namespace

int test_timing_pipeline( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_timing_pipeline" );

  unsigned num_procs = boost::thread::hardware_concurrency();
  vcl_cout << "detected "<< num_procs << " logical CPUs" << vcl_endl;
  if ( num_procs < 2 )
  {
    vcl_cerr << "This test requires at least 2 CPU cores" << vcl_endl;
    vcl_abort();
  }
  num_procs = real_hardware_concurrency();
  vcl_cout << "using "<< num_procs << " concurrent threads" << vcl_endl;

  long sync_time, async_time;

  vcl_cout << "--------------------------------\n"
           << "Testing sync_pipeline (parallel)\n"
           << "--------------------------------" << vcl_endl;
  sync_time = test_parallel_processes< sync_pipeline >( num_procs );

  vcl_cout << "---------------------------------\n"
           << "Testing async_pipeline (parallel)\n"
           << "---------------------------------" << vcl_endl;
  async_time = test_parallel_processes< async_pipeline >( num_procs );

  TEST_NEAR_REL( "Relative timing", sync_time / num_procs, async_time, .25 );


  vcl_cout << "----------------------------------\n"
           << "Testing sync_pipeline (sequential)\n"
           << "----------------------------------" << vcl_endl;
  sync_time = test_sequential_processes< sync_pipeline >( num_procs );

  vcl_cout << "-----------------------------------\n"
           << "Testing async_pipeline (sequential)\n"
           << "-----------------------------------" << vcl_endl;
  async_time = test_sequential_processes< async_pipeline >( num_procs );

  TEST_NEAR_REL( "Relative timing", sync_time / num_procs, async_time, .25 );


  vcl_cout << "----------------------------------------------------------\n"
           << "Testing sync_pipeline nested in sync_pipeline (sequential)\n"
           << "----------------------------------------------------------" << vcl_endl;
  sync_time = test_nested_sequential_processes< sync_pipeline, sync_pipeline >();

  vcl_cout << "------------------------------------------------------------\n"
           << "Testing async_pipeline nested in async_pipeline (sequential)\n"
           << "------------------------------------------------------------" << vcl_endl;
  async_time = test_nested_sequential_processes< async_pipeline, async_pipeline >();

  TEST_NEAR_REL( "Relative timing", sync_time / 2, async_time, .25 );


  vcl_cout << "----------------------------------\n"
            << "Testing sync_pipeline (nonuniform)\n"
            << "----------------------------------" << vcl_endl;
  sync_time = test_parallel_nonuniform_processes< sync_pipeline >( num_procs );

  vcl_cout << "-----------------------------------\n"
            << "Testing async_pipeline (nonuniform)\n"
            << "-----------------------------------" << vcl_endl;
  async_time = test_parallel_nonuniform_processes< async_pipeline >( num_procs );

  TEST_NEAR_REL( "Relative timing", 2 * sync_time / (num_procs + 1), async_time, .25 );


  vcl_cout << "---------------------------------------\n"
            << "Testing sync_pipeline (nonuniform-sink)\n"
            << "---------------------------------------" << vcl_endl;
  sync_time = test_parallel_nonuniform_processes_sink< sync_pipeline >();

  vcl_cout << "----------------------------------------\n"
            << "Testing async_pipeline (nonuniform-sink)\n"
            << "----------------------------------------" << vcl_endl;
  async_time = test_parallel_nonuniform_processes_sink< async_pipeline >();

  TEST_NEAR_REL( "Relative timing", 2 * sync_time / 3, async_time, .25 );



  return testlib_test_summary();
}
