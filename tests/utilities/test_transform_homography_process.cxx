/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <fstream>
#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/transform_vidtk_homography_process.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

vgl_h_matrix_2d<double> create_homog( double v[9])
{
  vnl_matrix_fixed<double,3,3> M( v );
  return vgl_h_matrix_2d<double>( M );
}

#define ORIGINAL(H) \
  do{ \
     double v[9] = { 1.0, 0.0, 0.0, \
                     0.0, 2.0, 0.0, \
                     0.0, 0.0, 3.0 }; \
     H.set_transform(create_homog(v)); \
     H.set_source_reference(timestamp(5, 10)); \
     H.set_dest_reference(timestamp(10, 15)); \
  }while(false);

void
test_equal( vgl_h_matrix_2d<double> const& H,
            vgl_h_matrix_2d<double> const& trueH
          )
{
  std::cout << "Result is\n" << H.get_matrix() << std::endl;
  std::cout << "Truth is \n" << trueH << std::endl;
  TEST_NEAR( "Result is correct",
             (H.get_matrix()-trueH.get_matrix()).frobenius_norm(),
             0.0, 1e-6 );
}

void
test_scaler_premult()
{
  transform_image_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "premult_scale", "5.0" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  image_to_image_homography itwh;
  ORIGINAL(itwh);

  xform_proc.set_source_homography( itwh );
  TEST( "Step", xform_proc.step(), true );
  double t1[] = {5.0, 0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 3.0};
  test_equal( xform_proc.bare_homography(), create_homog( t1 ) );
  double t2[] = {5.0, 0.0, 0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 1.0 };
  test_equal( xform_proc.get_premult_homography(), create_homog( t2 ) );
}


void
test_no_xform()
{
  std::cout << "\n\nTest no transform\n\n\n";

  transform_image_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  image_to_image_homography itwh;
  ORIGINAL(itwh);

  xform_proc.set_source_homography( itwh );
  TEST( "Step", xform_proc.step(), true );
  test_equal( xform_proc.bare_homography(), itwh.get_transform() );
  test_equal( xform_proc.inv_bare_homography(), itwh.get_inverse().get_transform() );
}


void
test_pre_xform()
{
  std::cout << "\n\nTest pre-multiply\n\n\n";

  transform_image_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "premult_matrix", "1 0 0  0 1 0  1 0 2" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  image_to_image_homography itwh;
  ORIGINAL(itwh);

  xform_proc.set_source_homography( itwh );
  TEST( "Step", xform_proc.step(), true );
  double t1[] =  { 1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 1.0, 0.0, 6.0 };
  test_equal( xform_proc.bare_homography(), create_homog(t1) );
  double t2[] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 2.0};
  test_equal( xform_proc.get_premult_homography(), create_homog(t2) );
  double t3[] = {1.0, 0.0, 0.0, 0.0, 0.5, 0.0, -0.166667, 0.0, 0.166667};
  test_equal( xform_proc.inv_bare_homography(), create_homog(t3) );
}


void
test_post_xform()
{
  std::cout << "\n\nTest post-multiply\n\n\n";

  transform_image_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "postmult_matrix", "1 0 0  0 1 0  1 0 2" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  image_to_image_homography itwh;
  ORIGINAL(itwh);

  xform_proc.set_source_homography( itwh );
  TEST( "Step", xform_proc.step(), true );
  double t1[] =  { 1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 3.0, 0.0, 6.0 };
  test_equal( xform_proc.bare_homography(), create_homog(t1) );
  double t3[] = {1.0, 0.0, 0.0, 0.0, 0.5, 0.0,-0.5, 0.0, 0.166667};
  test_equal( xform_proc.inv_bare_homography(), create_homog(t3) );
}


void
test_both_xform()
{
  std::cout << "\n\nTest both pre- and post-multiply\n\n\n";

  transform_image_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "premult_matrix", "1 0 0  0 1 0  1 0 3" );
  blk.set( "postmult_matrix", "1 0 0  0 1 0  1 0 2" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  image_to_image_homography itwh;
  ORIGINAL(itwh);

  xform_proc.set_source_homography( itwh );
  TEST( "Step", xform_proc.step(), true );
  double t1[] =  { 1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 10.0, 0.0, 18.0 };
  test_equal( xform_proc.bare_homography(), create_homog(t1) );
  double t2[] = {1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  1.0, 0.0, 3.0 };
  test_equal( xform_proc.get_premult_homography(), create_homog(t2) );
  double t3[] = {1.0, 0.0, 0.0, 0.0, 0.5, 0.0, -0.555556, 0.0, 0.0555556};
  test_equal( xform_proc.inv_bare_homography(), create_homog(t3) );
}

void test_xform_set()
{
  transform_image_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  double s0[] =  { 1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  1.0, 0.0, 2};

  image_to_image_homography post;
  post.set_transform(create_homog(s0));
  post.set_source_reference(timestamp(5, 10));
  post.set_dest_reference(timestamp(10, 15));
  xform_proc.set_postmult_homography(post);

  double s1[] =  {1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  1.0, 0.0, 3.0};
  image_to_image_homography pre;
  pre.set_transform(create_homog(s1));
  pre.set_source_reference(timestamp(5, 10));
  pre.set_dest_reference(timestamp(10, 15));
  xform_proc.set_premult_homography(pre);

  image_to_image_homography itwh;
  ORIGINAL(itwh);

  xform_proc.set_source_homography( itwh );
  TEST( "Step", xform_proc.step(), true );
  double t1[] =  { 1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 10.0, 0.0, 18.0 };
  test_equal( xform_proc.bare_homography(), create_homog(t1) );
  double t2[] = {1.0, 0.0, 0.0,  0.0, 1.0, 0.0,  1.0, 0.0, 3.0 };
  test_equal( xform_proc.get_premult_homography(), create_homog(t2) );
  double t3[] = {1.0, 0.0, 0.0, 0.0, 0.5, 0.0, -0.555556, 0.0, 0.0555556};
  test_equal( xform_proc.inv_bare_homography(), create_homog(t3) );

}

void test_pre_file()
{
  {
    std::ofstream out("test_homog.txt");
    out << "1 0 0  0 1 0  1 0 2" << std::endl;
    out.close();
  }
  transform_image_homography_process xform_proc( "buf" );
  config_block blk = xform_proc.params();
  blk.set( "premult_filename", "test_homog.txt" );
  TEST( "Configure buffer", xform_proc.set_params( blk ), true );
  TEST( "Initialize buffer", xform_proc.initialize(), true );

  image_to_image_homography itwh;
  ORIGINAL(itwh);

  xform_proc.set_source_homography( itwh );
  TEST( "Step", xform_proc.step(), true );
  double t1[] =  { 1.0, 0.0, 0.0, 0.0, 2.0, 0.0, 1.0, 0.0, 6.0 };
  test_equal( xform_proc.bare_homography(), create_homog(t1) );
  double t2[] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 2.0};
  test_equal( xform_proc.get_premult_homography(), create_homog(t2) );
  double t3[] = {1.0, 0.0, 0.0, 0.0, 0.5, 0.0, -0.166667, 0.0, 0.166667};
  test_equal( xform_proc.inv_bare_homography(), create_homog(t3) );
}

void test_errors()
{
  {
    transform_image_homography_process xform_proc( "buf" );
    config_block blk = xform_proc.params();
    blk.set( "premult_matrix", "1 0 0  0 1 0  1 0 2" );
    blk.set( "premult_scale", "5.0" );
    TEST( "Cannot have two premult config", xform_proc.set_params( blk ), false );
  }
  {
    transform_image_homography_process xform_proc( "buf" );
    config_block blk = xform_proc.params();
    blk.set( "premult_scale", "5.0" );
    blk.set( "premult_filename", "test_homog.txt" );
    TEST( "Cannot have two premult config", xform_proc.set_params( blk ), false );
  }
  {
    transform_image_homography_process xform_proc( "buf" );
    config_block blk = xform_proc.params();
    blk.set( "premult_filename", "test_homog.txt/DOES_NOT_EXIST.txt" );
    TEST( "File does not exist, set params", xform_proc.set_params( blk ), true );
    TEST( "File does not exist, init", xform_proc.initialize(), false );
  }
  {
    {
      std::ofstream out("test_homog.txt");
      out << "1 0 0  0 1 0" << std::endl;
      out.close();
    }
    transform_image_homography_process xform_proc( "buf" );
    config_block blk = xform_proc.params();
    blk.set( "premult_filename", "test_homog.txt" );
    TEST( "File bad, set params", xform_proc.set_params( blk ), true );
    TEST( "File bad, init", xform_proc.initialize(), false );
  }
}


} // end anonymous namespace

int test_transform_homography_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "transformation_homography_process" );

  test_no_xform();
  test_pre_xform();
  test_post_xform();
  test_both_xform();
  test_scaler_premult();
  test_xform_set();
  test_pre_file();
  test_errors();

  return testlib_test_summary();
}
