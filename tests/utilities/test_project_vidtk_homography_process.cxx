/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/project_vidtk_homography_process.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


void
test_equal( vgl_h_matrix_2d<double> const& H,
            double m00, double m01, double m02,
            double m10, double m11, double m12,
            double m20, double m21, double m22 )
{
  vnl_matrix_fixed<double,3,3> M;
  M(0,0) = m00;   M(0,1) = m01;   M(0,2) = m02;
  M(1,0) = m10;   M(1,1) = m11;   M(1,2) = m12;
  M(2,0) = m20;   M(2,1) = m21;   M(2,2) = m22;

  std::cout << "Result is\n" << H.get_matrix() << "\n";
  TEST_NEAR( "Result is correct",
             (H.get_matrix()-M).frobenius_norm(),
             0.0, 1e-6 );
}


void
test_no_xform()
{
  std::cout << "\n\nTest no transform\n\n\n";

  project_vidtk_homography_process< timestamp, timestamp > proj_proc( "buf" );
  config_block blk = proj_proc.params();
  TEST( "Configure ", proj_proc.set_params( blk ), true );
  TEST( "Initialize ", proj_proc.initialize(), true );

  double v[9] = { 1.0, 0.0, 0.0,
                  0.0, 2.0, 0.0,
                  0.0, 0.0, 3.0 };
  vnl_matrix_fixed<double,3,3> M( v );

  project_vidtk_homography_process< timestamp, timestamp >::homog_t srcH;
  srcH.set_transform( M );
  srcH.set_source_reference( timestamp( 10 ) );
  srcH.set_dest_reference( timestamp( 1 ) );

  proj_proc.set_source_homography( srcH );
  TEST( "Step", proj_proc.step(), true );
  TEST( "Checking output (src time):", srcH.get_source_reference(), timestamp( 10 ) );
  TEST( "Checking output (dst time):", srcH.get_dest_reference(), timestamp( 1 ) );
  test_equal( proj_proc.homography().get_transform(),
              1.0, 0.0, 0.0,
              0.0, 2.0, 0.0,
              0.0, 0.0, 3.0 );
}

void
test_scale()
{
  std::cout << "\n\nTest scale\n\n\n";

  project_vidtk_homography_process< timestamp, timestamp > proj_proc( "buf" );
  config_block blk = proj_proc.params();
  blk.set( "scale", 0.5 );
  TEST( "Configure ", proj_proc.set_params( blk ), true );
  TEST( "Initialize ", proj_proc.initialize(), true );

  double v[9] = { 1.0, 0.0, 10.0,
                  0.0, 1.0, 20.0,
                  0.0, 0.0, 1.0 };
  vnl_matrix_fixed<double,3,3> M( v );
  project_vidtk_homography_process< timestamp, timestamp >::homog_t srcH;
  srcH.set_transform( M );
  srcH.set_source_reference( timestamp( 10 ) );
  srcH.set_dest_reference( timestamp( 1 ) );

  proj_proc.set_source_homography( srcH );
  TEST( "Step", proj_proc.step(), true );
  TEST( "Checking output (src time):", srcH.get_source_reference(), timestamp( 10 ) );
  TEST( "Checking output (dst time):", srcH.get_dest_reference(), timestamp( 1 ) );

  test_equal( proj_proc.homography().get_transform(),
              1.0, 0.0, 20.0,
              0.0, 1.0, 40.0,
              0.0, 0.0, 1.0 );
}


void
test_matrix()
{
  std::cout << "\n\nTest matrix\n\n\n";

  project_vidtk_homography_process< timestamp, timestamp > proj_proc( "buf" );
  config_block blk = proj_proc.params();
  blk.set( "matrix", "0.5 0 0 0 0.5 0 0 0 1" );
  TEST( "Configure ", proj_proc.set_params( blk ), true );
  TEST( "Initialize ", proj_proc.initialize(), true );

  double v[9] = { 1.0, 0.0, 10.0,
                  0.0, 1.0, 20.0,
                  0.0, 0.0, 1.0 };
  vnl_matrix_fixed<double,3,3> M( v );
  project_vidtk_homography_process< timestamp, timestamp >::homog_t srcH;
  srcH.set_transform( M );
  srcH.set_source_reference( timestamp( 10 ) );
  srcH.set_dest_reference( timestamp( 1 ) );

  proj_proc.set_source_homography( srcH );
  TEST( "Step", proj_proc.step(), true );
  TEST( "Checking output (src time):", srcH.get_source_reference(), timestamp( 10 ) );
  TEST( "Checking output (dst time):", srcH.get_dest_reference(), timestamp( 1 ) );

  test_equal( proj_proc.homography().get_transform(),
              1.0, 0.0, 20.0,
              0.0, 1.0, 40.0,
              0.0, 0.0, 1.0 );
}

void
test_parameters()
{
  std::cout << "\n\nTest parameters\n\n\n";

  project_vidtk_homography_process< timestamp, timestamp > proj_proc( "buf" );
  config_block blk = proj_proc.params();
  blk.set( "scale", "0.5" );
  blk.set( "matrix", "0.5 0 0 0 0.5 0 0 0 1" );
  TEST( "Configure ", proj_proc.set_params( blk ), false );
}

} // end anonymous namespace

int test_project_vidtk_homography_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "project_vidtk_homography_process" );

  test_no_xform();
  test_scale();
  test_matrix();
  test_parameters();

  return testlib_test_summary();
}
