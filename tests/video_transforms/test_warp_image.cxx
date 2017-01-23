/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_transforms/warp_image.h>

#include <vil/vil_image_view.h>
#include <vil/vil_save.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

#include <iostream>
#include <algorithm>

#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

// same parameters as the vil_image_view constructor
vil_image_view<vxl_byte>
make_source( unsigned ni, unsigned nj, unsigned np1, unsigned np2 )
{
  vil_image_view<vxl_byte> img( ni, nj, np1, np2 );
  unsigned np = std::max( np1, np2 );

  for( unsigned bj = 0; bj < 4; ++bj )
  {
    for( unsigned bi = 0; bi < 4; ++bi )
    {
      unsigned ilo = ni/4 * bi;
      unsigned ihi = ni/4 * (bi+1);
      unsigned jlo = nj/4 * bj;
      unsigned jhi = nj/4 * (bj+1);
      for( unsigned j = jlo; j < jhi; ++j )
      {
        for( unsigned i = ilo; i < ihi; ++i )
        {
          for( unsigned p = 0; p < np; ++p )
          {
            unsigned idx;
            if( p % 3 == 0 )
            {
              idx = (bi+bj*4);
            }
            else if( p % 3 == 1 )
            {
              idx = ((3-bi)+bj*4);
            }
            else
            {
              idx = (bi+(3-bj)*4);
            }
            img(i,j,p) = idx*16 + p*2;
          }
        }
      }
    }
  }

  return img;
}

void
test_equal( char const* msg,
            vil_image_view<vxl_byte> const& img,
            unsigned i, unsigned j,
            vxl_byte r, vxl_byte g = 0, vxl_byte b = 0 )
{
  bool good = true;
  if( img.nplanes() >= 1 && img(i,j,0) != r )
  {
    good = false;
  }
  if( img.nplanes() >= 2 && img(i,j,1) != g )
  {
    good = false;
  }
  if( img.nplanes() >= 3 && img(i,j,2) != b )
  {
    good = false;
  }

  if( ! good )
  {
    std::cout << "At ("<<i<<","<<j<<"), expecting (got)";
    if( img.nplanes() >= 1 )
    {
      std::cout << " " << int(r) << "("<<int(img(i,j,0))<<")";
    }
    if( img.nplanes() >= 2 )
    {
      std::cout << " " << int(g) << "("<<int(img(i,j,1))<<")";
    }
    if( img.nplanes() >= 3 )
    {
      std::cout << " " << int(b) << "("<<int(img(i,j,2))<<")";
    }
    std::cout << "" << std::endl;
  }

  TEST( msg, good, true );
}


void
test_simple_warp_image()
{
  vil_image_view<vxl_byte> src = make_source( 32, 32, 1, 3 );

  vgl_h_matrix_2d<double> h;
  h.set_identity();

  {
    std::cout << "Identity" << std::endl;
    vil_image_view<vxl_byte> dest( 32, 32, 1, 3 );
    warp_image( src, dest, h );

    test_equal( "sample 1 matches", dest, 6,6, 0,50,196 );
    test_equal( "sample 2 matches", dest, 16,24, 224,210,36 );
  }

  {
    std::cout << "Translation +ve" << std::endl;
    h.set_translation( -4, -7 );
    vil_image_view<vxl_byte> dest( 32, 32, 1, 3 );
    dest.fill( 100 );
    warp_image( src, dest, h );

    test_equal( "sample 1 matches", dest, 10,13, 0,50,196 );
    test_equal( "sample 2 matches", dest, 20,31, 224,210,36 );
    test_equal( "sample 3 matches", dest, 4,6, 0,0,0 );
  }

  {
    std::cout << "Translation -ve" << std::endl;
    h.set_translation( 4, 7 );
    vil_image_view<vxl_byte> dest( 32, 32, 1, 3 );
    dest.fill( 100 );
    warp_image( src, dest, h );

    test_equal( "sample 1 matches", dest, 2,0, 0,50,196 );
    test_equal( "sample 2 matches", dest, 12,17, 224,210,36 );
    test_equal( "sample 3 matches", dest, 11,16, 144,162,84 );
    test_equal( "sample 4 matches", dest, 27,24, 240,194,52 );
    test_equal( "sample 5 matches", dest, 28,24, 0,0,0 );
  }

  {
    std::cout << "Unmapped fill parameter" << std::endl;
    h.set_translation( 4, 7 );
    vil_image_view<vxl_byte> dest( 32, 32, 1, 3 );
    dest.fill( 100 );
    vil_image_view<bool> unmapped_mask( 32, 32 );
    warp_image( src, dest, h,
                warp_image_parameters()
                .set_unmapped_value( 128 ),
                &unmapped_mask );

    test_equal( "sample 1 matches", dest, 2,0, 0,50,196 );
    test_equal( "sample 2 matches", dest, 12,17, 224,210,36 );
    test_equal( "sample 3 matches", dest, 11,16, 144,162,84 );
    test_equal( "sample 4 matches", dest, 27,24, 240,194,52 );
    test_equal( "sample 5 matches", dest, 28,24, 128,128,128 );

    std::cout<< "Unmapped mask" << std::endl;
    TEST( "sample 1 matches", unmapped_mask(2,0), false );
    TEST( "sample 2 matches", unmapped_mask(12,17), false );
    TEST( "sample 3 matches", unmapped_mask(11,16), false );
    TEST( "sample 4 matches", unmapped_mask(27,24), false );
    TEST( "sample 5 matches", unmapped_mask(28,24), true );
  }

  {
    std::cout << "Unmapped fill parameter binary mask" << std::endl;
    vil_image_view<bool> mask_src( 32, 32, 1 );
    mask_src.fill( false );

    h.set_translation( 4, 7 );
    vil_image_view<bool> dest( 32, 32, 1 );
    dest.fill( true );

    vil_image_view<bool> unmapped_mask( 32, 32 );

    warp_image_parameters param;
    param.set_unmapped_value( 0 );
    param.set_interpolator( warp_image_parameters::NEAREST );

    warp_image( mask_src, dest, h, param, &unmapped_mask );

    TEST( "sample 1 matches", dest(2,0), false );
    TEST( "sample 2 matches", dest(12,17), false );
    TEST( "sample 3 matches", dest(11,16), false );
    TEST( "sample 4 matches", dest(27,24), false );
    TEST( "sample 5 matches", dest(28,24), false );

    std::cout<< "Unmapped mask" << std::endl;
    TEST( "sample 1 matches", unmapped_mask(2,0), false );
    TEST( "sample 2 matches", unmapped_mask(12,17), false );
    TEST( "sample 3 matches", unmapped_mask(11,16), false );
    TEST( "sample 4 matches", unmapped_mask(27,24), false );
    TEST( "sample 5 matches", unmapped_mask(28,24), true );
  }

  {
    std::cout << "Stability" << std::endl;
    vil_image_view<vxl_byte> dest( 12, 12, 1, 3 );
    const double random_homog[9] = { 0.1, 100, -100, 20.5, 1000, 0.2, 1, 2, 0.1 };
    vgl_h_matrix_2d<double> homog;
    homog.set( random_homog );
    bool caught_exception = false;
    try {
      warp_image( src, dest, homog );
    } catch(...) {
      caught_exception = true;
    }

    TEST( "High Shear Case", caught_exception, false );

    homog.set_identity();
    homog.set_translation( -10000.0, 10000.0 );
    caught_exception = false;
    try {
      warp_image( src, dest, homog );
    } catch(...) {
      caught_exception = true;
    }

    TEST( "Translation Out of Bounds Case", caught_exception, false );
  }

  {
    std::cout << "Return type" << std::endl;

    vil_image_view<vxl_byte> dest( 32, 32, 1, 3 );
    h.set_identity();
    h.set_translation( 31, 0 );
    bool retVal = warp_image( src, dest, h );

    TEST( "Bottom Upper Check", retVal, true );

    h.set_translation( 31.1, 0 );
    retVal = warp_image( src, dest, h );

    TEST( "Bottom Lower Check", retVal, false );

    h.set_translation( -31.1, 0 );
    retVal = warp_image( src, dest, h );

    TEST( "Top Upper Check", retVal, false );

    h.set_translation( -31, 0 );
    retVal = warp_image( src, dest, h );

    TEST( "Top Lower Check", retVal, true );

    std::cout << "Corner mask cases" << std::endl;

    vil_image_view<bool> mask( 32, 32 );
    h.set_identity();
    h.set_translation( 0, -31 );
    retVal = warp_image( src, dest, h, &mask );
    TEST( "Sample 1", mask(12,31), false );
    TEST( "Sample 2", mask(12,30), true );
    TEST( "Sample 3", mask(31,31), false );
  }
}

} // end anonymous namespace

int test_warp_image( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "warp_image" );

  test_simple_warp_image();

  return testlib_test_summary();
}
