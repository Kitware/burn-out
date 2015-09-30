/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <utilities/homography_scoring_process.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;



void
test_scoring()
{
  vcl_cout << "\n\nTest datum nearest to\n\n\n";

  homography_scoring_process scoring( "scoring" );

  config_block blk = scoring.params();
  blk.set( "disabled", "false" );
  blk.set( "max_dist_offset", "2" );
  blk.set( "area_percent_factor", ".2" );
  blk.set( "quadrant", "0" );
  blk.set( "height", "1" );
  blk.set( "width", "1" );
  TEST( "Configure scoring", scoring.set_params( blk ), true );
  TEST( "Initialize scoring", scoring.initialize(), true );

  vgl_h_matrix_2d<double> good_homography;
  vgl_h_matrix_2d<double> test_homography;

  good_homography.set_identity();
  test_homography.set_identity();

  scoring.set_good_homography( good_homography );
  scoring.set_test_homography( test_homography );
  TEST( "Step", scoring.step(), true );
  TEST( "Identity homographies", scoring.is_good_homography(), true );

  test_homography.set_identity();
  test_homography.set_translation( 0.5, 0.5 );

  scoring.set_test_homography( test_homography );
  TEST( "Step", scoring.step(), true );
  TEST( "Distance difference (within range)", scoring.is_good_homography(), true );

  test_homography.set_identity();
  test_homography.set_translation( 4, 2 );

  scoring.set_test_homography( test_homography );
  TEST( "Step", scoring.step(), true );
  TEST( "Distance difference (outside of range)", scoring.is_good_homography(), false );

  test_homography.set_identity();
  test_homography.set_scale( 1.05 );

  scoring.set_test_homography( test_homography );
  TEST( "Step", scoring.step(), true );
  TEST( "Scaling (area within range)", scoring.is_good_homography(), true );

  test_homography.set_identity();
  test_homography.set_scale( 1.1 );

  scoring.set_test_homography( test_homography );
  TEST( "Step", scoring.step(), true );
  TEST( "Scaling (area out of range)", scoring.is_good_homography(), false );

  good_homography.set_identity();
  good_homography.set_translation( 1, 1 );
  test_homography.set_identity();
  test_homography.set_translation( 1, 1 );

  scoring.set_good_homography( good_homography );
  scoring.set_test_homography( test_homography );
  TEST( "Step", scoring.step(), true );
  TEST( "Translations (same)", scoring.is_good_homography(), true );

  good_homography.set_identity();
  good_homography.set_translation( 1, 1 );
  test_homography.set_identity();
  test_homography.set_translation( 1, 1 );
  test_homography.set_scale( 1.05 );

  scoring.set_good_homography( good_homography );
  scoring.set_test_homography( test_homography );
  TEST( "Step", scoring.step(), true );
  TEST( "Minor differences", scoring.is_good_homography(), true );
}


} // end anonymous namespace

int test_homography_scoring( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "homography_scoring_process" );

  test_scoring();

  return testlib_test_summary();
}
