/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <testlib/testlib_test.h>

#include <process_framework/pipeline_aid.h>
#include <process_framework/process.h>
#include <utilities/config_block.h>
#include <pipeline/sync_pipeline.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


struct do_something
  : public process
{
  typedef do_something self_type;

  do_something( vcl_string const& name, unsigned max )
    : process( name, "do_something" ),
      max_count_( max ),
      count_( 0 ),
      recv_count_( 0 ),
      last_datum_( 0 )
  {
  }

  config_block params() const
  {
    return config_block();
  }

  bool set_params( config_block const& )
  {
    return true;
  }

  bool initialize()
  {
    count_ = 0;
    recv_count_ = 0;
    last_datum_ = 0;
    return true;
  }

  bool step()
  {
    if( count_ < max_count_ )
    {
      ++count_;
      return true;
    }
    else
    {
      return false;
    }
  }

  void set_datum( unsigned d )
  {
    last_datum_ = d;
    ++recv_count_;
  }

  VIDTK_INPUT_PORT( set_datum, unsigned );

  void set_optional_datum( unsigned d )
  {
    last_datum_ = d;
    ++recv_count_;
  }

  VIDTK_OPTIONAL_INPUT_PORT( set_optional_datum, unsigned );


  unsigned datum() const
  {
    return count_;
  }

  VIDTK_OUTPUT_PORT( unsigned, datum );

  unsigned max_count_;
  unsigned count_;
  unsigned recv_count_;
  unsigned last_datum_;
};


unsigned
execute_pipeline( pipeline& p, unsigned max )
{
  unsigned steps = 0;
  while( p.execute() && steps < max )
  {
    ++steps;
  }
  return steps;
}


void
test_stop_no_sink()
{
  // When no sinks are executable, the pipeline should stop.

  vcl_cout << "\n\nSimple single node\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 3 );
    p.add( &n1 );
    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 3 steps", steps, 3 );
  }

  vcl_cout << "\n\nTwo nodes w/ required\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 3 );
    p.add( &n1 );
    do_something n2( "n2", 5 );
    p.add( &n2 );
    p.connect( n1.datum_port(), n2.set_datum_port() );
    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 3 steps", steps, 3 );
  }

  vcl_cout << "\n\nTwo nodes w/ optional\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 3 );
    p.add( &n1 );
    do_something n2( "n2", 5 );
    p.add( &n2 );
    p.connect( n1.datum_port(), n2.set_optional_datum_port() );
    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 5 steps", steps, 5 );
  }

  vcl_cout << "\n\nTwo sources w/ required\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 3 );
    p.add( &n1 );
    do_something n2( "n2", 5 );
    p.add( &n2 );
    do_something n3( "n3", 10 );
    p.add( &n3 );
    p.connect( n1.datum_port(), n3.set_datum_port() );
    p.connect( n2.datum_port(), n3.set_datum_port() );
    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 3 steps", steps, 3 );
  }

  vcl_cout << "\n\nTwo sources w/ no execute source\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 10 );
    p.add( &n1 );
    do_something n2( "n2", 5 );
    p.add_without_execute( &n2 );
    do_something n3( "n3", 3 );
    p.add( &n3 );
    p.connect( n1.datum_port(), n3.set_datum_port() );
    p.connect( n2.datum_port(), n3.set_datum_port() );
    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 3 steps", steps, 3 );
  }

  vcl_cout << "\n\nTwo nodes w/ isolated no execute\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 5 );
    p.add( &n1 );
    do_something n2( "n2", 3 );
    p.add_without_execute( &n2 );

    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 5 steps", steps, 5 );
  }
}


void test_stop_no_output()
{
  // The pipeline should continue while there are output nodes, even
  // if they are not sinks.

  vcl_cout << "\n\nOutput not marked\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 5 );
    p.add( &n1 );
    do_something n2( "n2", 10 );
    p.add( &n2 );
    p.connect( n1.datum_port(), n2.set_datum_port() );
    do_something n3( "n3", 3 );
    p.add( &n3 );
    p.connect( n2.datum_port(), n3.set_datum_port() );

    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 3 steps", steps, 3 );
  }

  vcl_cout << "\n\nOutput marked\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 5 );
    p.add( &n1 );
    do_something n2( "n2", 10 );
    p.add( &n2 );
    p.set_output_node( &n2, true );
    p.connect( n1.datum_port(), n2.set_datum_port() );
    do_something n3( "n3", 3 );
    p.add( &n3 );
    p.connect( n2.datum_port(), n3.set_datum_port() );

    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 5 steps", steps, 5 );
  }

  vcl_cout << "\n\nSink marked as not output\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 5 );
    p.add( &n1 );
    do_something n2( "n2", 10 );
    p.add( &n2 );
    p.connect( n1.datum_port(), n2.set_datum_port() );
    do_something n3( "n3", 3 );
    p.add( &n3 );
    p.set_output_node( &n3, false );
    p.connect( n2.datum_port(), n3.set_datum_port() );

    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 0 steps", steps, 0 );
  }

  vcl_cout << "\n\nOne sink marked as not output, other isn't\n";
  {
    sync_pipeline p;
    do_something n1( "n1", 5 );
    p.add( &n1 );
    do_something n2( "n2", 10 );
    p.add( &n2 );
    p.connect( n1.datum_port(), n2.set_datum_port() );
    do_something n3( "n3", 3 );
    p.add( &n3 );
    p.set_output_node( &n3, false );
    p.connect( n2.datum_port(), n3.set_datum_port() );
    do_something n4( "n4", 4 );
    p.add( &n4 );
    p.connect( n2.datum_port(), n4.set_datum_port() );

    TEST( "Set params", p.set_params( p.params() ), true );
    TEST( "Initialize", p.initialize(), true );
    unsigned steps = execute_pipeline( p, 50 );
    vcl_cout << "Step count = " << steps << "\n";
    TEST( "Stopped after 4 steps", steps, 4 );
  }
}



} // end anonymous namespace

int test_stop_pipeline( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_stop_pipeline" );

  test_stop_no_sink();
  test_stop_no_output();

  return testlib_test_summary();
}
