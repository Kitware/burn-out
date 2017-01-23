/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <process_framework/pipeline_aid.h>
#include <process_framework/process.h>
#include <utilities/config_block.h>
#include <pipeline_framework/sync_pipeline.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


struct do_something
  : public process
{
  typedef do_something self_type;

  do_something( vcl_string const& _name, unsigned max )
    : process( _name, "do_something" ),
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


void
test_connect_duplicate_input_run()
{
  {
    sync_pipeline p;
    do_something n1( "n1", 5 );
    p.add( &n1 );
    do_something n2( "n2", 3 );
    p.add( &n2 );
    p.connect( n1.datum_port(), n2.set_datum_port() );
    // The connect should fatally error.
    p.connect( n1.datum_port(), n2.set_datum_port() );
  }
}


} // end anonymous namespace

int test_connect_duplicate_input( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_connect_duplicate_input" );

  test_connect_duplicate_input_run();

  return testlib_test_summary();
}
