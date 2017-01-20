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


int g_func1_result;
void function1( int A, double const& B )
{
  std::cout << "func1: A=" << A << "; B=" << B << "\n";
  if( A==5 && B==3.2 )
  {
    g_func1_result = 1;
  }
  else if( A==7 && B==3.1 )
  {
    g_func1_result = 2;
  }
  else
  {
    g_func1_result = 0;
  }
}


struct feeder_process
  : vidtk::process
{
  typedef feeder_process self_type;

  feeder_process()
    : vidtk::process( "feeder", "feeder_process" )
  {
  }

  virtual config_block params() const
  {
    return config_block();
  }

  virtual bool set_params( config_block const& )
  {
    return true;
  }

  virtual bool initialize()
  {
    int_value_1_ = 0;
    double_value_1_ = 0.0;

    return true;
  }

  virtual bool step()
  {
    int_value_1_ = next_int_value_1_;
    double_value_1_ = next_double_value_1_;

    return true;
  }

  int int_value_1() const
  {
    return int_value_1_;
  }

  VIDTK_OUTPUT_PORT( int, int_value_1 );

  double double_value_1() const
  {
    return double_value_1_;
  }

  VIDTK_OUTPUT_PORT( double, double_value_1 );

  int next_int_value_1_;
  double next_double_value_1_;

private:
  int int_value_1_;
  double double_value_1_;
};


void
test_function_caller_2_process()
{
  std::cout << "\n\nTest function_caller_2_process\n\n\n";

  vidtk::sync_pipeline p;

  feeder_process feeder;
  p.add( &feeder );

  function_caller_2_process<int, double const&> caller( "caller" );
  caller.set_function( &function1 );
  p.add( &caller );
  p.connect( feeder.int_value_1_port(), caller.set_argument_1_port() );
  p.connect( feeder.double_value_1_port(), caller.set_argument_2_port() );

  g_func1_result = -1;
  feeder.next_int_value_1_ = 5;
  feeder.next_double_value_1_ = 3.2;

  TEST( "Pipeline initialize", p.initialize(), true );
  TEST( "Pipeline step", p.execute(), process::SUCCESS );
  std::cout << "Func 1 result" << g_func1_result << "\n";
  TEST( "Function 1 was called correctly", g_func1_result, 1 );

  feeder.next_int_value_1_ = 7;
  feeder.next_double_value_1_ = 3.1;
  TEST( "Pipeline step", p.execute(), process::SUCCESS );
  std::cout << "Func 1 result" << g_func1_result << "\n";
  TEST( "Function 1 was called correctly", g_func1_result, 2 );
}


} // end anonymous namespace

int test_function_caller_processes( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "function_caller_processes" );

  test_function_caller_2_process();

  return testlib_test_summary();
}
