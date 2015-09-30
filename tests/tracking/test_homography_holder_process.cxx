/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/homography_holder_process.h>
#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>

using namespace vidtk;

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void test_parameters()
{
  homography_holder_process hhp( "hhp" );

  config_block blk = hhp.params();

  try
  {
    blk.set( "matrix", "20.5 0 5  0 3 5  0 0 1" );
    blk.set( "scale", "0.5" );
    blk.set( "translation", "10 -20.5" );
    blk.set( "reference_image:frame_number", "15" );
    blk.set( "reference_image:time_in_secs", "0.5" );
  }
  catch( unchecked_return_value & e )
  {
    TEST("Setting parameters in config block.", true, false );
  }

  TEST( "set_params() called.", hhp.set_params( blk ), true );

  TEST( "initialize() called.", hhp.initialize(), true );

  TEST( "step() called.", hhp.step(), true );

  image_to_plane_homography H = hhp.homography_ref_to_wld();

  homography::transform_t T = H.get_transform();

  TEST( "Checking x scale", T.get(0,0), 10.25 );

  TEST( "Checking y scale", T.get(1,1), 1.5 );

  TEST( "Checking x translation", T.get(0,2), 10 );

  TEST( "Checking y translation", T.get(1,2), -20.5 );

  TEST( "Checking 2,2", T.get(2,2), 1 );

  TEST( "Checking frame_number", H.get_source_reference().frame_number(), 15 );

  TEST( "Checking timestamp", H.get_source_reference().time_in_secs(), 0.5 );
} // test_parameters()

} // end anonymous namespace

int test_homography_holder_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "homography holder process" );

  test_parameters();

  return testlib_test_summary();
}
