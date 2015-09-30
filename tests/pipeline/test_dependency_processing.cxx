/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <testlib/testlib_test.h>

#include <process_framework/function_caller_2_process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <pipeline/sync_pipeline.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


unsigned g_val1;
unsigned g_val2;
unsigned g_val3;
unsigned g_source1;
unsigned g_sink1;
unsigned g_source2;
unsigned g_sink2;

void reset_globals()
{
  g_val1 = 0;
  g_val2 = 0;
  g_val3 = 0;
  g_source1 = 0;
  g_sink1 = 0;
  g_source2 = 0;
  g_sink2 = 0;
}


process::step_status exec1()
{
  ++g_val1;
  return g_val1 < 3 ? process::SUCCESS : process::FAILURE;
}

process::step_status exec2()
{
  ++g_val2;
  return g_val2 < 5 ? process::SUCCESS : process::FAILURE;
}

process::step_status exec3()
{
  ++g_val3;
  return g_val3 < 7 ? process::SUCCESS : process::FAILURE;
}

unsigned source1()
{
  ++g_source1;
  return g_source1;
}

void sink1( unsigned val )
{
  ++g_sink1;
  if( g_sink1 != val )
  {
    vcl_cerr << "sink1: expected " << g_sink1
             << " got " << val << "\n";
  }
}


unsigned source2()
{
  ++g_source2;
  return g_source2;
}

void sink2( unsigned val )
{
  ++g_sink2;
  if( g_sink2 != val )
  {
    vcl_cerr << "sink2: expected " << g_sink2
             << " got " << val << "\n";
  }
}


void
test_required_dependent_processing()
{
  vcl_cout << "\n\nRequired dependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  sync_pipeline_node n1;
  n1.set_name( "n1" );
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  sync_pipeline_node n2;
  n2.set_execute_func( &exec2 );
  p.add( n2 );
  p.connect( n1.id(), boost::bind(&source1), n2.id(), boost::bind(&sink1,_1) );

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Step 1", p.execute(), process::SUCCESS );
  TEST( "Step 2", p.execute(), process::SUCCESS );
  TEST( "Step 3 (step should fail)", p.execute(), process::FAILURE );
  TEST( "Data flowed properly", g_sink1, g_source1 );
  TEST( "Data flow ran 2 times", g_sink1, 2 );
  TEST( "Node 1 ran 3 times", g_val1, 3 );
  TEST( "Node 2 ran 5 times", g_val2, 2 );
}


void
test_independent_processing()
{
  vcl_cout << "\n\nIndependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  sync_pipeline_node n1;
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  sync_pipeline_node n2;
  n2.set_execute_func( &exec2 );
  p.add( n2 );

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Step 1", p.execute(), process::SUCCESS );
  TEST( "Step 2", p.execute(), process::SUCCESS );
  TEST( "Step 3", p.execute(), process::SUCCESS );
  TEST( "Step 4", p.execute(), process::SUCCESS );
  TEST( "Step 5 (step should fail)", p.execute(), process::FAILURE );
  TEST( "Data flowed properly", g_sink1, g_source1 );
  TEST( "Data flow ran 0 times", g_sink1, 0 );
  TEST( "Node 1 ran 3 times", g_val1, 3 );
  TEST( "Node 2 ran 5 times", g_val2, 5 );
}


void
test_optional_dependent_processing()
{
  vcl_cout << "\n\nOptional dependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  sync_pipeline_node n1;
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  sync_pipeline_node n2;
  n2.set_execute_func( &exec2 );
  p.add( n2 );
  p.connect_optional( n1.id(), boost::bind(&source1), n2.id(), boost::bind(&sink1,_1) );

  sync_pipeline_node n3;
  n3.set_execute_func( &exec3 );
  p.add( n3 );
  p.connect( n2.id(), boost::bind(&source2), n3.id(), boost::bind(&sink2,_1) );

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Step 1", p.execute(), process::SUCCESS );
  TEST( "Step 2", p.execute(), process::SUCCESS );
  TEST( "Step 3", p.execute(), process::SUCCESS );
  TEST( "Step 4", p.execute(), process::SUCCESS );
  TEST( "Step 5 (step should fail)", p.execute(), process::FAILURE );
  TEST( "Data 1 flowed properly", g_sink1, g_source1 );
  TEST( "Data 1 flow ran 2 times", g_sink1, 2 );
  TEST( "Data 2 flowed properly", g_sink2, g_source2 );
  TEST( "Data 2 flow ran 4 times", g_sink2, 4 );
  TEST( "Node 1 ran 3 times", g_val1, 3 );
  TEST( "Node 2 ran 5 times", g_val2, 5 );
  TEST( "Node 3 ran 4 times", g_val3, 4 );
}


void
test_mixed_optional_and_required_dependent_processing()
{
  vcl_cout << "\n\nMixed optional and required dependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  sync_pipeline_node n1;
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  sync_pipeline_node n2;
  n2.set_execute_func( &exec2 );
  p.add( n2 );

  sync_pipeline_node n3;
  n3.set_execute_func( &exec3 );
  p.add( n3 );
  p.connect_optional( n1.id(), boost::bind(&source1), n3.id(), boost::bind(&sink1,_1) );
  p.connect( n2.id(), boost::bind(&source2), n3.id(), boost::bind(&sink2,_1) );

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Step 1", p.execute(), process::SUCCESS );
  TEST( "Step 2", p.execute(), process::SUCCESS );
  TEST( "Step 3", p.execute(), process::SUCCESS );
  TEST( "Step 4", p.execute(), process::SUCCESS );
  TEST( "Step 5 (step should fail)", p.execute(), process::FAILURE );
  TEST( "Data1 flowed properly", g_sink1, g_source1 );
  TEST( "Data1 flow ran 2 times", g_sink1, 2 );
  vcl_cout << "flow 1 = " << g_sink1 << "\n";
  TEST( "Data2 flowed properly", g_sink2, g_source2 );
  TEST( "Data2 flow ran 4 times", g_sink2, 4 );
  vcl_cout << "flow 2 = " << g_sink2 << "\n";
  TEST( "Node 1 ran 3 times", g_val1, 3 );
  TEST( "Node 2 ran 5 times", g_val2, 5 );
  TEST( "Node 3 ran 4 times", g_val3, 4 );
}


void
test_dependents_tested_once_processing()
{
  vcl_cout << "\n\nDependents tested once\n\n";

  // Test that a dependent node is never executed even though a
  // sibling optional node is.

  reset_globals();

  sync_pipeline p;

  sync_pipeline_node n1;
  n1.set_name( "n1" );
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  sync_pipeline_node n2;
  n2.set_execute_func( &exec2 );
  n2.set_name( "n2" );
  p.add( n2 );
  p.connect_optional( n1.id(), boost::bind(&source1), n2.id(), boost::bind(&sink1,_1) );

  sync_pipeline_node n3;
  n3.set_name( "n3" );
  n3.set_execute_func( &exec3 );
  p.add( n3 );
  p.connect( n1.id(), boost::bind(&source2), n3.id(), boost::bind(&sink2,_1) );

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Step 1", p.execute(), process::SUCCESS );
  TEST( "Step 2", p.execute(), process::SUCCESS );
  TEST( "Step 3", p.execute(), process::SUCCESS );
  TEST( "Step 4", p.execute(), process::SUCCESS );
  TEST( "Step 5 (step should fail)", p.execute(), process::FAILURE );
  TEST( "Data1 flowed properly", g_sink1, g_source1 );
  TEST( "Data1 flow ran 2 times", g_sink1, 2 );
  vcl_cout << "flow 1 = " << g_sink1 << "\n";
  TEST( "Data2 flowed properly", g_sink2, g_source2 );
  TEST( "Data2 flow ran 2 times", g_sink2, 2 );
  vcl_cout << "flow 2 = " << g_sink2 << "\n";
  TEST( "Node 1 ran 3 times", g_val1, 3 );
  TEST( "Node 2 ran 5 times", g_val2, 5 );
  TEST( "Node 3 ran 2 times", g_val3, 2 );
}




} // end anonymous namespace

int test_dependency_processing( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_dependency_processing" );

  test_required_dependent_processing();
  test_independent_processing();
  test_optional_dependent_processing();
  test_mixed_optional_and_required_dependent_processing();
  test_dependents_tested_once_processing();

  return testlib_test_summary();
}
