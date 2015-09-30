/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <tracking/full_tracking_super_process.h>
#include <utilities/config_block.h>
#include <vxl_config.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_finalizer_params()
{
  full_tracking_super_process< vxl_byte > ftsp( "ftsp" );
  config_block blk;
  blk = ftsp.params();

  // Check to make sure enabled is tested before checking
  // the finalizing duration.
  blk.set( "detect_and_track_sp:output_finalizer:enabled", "false" );
  blk.set( "detect_and_track_sp:output_finalizer:delay_frames", "5" );
  blk.set( "detect_and_track_sp:tracking_sp:track_init_duration_frames", "10" );
  TEST( "Finalizer disabled, trying to set parameters",
        ftsp.set_params( blk ), true );

  blk.set( "detect_and_track_sp:output_finalizer:enabled", "true" );
  TEST( "Finalizer enabled, trying to set incorrect delay_frames",
        ftsp.set_params( blk ), false );

  // Check to see if a legitimate case works.
  blk.set( "detect_and_track_sp:output_finalizer:delay_frames", "21" );
  blk.set( "detect_and_track_sp:tracking_sp:track_termination_duration_frames", "5" );
  blk.set( "detect_and_track_sp:tracking_sp:back_tracking_disabled", "false" );
  blk.set( "detect_and_track_sp:tracking_sp:back_tracking_duration_frames", "10" );
  TEST( "Finalizer enabled, trying to set correct delay_frames",
        ftsp.set_params( blk ), true );

  blk.set( "detect_and_track_sp:output_finalizer:delay_frames", "20" );
  TEST( "Finalizer enabled, trying to set incorrect delay_frames",
        ftsp.set_params( blk ), false );
}

} // end anonymous namespace


int test_full_tracking_super_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "full_tracking_super_process" );

  test_finalizer_params();

  return testlib_test_summary();
}
