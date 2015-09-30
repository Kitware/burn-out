/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <testlib/testlib_test.h>

#include <video/deep_copy_image_process.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


void
step( deep_copy_image_process<vxl_byte>& proc, vil_image_view<vxl_byte> const& img )
{
  proc.set_source_image( img );
  if( ! proc.step() )
  {
    TEST( "Step with image", false, true );
  }
}

void
test_deep_copy()
{
  vcl_cout << "\n\nTest deep copy\n\n\n";

  deep_copy_image_process< vxl_byte > proc( "proc" );
  TEST( "Initialize", proc.initialize(), true );

  vil_image_view<vxl_byte> img1( 2, 2, 3 );
  img1.fill( 0 );
  img1(0,0,0) = 1;

  step( proc, img1 );
  TEST( "Image was set", proc.image()(0,0,0), 1 );
  img1(0,0,0) = 5;
  TEST( "Image is deep copy", proc.image()(0,0,0), 1 );
  TEST( "Image layout is copied", img1.planestep(), proc.image().planestep() );

  vil_image_view<vxl_byte> img2( 2, 2, 1, 3 );
  img2.fill( 0 );
  img2(0,0,0) = 2;

  step( proc, img2 );
  TEST( "Image was set", proc.image()(0,0,0), 2 );
  img2(0,0,0) = 6;
  TEST( "Image is deep copy", proc.image()(0,0,0), 2 );
  TEST( "Image layout is copied", img2.planestep(), proc.image().planestep() );
}


void
test_force_component_order()
{
  vcl_cout << "\n\nTest force component order\n\n\n";

  deep_copy_image_process< vxl_byte > proc( "proc" );
  config_block blk = proc.params();
  blk.set( "force_component_order", "true" );
  TEST( "Configure", proc.set_params( blk ), true );
  TEST( "Initialize", proc.initialize(), true );

  vil_image_view<vxl_byte> img1( 2, 2, 3 );
  img1.fill( 0 );
  img1(0,0,0) = 1;

  step( proc, img1 );
  TEST( "Image was set", proc.image()(0,0,0), 1 );
  img1(0,0,0) = 5;
  TEST( "Image is deep copy", proc.image()(0,0,0), 1 );
  TEST( "Image layout is component-wise", proc.image().planestep(), 1 );

  vil_image_view<vxl_byte> img2( 2, 2, 1, 3 );
  img2.fill( 0 );
  img2(0,0,0) = 2;

  step( proc, img2 );
  TEST( "Image was set", proc.image()(0,0,0), 2 );
  img2(0,0,0) = 6;
  TEST( "Image is deep copy", proc.image()(0,0,0), 2 );
  TEST( "Image layout is component-wise", proc.image().planestep(), 1 );
}


void
test_shallow_copy()
{
  vcl_cout << "\n\nTest shallow copy\n\n\n";

  deep_copy_image_process< vxl_byte > proc( "proc" );
  config_block blk = proc.params();
  blk.set( "deep_copy", "false" );
  TEST( "Configure", proc.set_params( blk ), true );
  TEST( "Initialize", proc.initialize(), true );

  vil_image_view<vxl_byte> img1( 2, 2, 3 );
  img1.fill( 0 );
  img1(0,0,0) = 1;

  step( proc, img1 );
  TEST( "Image was set", proc.image()(0,0,0), 1 );
  img1(0,0,0) = 5;
  TEST( "Image is a shallow copy", proc.image()(0,0,0), 5 );


  vil_image_view<vxl_byte> img2( 2, 2, 1, 3 );
  img2.fill( 0 );
  img2(0,0,0) = 2;

  step( proc, img2 );
  TEST( "Image was set", proc.image()(0,0,0), 2 );
  img2(0,0,0) = 6;
  TEST( "Image is a shallow copy", proc.image()(0,0,0), 6 );
  TEST( "Image is a not related to first image", img1(0,0,0), 5 );
  TEST( "Image layout is copied", img2.planestep(), proc.image().planestep() );
}


} // end anonymous namespace

int test_deep_copy_image_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "deep_copy_image_process" );

  test_deep_copy();
  test_shallow_copy();
  test_force_component_order();

  return testlib_test_summary();
}
