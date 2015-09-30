/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_sstream.h>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>

#include <tracking/shot_stitching_algo.h>

#include <utilities/homography.h>

#include <testlib/testlib_test.h>

namespace { // anon

enum {IMG_SQUARE, IMG_SQUARE_TRANS_7_4, IMG_SQUARE_TRANS_13_1, IMG_BLANK };
struct GLOBALS
{
  vgl_h_matrix_2d<double> identity;
  vil_image_view<vxl_byte> imgs[4];
  bool init( const vcl_string& data_dir )
  {
    identity.set_identity();
    vcl_string img_fns[3] = { "square.png", "square_trans+7+4.png", "square_trans+13-1.png" };
    for (unsigned i=0; i<3; ++i)
    {
      vcl_string full_path = data_dir + "/" + img_fns[i];
      imgs[i] = vil_load( full_path.c_str() );
      if ( ! imgs[i] )
      {
        vcl_ostringstream oss;
        oss << "Failed to load '" << full_path << "'";
        TEST( oss.str().c_str(), false, true );
        return false;
      }
    }
    imgs[IMG_BLANK] = vil_image_view<vxl_byte>( 128, 128, 3 );
    imgs[IMG_BLANK].fill( 0 );
    return true;
  }
} G;

bool
matrices_match( const vgl_h_matrix_2d<double>& a,
                const vgl_h_matrix_2d<double>& b,
                double fuzz = 1.0e-05 )
{
  for (unsigned r=0; r<3; ++r)
  {
    for (unsigned c=0; c<3; ++c)
    {
      double atmp = (a.get(2,2) == 0) ? 0 : a.get(r,c) / a.get(2,2);
      double btmp = (b.get(2,2) == 0) ? 0 : b.get(r,c) / b.get(2,2);
      if (vcl_abs(atmp - btmp) > fuzz) return false;
    }
  }
  return true;
}

void
test_blank_image()
{
  vidtk::shot_stitching_algo<vxl_byte> stitcher;
  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_BLANK], G.imgs[IMG_BLANK] );
  TEST( "Blank image returns invalid homography", h.is_valid(), false );
}

void
test_identity_image()
{
  vidtk::shot_stitching_algo<vxl_byte> stitcher;
  vidtk::config_block blk = stitcher.params();
  blk.set( "ssp_klt_tracking:feature_count", "10" );
  stitcher.set_params( blk );
  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_SQUARE], G.imgs[IMG_SQUARE] );
  TEST( "Same-image returns valid homography", h.is_valid(), true );
  TEST( "Same-image returns identity", matrices_match( h.get_transform(), G.identity ), true );
}

void
test_translation()
{
  vidtk::shot_stitching_algo<vxl_byte> stitcher;
  vidtk::config_block blk = stitcher.params();
  blk.set( "ssp_klt_tracking:feature_count", "10" );
  stitcher.set_params( blk );
  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_SQUARE], G.imgs[IMG_SQUARE_TRANS_7_4] );
  {
    vgl_h_matrix_2d<double> truth;
    truth.set_identity();
    truth.set_translation( 7.0, 4.0 );
    TEST( "Translation test #1 returns (roughly) +7+4", matrices_match( h.get_transform(), truth, 1.0), true );
  }
  h = stitcher.stitch( G.imgs[IMG_SQUARE_TRANS_7_4], G.imgs[IMG_SQUARE] );
  {
    vgl_h_matrix_2d<double> truth;
    truth.set_identity();
    truth.set_translation( -7.0, -4.0 );
    TEST( "Translation test #2 returns (roughly) -7-4", matrices_match( h.get_transform(), truth, 1.0), true );
  }
  h = stitcher.stitch( G.imgs[IMG_SQUARE], G.imgs[IMG_SQUARE_TRANS_13_1] );
  {
    vgl_h_matrix_2d<double> truth;
    truth.set_identity();
    truth.set_translation( 13.0, -1.0 );
    TEST( "Translation test #3 returns (roughly) +13-1", matrices_match( h.get_transform(), truth, 1.0), true );
  }

}

void
test_homography_failure()
{
  vidtk::shot_stitching_algo<vxl_byte> stitcher;
  vidtk::config_block blk = stitcher.params();
  blk.set( "ssp_klt_tracking:feature_count", "10" );
  stitcher.set_params( blk );
  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_SQUARE], G.imgs[IMG_BLANK] );
  TEST( "KLT tracking failure returns invalid homography", h.is_valid(), false );
}

} // anon namespace

int test_shot_stitching_algo( int argc, char* argv[] )
{
  testlib_test_start( "shot stitching algo" );

  if ( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    bool data_ready = G.init( argv[1] );
    TEST( "Data loaded", data_ready, true );
    if (data_ready)
    {
      test_blank_image();
      test_identity_image();
      test_translation();
      test_homography_failure();
    }
  }

  return testlib_test_summary();
}
