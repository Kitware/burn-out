/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_string.h>
#include <vcl_cstdio.h>

#include <testlib/testlib_test.h>

#include <descriptors/tot_super_process.h>
#include <utilities/timestamp.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void run_simple_tests()
{
  tot_super_process<vxl_byte> tot_process( "tot_super_process" );

  config_block params = tot_process.params();
  params.set( "disabled", "true" );

  TEST( "Set parameters", true, tot_process.set_params( params ) );
  TEST( "Initialize", true, tot_process.initialize() );

  track_sptr track1( new track() );
  track_sptr track2( new track() );
  track1->set_id( 1 );
  track2->set_id( 2 );

  track::vector_t trks;
  trks.push_back( track1 );
  trks.push_back( track2 );

  tot_process.set_source_tracks( trks );
  tot_process.set_source_timestamp( timestamp( 0, 0 ) );

  TEST( "Step", tot_process.step2(), true );

  TEST( "Disabled Output", tot_process.output_tracks().size(), 2 );
}

} // end anonymous namespace

int test_tot_super_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "tot_super_process" );

  run_simple_tests();

  return testlib_test_summary();
}
