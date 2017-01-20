/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief Tests for MAP-Tk shot stitching algorithm implementation
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include <testlib/testlib_test.h>

#include <tracking/maptk_shot_stitching_algo.h>

#include <vil/vil_convert.h>
#include <vil/vil_load.h>


namespace  // anonymous
{


enum { IMG_SQUARE, IMG_SQUARE_TRANS_7_4, IMG_SQUARE_TRANS_13_1,
       IMG_2_SQUARE, IMG_2_SQUARE_TRANS_7_4,
       IMG_COW,
       IMG_BLANK }; // must be last


struct GLOBALS
{
  vgl_h_matrix_2d < double > identity;
  vil_image_view < vxl_byte > imgs[IMG_BLANK + 1];
  vil_image_view < bool > mask_img;


  bool init( const std::string& data_dir )
  {
    identity.set_identity();
    std::string img_fns[6] = { "square.png", "square_trans+7+4.png", "square_trans+13-1.png",
                              "square-2.png", "square-2_trans+7+4.png", "k5441-1.jpg"};
    for (unsigned i = 0; i < 6; ++i)
    {
      std::string full_path = data_dir + "/" + img_fns[i];
      imgs[i] = vil_load( full_path.c_str() );
      if ( ! imgs[i] )
      {
        std::ostringstream oss;
        oss << "Failed to load '" << full_path << "'";
        TEST( oss.str().c_str(), false, true );
        return false;
      }
    }

    std::string full_path = data_dir + "/ssa_mask.png";
    mask_img = vil_convert_cast( bool(), vil_load( full_path.c_str() ) );
    if ( ! mask_img )
      {
        std::ostringstream oss;
        oss << "Failed to load '" << full_path << "'";
        TEST( oss.str().c_str(), false, true );
        return false;
      }

    // calculate blank image.
    imgs[IMG_BLANK] = vil_image_view< vxl_byte >( 128, 128, 3 );
    imgs[IMG_BLANK].fill( 0 );
    return true;
  }
} G;


bool
matrices_match(const vgl_h_matrix_2d< double >&  a,
               const vgl_h_matrix_2d< double >&  b,
               double fuzz = 1.0e-05)
{
  for ( unsigned r = 0; r < 3; ++r )
  {
    for ( unsigned c = 0; c < 3; ++c )
    {
      double atmp = ( a.get(2, 2) == 0 ) ? 0 : a.get(r, c) / a.get(2, 2);
      double btmp = ( b.get(2, 2) == 0 ) ? 0 : b.get(r, c) / b.get(2, 2);
      if ( fabs(atmp - btmp) > fuzz )
      {
        return false;
      }
    }
  }
  return true;
}


void
test_blank_image()
{
  std::cerr << "[[ test_blank_image ]]" << std::endl;

  vidtk::maptk_shot_stitching_algo< vxl_byte > stitcher;
  vil_image_view< bool > mask;
  vidtk::image_to_image_homography h = stitcher.stitch(G.imgs[IMG_BLANK], mask,
                                                       G.imgs[IMG_BLANK], mask);
  TEST("Blank image returns invalid homography", h.is_valid(), false);
}


void
test_identity_image()
{
  std::cerr << "[[ test_identity_image ]]" << std::endl;

  vidtk::maptk_shot_stitching_algo<vxl_byte> stitcher;
  vil_image_view < bool > mask;

  vidtk::config_block blk = stitcher.params();
  blk.set_value("min_inliers", 4);
  TEST( "stitcher.set_params successful", stitcher.set_params( blk ), true );

  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_SQUARE], mask,
                                                        G.imgs[IMG_SQUARE], mask );
  std::cerr << "H: " << h << std::endl;

  TEST( "Same-image returns valid homography", h.is_valid(), true );
  TEST( "Same-image returns identity", matrices_match( h.get_transform(), G.identity ), true );
}


void
test_identity_image_2()
{
  std::cerr << "[[ test_identity_image_2 ]]" << std::endl;

  vidtk::maptk_shot_stitching_algo<vxl_byte> stitcher;
  vil_image_view < bool > mask;

  vidtk::config_block blk = stitcher.params();
  TEST( "stitcher.set_params successful", stitcher.set_params( blk ), true );

  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_COW], mask,
                                                        G.imgs[IMG_COW], mask );
  std::cerr << "H: " << h << std::endl;

  TEST( "Same-image returns valid homography", h.is_valid(), true );
  // Non-determinism due to the use of RANSAC forces me to make the epsilon a
  // little higher consistent passing.
  TEST( "Same-image returns identity", matrices_match( h.get_transform(), G.identity, 1.0e-3 ), true );
  // TODO: A better test for this more "real-world" example would be to apply
  // the 4 corners to the generated homography and measure mean-square error
  // of where they end up.
}


void test_homography_failure()
{
  std::cerr << "[[ test_homography_failure ]]" << std::endl;

  vidtk::maptk_shot_stitching_algo<vxl_byte> stitcher;
  vil_image_view < bool > mask;

  vidtk::config_block blk = stitcher.params();
  blk.set_value("min_inliers", 4);
  TEST( "stitcher.set_params successful", stitcher.set_params( blk ), true );

  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_SQUARE], mask,
                                                        G.imgs[IMG_BLANK], mask );
  TEST( "KLT tracking failure returns invalid homography", h.is_valid(), false );
}


void
test_translation()
{
  std::cerr << "[[ test_translation ]]" << std::endl;

  vidtk::maptk_shot_stitching_algo<vxl_byte> stitcher;
  vil_image_view < bool > mask;

  vidtk::config_block blk = stitcher.params();
  blk.set_value("min_inliers", 4);
  TEST( "stitcher.set_params successful", stitcher.set_params( blk ), true );

  vidtk::image_to_image_homography h = stitcher.stitch( G.imgs[IMG_SQUARE], mask,
                                                        G.imgs[IMG_SQUARE_TRANS_7_4], mask );

  {
    vgl_h_matrix_2d<double> truth;
    truth.set_identity();
    truth.set_translation( 7.0, 4.0 );
    TEST( "Translation test #1 returns (roughly) +7+4", matrices_match( h.get_transform(), truth, 1.0), true );
  }

  h = stitcher.stitch( G.imgs[IMG_SQUARE_TRANS_7_4], mask,
                       G.imgs[IMG_SQUARE], mask );
  {
    vgl_h_matrix_2d<double> truth;
    truth.set_identity();
    truth.set_translation( -7.0, -4.0 );
    TEST( "Translation test #2 returns (roughly) -7-4", matrices_match( h.get_transform(), truth, 1.0), true );
  }

  h = stitcher.stitch( G.imgs[IMG_SQUARE], mask,
                       G.imgs[IMG_SQUARE_TRANS_13_1], mask );
  {
    vgl_h_matrix_2d<double> truth;
    truth.set_identity();
    truth.set_translation( 13.0, -1.0 );
    TEST( "Translation test #3 returns (roughly) +13-1", matrices_match( h.get_transform(), truth, 1.0), true );
  }

}


void test_masked_translation()
{
  std::cerr << "[[ test_masked_translation ]]" << std::endl;

  vidtk::maptk_shot_stitching_algo<vxl_byte> stitcher;
  vil_image_view < bool > mask;

  vidtk::config_block blk_dflt = stitcher.params();
  blk_dflt.set_value("min_inliers", 4);

  // TODO: Additional tests for ORB fall-back
  //vidtk::config_block blk_orb = stitcher.params();
  //blk_orb.set_value("min_inliers", 4);
  //blk_orb.set_value("feature_detector:ocv:detector:type", "ORB");
  //blk_orb.set_value("descriptor_extractor:ocv:extractor:type", "ORB");

  vidtk::image_to_image_homography h;

  vgl_h_matrix_2d<double> truth;
  truth.set_identity();
  truth.set_translation( 7.0, 4.0 );

  { // default parameters, without mask
    TEST( "stitcher.set_params(DEFAULT) successful", stitcher.set_params( blk_dflt ), true );
    h = stitcher.stitch( G.imgs[IMG_2_SQUARE], mask,
                         G.imgs[IMG_2_SQUARE_TRANS_7_4], mask );
    TEST( "No-mask test DOES NOT return (roughly) +7+4", matrices_match( h.get_transform(), truth, 1.0 ), false );
  }

  { // default parameters, with mask
    TEST( "stitcher.set_params(DEFAULT) successful", stitcher.set_params( blk_dflt ), true );
    h = stitcher.stitch( G.imgs[IMG_2_SQUARE], G.mask_img,
                         G.imgs[IMG_2_SQUARE_TRANS_7_4], G.mask_img );
    TEST( "Mask test return (roughly) +7+4", matrices_match( h.get_transform(), truth, 1.0), true );
  }
}


}


int test_maptk_shot_stitching_algo( int argc, char* argv[] )
{
  testlib_test_start( "maptk shot stitching algo" );

  if ( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    bool data_ready = G.init( argv[1] );
    TEST( "Data loaded", data_ready, true );
    if ( data_ready )
    {
      test_blank_image();
      test_identity_image();
      test_identity_image_2();
      test_homography_failure();
      test_translation();
      test_masked_translation();
    }
  }

  return testlib_test_summary();
}
