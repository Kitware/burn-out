/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <vil/vil_image_view.h>
#include <testlib/testlib_test.h>

#include <process_framework/function_caller_2_process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>
#include <pipeline_framework/sync_pipeline.h>


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

/// This is an empty, instantiable process for testing purposes, i.e. to be able
/// to instiaitiate a pipeline node for a pipeline, etc.
class empty_test_process
: public process
{
public:
  typedef empty_test_process self_type;

  empty_test_process( std::string const& _name )
    : process( _name, "empty_test_process")
  {
  }

  config_block params() const
  {
    return config_block();
  }

  virtual bool set_params( config_block const& )
  {
    return true;
  }

  virtual bool initialize()
  {
    return true;
  }

  virtual bool step()
  {
    return true;
  }

  void set_sink1(unsigned s1)
  {
    ++g_sink1;
    if ( g_sink1 != s1 )
    {
      std::cerr << "sink1: expected " << g_sink1
               << " got " << s1 << "\n";
    }
  }
  VIDTK_INPUT_PORT( set_sink1, unsigned );

  void set_sink2(unsigned s2)
  {
    ++g_sink2;
    if ( g_sink2 != s2 )
    {
      std::cerr << "sink2: expected " << g_sink2
               << " got " << s2 << "\n";
    }
  }
  VIDTK_INPUT_PORT( set_sink2, unsigned );

  unsigned source1() const
  {
    return ++g_source1;
  }
  VIDTK_OUTPUT_PORT( unsigned, source1 );

  unsigned source2() const
  {
    return ++g_source2;
  }
  VIDTK_OUTPUT_PORT( unsigned, source2 );

protected:
  config_block config_;

};


void
test_required_dependent_processing()
{
  std::cout << "\n\nRequired dependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  process_smart_pointer< empty_test_process > proc1( new empty_test_process("proc1") );
  sync_pipeline_node n1(proc1);
  n1.set_name( "n1" );
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  process_smart_pointer< empty_test_process > proc2( new empty_test_process("proc2") );
  sync_pipeline_node n2(proc2);
  n2.set_execute_func( &exec2 );
  p.add( n2 );
  p.connect( proc1->source1_port(), proc2->set_sink1_port() );

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
  std::cout << "\n\nIndependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  process_smart_pointer< empty_test_process > proc1( new empty_test_process("proc1") );
  sync_pipeline_node n1(proc1);
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  process_smart_pointer< empty_test_process > proc2( new empty_test_process("proc2") );
  sync_pipeline_node n2(proc2);
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
  std::cout << "\n\nOptional dependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  process_smart_pointer< empty_test_process > proc1( new empty_test_process("proc1") );
  sync_pipeline_node n1(proc1);
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  process_smart_pointer< empty_test_process > proc2( new empty_test_process("proc2") );
  sync_pipeline_node n2(proc2);
  n2.set_execute_func( &exec2 );
  p.add( n2 );
  p.connect_optional( proc1->source1_port(), proc2->set_sink1_port() );

  process_smart_pointer< empty_test_process > proc3( new empty_test_process("proc3") );
  sync_pipeline_node n3(proc3);
  n3.set_execute_func( &exec3 );
  p.add( n3 );
  p.connect( proc2->source2_port(), proc3->set_sink2_port() );

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
  std::cout << "\n\nMixed optional and required dependent processing\n\n";

  reset_globals();

  sync_pipeline p;

  process_smart_pointer< empty_test_process > proc1( new empty_test_process("proc1") );
  sync_pipeline_node n1(proc1);
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  process_smart_pointer< empty_test_process > proc2( new empty_test_process("proc2") );
  sync_pipeline_node n2(proc2);
  n2.set_execute_func( &exec2 );
  p.add( n2 );

  process_smart_pointer< empty_test_process > proc3( new empty_test_process("proc3") );
  sync_pipeline_node n3(proc3);
  n3.set_execute_func( &exec3 );
  p.add( n3 );
  p.connect_optional( proc1->source1_port(), proc3->set_sink1_port() );
  p.connect( proc2->source2_port(), proc3->set_sink2_port() );

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Step 1", p.execute(), process::SUCCESS );
  TEST( "Step 2", p.execute(), process::SUCCESS );
  TEST( "Step 3", p.execute(), process::SUCCESS );
  TEST( "Step 4", p.execute(), process::SUCCESS );
  TEST( "Step 5 (step should fail)", p.execute(), process::FAILURE );
  TEST( "Data1 flowed properly", g_sink1, g_source1 );
  TEST( "Data1 flow ran 2 times", g_sink1, 2 );
  std::cout << "flow 1 = " << g_sink1 << "\n";
  TEST( "Data2 flowed properly", g_sink2, g_source2 );
  TEST( "Data2 flow ran 4 times", g_sink2, 4 );
  std::cout << "flow 2 = " << g_sink2 << "\n";
  TEST( "Node 1 ran 3 times", g_val1, 3 );
  TEST( "Node 2 ran 5 times", g_val2, 5 );
  TEST( "Node 3 ran 4 times", g_val3, 4 );
}


void
test_dependents_tested_once_processing()
{
  std::cout << "\n\nDependents tested once\n\n";

  // Test that a dependent node is never executed even though a
  // sibling optional node is.

  reset_globals();

  sync_pipeline p;

  process_smart_pointer< empty_test_process > proc1( new empty_test_process("proc1") );
  sync_pipeline_node n1(proc1);
  n1.set_name( "n1" );
  n1.set_execute_func( &exec1 );
  p.add( n1 );

  process_smart_pointer< empty_test_process > proc2( new empty_test_process("proc2") );
  sync_pipeline_node n2(proc2);
  n2.set_execute_func( &exec2 );
  n2.set_name( "n2" );
  p.add( n2 );
  p.connect_optional( proc1->source1_port(), proc2->set_sink1_port() );

  process_smart_pointer< empty_test_process > proc3( new empty_test_process("proc3") );
  sync_pipeline_node n3(proc3);
  n3.set_name( "n3" );
  n3.set_execute_func( &exec3 );
  p.add( n3 );
  p.connect( proc1->source2_port(), proc3->set_sink2_port() );

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Step 1", p.execute(), process::SUCCESS );
  TEST( "Step 2", p.execute(), process::SUCCESS );
  TEST( "Step 3", p.execute(), process::SUCCESS );
  TEST( "Step 4", p.execute(), process::SUCCESS );
  TEST( "Step 5 (step should fail)", p.execute(), process::FAILURE );
  TEST( "Data1 flowed properly", g_sink1, g_source1 );
  TEST( "Data1 flow ran 2 times", g_sink1, 2 );
  std::cout << "flow 1 = " << g_sink1 << "\n";
  TEST( "Data2 flowed properly", g_sink2, g_source2 );
  TEST( "Data2 flow ran 2 times", g_sink2, 2 );
  std::cout << "flow 2 = " << g_sink2 << "\n";
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
