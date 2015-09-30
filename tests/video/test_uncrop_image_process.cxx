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

#include <video/uncrop_image_process.h>

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
test_uncrop( unsigned h_pad, unsigned v_pad, 
             vxl_byte bkgnd_val,
             bool disabled,
             unsigned trueni, unsigned truenj )
{
  uncrop_image_process<vxl_byte> uncrop_process( "up" );

  config_block params = uncrop_process.params();
  params.set( "horizontal_padding", h_pad );
  params.set( "vertical_padding", v_pad );
  params.set( "background_value", vul_sprintf( "%d", bkgnd_val ) );
  if( disabled )
  {
    h_pad = 0;
    v_pad = 0;
    truenj = 240;
    trueni = 320;
    params.set( "disabled", "true" );
  }

  vcl_cout << "Trying with the following parameters\n";
  params.print( vcl_cout );

  TEST( "Set parameters", true, uncrop_process.set_params( params ) );
  TEST( "Initialize", true, uncrop_process.initialize() );

  vil_image_view<vxl_byte> src;
  for( unsigned step = 0; step < 3; ++step )
  {
    vcl_cout << "Step " << step << "\n";
    src = make_source( 320, 240, 1, 3 );
    uncrop_process.set_source_image( src );
    TEST( "Step", uncrop_process.step(), true );
    vil_image_view<vxl_byte> uncropped = uncrop_process.uncropped_image();
    TEST( "Uncropped image size",
          uncropped.ni() == trueni &&
          uncropped.nj() == truenj &&
          uncropped.nplanes() == src.nplanes(),
          true );

    // Top-left corner
    TEST( "Uncropped top-left corner sample",
          uncropped(h_pad,v_pad,2) == src(0,0,2),
          true );

    // Bottom-right corner
    TEST( "Uncropped bottom-right sample",
          uncropped(h_pad+src.ni()-1,v_pad+src.nj()-1,2) == src(src.ni()-1, src.nj()-1,2),
          true );

    if( h_pad > 0 )
    {
      TEST( "Uncropped (right of top-left) pixel sample",
            uncropped(h_pad-1,v_pad,0) == bkgnd_val,
            true );
      TEST( "Uncropped  (right of to top-left) pixel sample",
            uncropped(h_pad-1,v_pad,1) == bkgnd_val,
            true );

      TEST( "Uncropped (right of bottom-right) pixel sample",
            uncropped(h_pad+src.ni(), v_pad+src.nj()-1, 0) == bkgnd_val,
            true );
      TEST( "Uncropped (pixel sample 3",
            uncropped(h_pad+src.ni(), v_pad+src.nj()-1, 1) == bkgnd_val,
            true );
    }

    if( v_pad > 0 )
    {
      TEST( "Uncropped  (left of  top-left) pixel sample",
            uncropped(h_pad,v_pad-1,0) == bkgnd_val,
            true );
      TEST( "Uncropped (below bottom-right) pixel sample",
            uncropped(h_pad+src.ni()-1,v_pad+src.nj(),0) == bkgnd_val,
            true );
    }
  }
}

void
test_uncrop_bin( unsigned h_pad, unsigned v_pad, 
                 bool bkgnd_val,
                 bool disabled,
                 unsigned trueni, unsigned truenj )
{
  uncrop_image_process<bool> uncrop_process( "up" );

  config_block params = uncrop_process.params();
  params.set( "horizontal_padding", h_pad );
  params.set( "vertical_padding", v_pad );
  params.set( "background_value", bkgnd_val );
  if( disabled )
  {
    h_pad = 0; 
    v_pad = 0;
    truenj = 240;
    trueni = 320;
    params.set( "disabled", "true" );
  }

  vcl_cout << "Trying with the following parameters\n";
  params.print( vcl_cout );

  TEST( "Set parameters", true, uncrop_process.set_params( params ) );
  TEST( "Initialize", true, uncrop_process.initialize() );

  vil_image_view<bool> src( 320, 240);
  src.fill( !bkgnd_val);
  uncrop_process.set_source_image( src );

  TEST( "Step", uncrop_process.step(), true );
  vil_image_view<bool> uncropped = uncrop_process.uncropped_image();
  TEST( "Uncropped image size",
        uncropped.ni() == trueni &&
        uncropped.nj() == truenj &&
        uncropped.nplanes() == src.nplanes(),
        true );

  // Top-left corner
  TEST( "Uncropped top-left corner sample", uncropped(h_pad,v_pad) == src(0,0), true );

  // Bottom-right corner
  TEST( "Uncropped bottom-right sample",
        uncropped(h_pad+src.ni()-1,v_pad+src.nj()-1) == src(src.ni()-1, src.nj()-1),
        true );

  if( h_pad > 0 )
  {
    TEST( "Uncropped (right of top-left) pixel sample",
          uncropped(h_pad-1,v_pad) == bkgnd_val,
          true );
    TEST( "Uncropped  (right of to top-left) pixel sample",
          uncropped(h_pad-1,v_pad) == bkgnd_val,
          true );

    TEST( "Uncropped (right of bottom-right) pixel sample",
          uncropped(h_pad+src.ni(), v_pad+src.nj()-1) == bkgnd_val,
          true );
    TEST( "Uncropped (pixel sample 3",
          uncropped(h_pad+src.ni(), v_pad+src.nj()-1) == bkgnd_val,
          true );
  }

  if( v_pad > 0 )
  {
    TEST( "Uncropped  (left of  top-left) pixel sample",
          uncropped(h_pad,v_pad-1) == bkgnd_val,
          true );
    TEST( "Uncropped (below bottom-right) pixel sample",
          uncropped(h_pad+src.ni()-1,v_pad+src.nj()) == bkgnd_val,
          true );
  }

}


} // end anonymous namespace

int test_uncrop_image_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "uncrop_image_process" );

  vcl_cout << "\n Checking vxl_byte 16x8.\n" << vcl_endl;
  test_uncrop( /*ununcrop param*/ 16, 8, 1, 
               /*disabled*/ false,
               /*true uncropped size*/ 352, 256 );

  vcl_cout << "\n Checking vxl_byte disabled.\n" << vcl_endl;
  test_uncrop( /*ununcrop param*/ 16, 8, 1, 
               /*disabled*/ true,
               /*true uncropped size*/ 352, 256 );

  vcl_cout << "\n Checking bin 16x8.\n" << vcl_endl;
  test_uncrop_bin( /*ununcrop param*/ 16, 8, false,
                   /*disabled*/ false,
                   /*true uncropped size*/ 352, 256 );

  vcl_cout << "\n Checking bin 16x8, inverted background.\n" << vcl_endl;
  test_uncrop_bin( /*ununcrop param*/ 16, 8, true,
                   /*disabled*/ false,
                   /*true uncropped size*/ 352, 256 );

  vcl_cout << "\n Checking bin 0x0.\n" << vcl_endl;
  test_uncrop_bin( /*ununcrop param*/ 0, 0, false,
                   /*disabled*/ false,
                   /*true uncropped size*/ 320, 240 );

  vcl_cout << "\n Checking bool disabled.\n" << vcl_endl;
  test_uncrop_bin( /*ununcrop param*/ 16, 8, false,
                   /*disabled*/ true,
                   /*true uncropped size*/ 352, 256 );

  return testlib_test_summary();
}
