/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <testlib/testlib_test.h>

#include <video_transforms/mask_image_process.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


void
step( mask_image_process& proc, vil_image_view<bool> const& img, vil_image_view<bool> const& mask  )
{
  proc.set_mask_image( mask );
  proc.set_source_image( img );
  if( ! proc.step() )
  {
    TEST( "Step with image & mask", false, true );
  }
}

void
step_no_mask( mask_image_process& proc, vil_image_view<bool> const& img )
{
  proc.set_source_image( img );
  if( ! proc.step() )
  {
    TEST( "Step with image & no mask", false, true );
  }
}

void
test_image_mask()
{
  vcl_cout << "\n\nTest image mask\n\n\n";

  mask_image_process proc( "proc" );
  config_block blk = proc.params();
  //blk.set( "masked_value", "false" );
  TEST( "Configure", proc.set_params( blk ), true );
  TEST( "Initialize", proc.initialize(), true );

  vil_image_view<bool> mask( 2, 2);
  mask.fill( false );
  mask(0,1) = true;
  mask(1,1) = true;

  vil_image_view<bool> img1( 2, 2);
  img1.fill( false );
  img1(1,0) = true;
  img1(1,1) = true;

  step( proc, img1, mask );

  TEST( "Off pixel (masked)", proc.image()(0,1), false );
  TEST( "On pixel (masked)", proc.image()(1,1), false );
  TEST( "On pixel (not masked) ", proc.image()(1,0), true );
  TEST( "Off pixel (not masked) ", proc.image()(0,0), false );

  vil_image_view<bool> img2( 2, 2);
  img2.fill( true );

  step( proc, img2, mask );

  TEST( "On pixel (masked)", proc.image()(0,1), false );
  TEST( "On pixel (masked)", proc.image()(1,1), false );
  TEST( "On pixel (not masked) ", proc.image()(1,0), true );
  TEST( "On pixel (not masked) ", proc.image()(0,0), true );

  step_no_mask( proc, img2 );

  TEST( "On pixel (not masked)", proc.image()(0,1), true );
  TEST( "On pixel (not masked)", proc.image()(1,1), true );
  TEST( "On pixel (not masked) ", proc.image()(1,0), true );
  TEST( "On pixel (not masked) ", proc.image()(0,0), true );

}


} // end anonymous namespace

int test_mask_image_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "mask_image_process" );

  test_image_mask();

  return testlib_test_summary();
}
