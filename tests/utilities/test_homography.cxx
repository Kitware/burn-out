/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <testlib/testlib_test.h>

#include <utilities/homography_util.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

#define HOMOGS(CALL)                            \
  CALL( image_to_image_homography, i2i )        \
  CALL( image_to_plane_homography, i2p )        \
  CALL( plane_to_image_homography, p2i )        \
  CALL( plane_to_plane_homography, p2p )        \
  CALL( image_to_utm_homography, i2u )          \
  CALL( utm_to_image_homography, u2i )          \
  CALL( plane_to_utm_homography, p2u )          \
  CALL( utm_to_plane_homography, u2p )


// ----------------------------------------------------------------
/*

  @todo: Decide if this function is required. It's currently
         unused and causing a warning. Adding it to the list
         off tests that run, however, fails.

void test_assign()
{

#define H_TEST(T,V)                              \
  { T h, hr;                                    \
  h.set_identity( true );                       \
  hr = h;                                       \
  TEST( # T " assignment", h, hr ); }

  HOMOGS( H_TEST )

#undef H_TEST
}
*/

// ----------------------------------------------------------------
// Test the type system.
void test_types()
{
  // compile time test
#define M_TEST(T,V) T V;
  HOMOGS(M_TEST)

    // i2u = w2u * i2w;
    // s2d = x2d * s2x;
#define M3_MULT(S, X, D) S ## 2 ## D = X ## 2 ## D * S ## 2 ## X;
#define M2_MULT(A, B, C) M3_MULT(A, B, C); M3_MULT(A, C, B);
#define M1_MULT(A, B, C) M2_MULT(A, B, C); M2_MULT(B, C, A); M2_MULT(C, A, B);

M1_MULT(i, p, u)

// This is a success if it compiles

#undef M_TEST
#undef M3_MULT
#undef M2_MULT
#undef M1_MULT
#undef M0_MULT
}


// Test point translation

// Test reference propogation




/*
  image_to_image_homography i2i;
  image_to_plane_homography i2p;
  plane_to_image_homography p2i;
  plane_to_plane_homography p2p;
  image_to_utm_homography   i2u;
  utm_to_image_homography   u2i;
  plane_to_utm_homography   p2u;
  utm_to_plane_homography   u2p;
*/

} // end namespace


// ----------------------------------------------------------------
int test_homography( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "homography_test" );

  test_types();

  return testlib_test_summary();
}
