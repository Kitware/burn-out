/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_algorithm.h>
#include <vil/vil_image_view.h>
#include <vnl/vnl_random.h>
#include <vul/vul_sprintf.h>
#include <testlib/testlib_test.h>

#include <video/crop_image_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

vnl_random g_rand;

// same parameters as the vil_image_view constructor
vil_image_view<vxl_byte>
make_source( unsigned ni, unsigned nj, unsigned np1, unsigned np2 )
{
  vil_image_view<vxl_byte> img( ni, nj, np1, np2 );
  unsigned np = vcl_max( np1, np2 );

  for( unsigned j = 0; j < nj; ++j )
  {
    for( unsigned i = 0; i < ni; ++i )
    {
      for( unsigned p = 0; p < np; ++p )
      {
        img(i,j,p) = g_rand.lrand32(0,255);
      }
    }
  }

  return img;
}


void
test_crop( int ulx, int uly, int lrx, int lry,
           bool disabled,
           unsigned truei0, unsigned truej0,
           unsigned trueni, unsigned truenj )
{
  crop_image_process<vxl_byte> crop_process( "cp" );

  config_block params = crop_process.params();
  params.set( "upper_left", vul_sprintf( "%d %d", ulx, uly ) );
  params.set( "lower_right", vul_sprintf( "%d %d", lrx, lry ) );
  if( disabled )
  {
    params.set( "disabled", "true" );
  }

  vcl_cout << "Trying with the following parameters\n";
  params.print( vcl_cout );

  TEST( "Set parameters", true, crop_process.set_params( params ) );
  TEST( "Initialize", true, crop_process.initialize() );

  vil_image_view<vxl_byte> src;
  for( unsigned step = 0; step < 3; ++step )
  {
    vcl_cout << "Step " << step << "\n";
    src = make_source( 256, 256, 1, 3 );
    crop_process.set_source_image( src );
    TEST( "Step", crop_process.step(), true );
    vil_image_view<vxl_byte> cropped = crop_process.cropped_image();
    TEST( "Cropped image size",
          cropped.ni() == trueni &&
          cropped.nj() == truenj &&
          cropped.nplanes() == src.nplanes(),
          true );
    TEST( "Cropped pixel sample",
          cropped(2,1,2) == src(truei0+2,truej0+1,2),
          true );
  }
}


} // end anonymous namespace

int test_crop_image_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "crop_image_process" );

  test_crop( /*crop param*/ 5, 8, 16, 43,
             /*disabled*/ false,
             /*true offset*/ 5, 8,
             /*true cropped size*/ 11, 35 );

  test_crop( /*crop param*/ 5, 8, -16, -43,
             /*disabled*/ false,
             /*true offset*/ 5, 8,
             /*true cropped size*/ 235, 205 );

  test_crop( /*crop param*/ 10, 8, 0, 0,
             /*disabled*/ false,
             /*true offset*/ 10, 8,
             /*true cropped size*/ 246, 248 );

  test_crop( /*crop param*/ -16, -8, 0, 0,
             /*disabled*/ false,
             /*true offset*/ 240, 248,
             /*true cropped size*/ 16, 8 );

  test_crop( /*crop param*/ -16, 8, -8, 16,
             /*disabled*/ false,
             /*true offset*/ 240, 8,
             /*true cropped size*/ 8, 8 );

  return testlib_test_summary();
}
