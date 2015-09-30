/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/transform_homography_process.h>
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

  vcl_cout << "Result is\n" << H.get_matrix() << "\n";
  TEST_NEAR( "Result is correct",
             (H.get_matrix()-M).frobenius_norm(),
             0.0, 1e-6 );
}


void
test_no_xform()
{
  vcl_cout << "\n\nTest no transform\n\n\n";

  transform_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  double v[9] = { 1.0, 0.0, 0.0,
                  0.0, 2.0, 0.0,
                  0.0, 0.0, 3.0 };
  vnl_matrix_fixed<double,3,3> M( v );
  vgl_h_matrix_2d<double> srcH( M );

  xform_proc.set_source_homography( srcH );
  TEST( "Step", xform_proc.step(), true );
  test_equal( xform_proc.homography(),
              1.0, 0.0, 0.0,
              0.0, 2.0, 0.0,
              0.0, 0.0, 3.0 );
}


void
test_pre_xform()
{
  vcl_cout << "\n\nTest pre-multiply\n\n\n";

  transform_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "premult_matrix", "1 0 0  0 1 0  1 0 2" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  double v[9] = { 1.0, 0.0, 0.0,
                  0.0, 2.0, 0.0,
                  0.0, 0.0, 3.0 };
  vnl_matrix_fixed<double,3,3> M( v );
  vgl_h_matrix_2d<double> srcH( M );

  xform_proc.set_source_homography( srcH );
  TEST( "Step", xform_proc.step(), true );
  test_equal( xform_proc.homography(),
              1.0, 0.0, 0.0,
              0.0, 2.0, 0.0,
              1.0, 0.0, 6.0 );
}


void
test_post_xform()
{
  vcl_cout << "\n\nTest post-multiply\n\n\n";

  transform_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "postmult_matrix", "1 0 0  0 1 0  1 0 2" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  double v[9] = { 1.0, 0.0, 0.0,
                  0.0, 2.0, 0.0,
                  0.0, 0.0, 3.0 };
  vnl_matrix_fixed<double,3,3> M( v );
  vgl_h_matrix_2d<double> srcH( M );

  xform_proc.set_source_homography( srcH );
  TEST( "Step", xform_proc.step(), true );
  test_equal( xform_proc.homography(),
              1.0, 0.0, 0.0,
              0.0, 2.0, 0.0,
              3.0, 0.0, 6.0 );
}


void
test_both_xform()
{
  vcl_cout << "\n\nTest both pre- and post-multiply\n\n\n";

  transform_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "premult_matrix", "1 0 0  0 1 0  1 0 3" );
  blk.set( "postmult_matrix", "1 0 0  0 1 0  1 0 2" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  double v[9] = { 1.0, 0.0, 0.0,
                  0.0, 2.0, 0.0,
                  0.0, 0.0, 3.0 };
  vnl_matrix_fixed<double,3,3> M( v );
  vgl_h_matrix_2d<double> srcH( M );

  xform_proc.set_source_homography( srcH );
  TEST( "Step", xform_proc.step(), true );
  test_equal( xform_proc.homography(),
              1.0, 0.0, 0.0,
              0.0, 2.0, 0.0,
              10.0, 0.0, 18.0 );
}


} // end anonymous namespace

int test_transform_homography_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "transformation_homography_process" );

  test_no_xform();
  test_pre_xform();
  test_post_xform();
  test_both_xform();

  return testlib_test_summary();
}
