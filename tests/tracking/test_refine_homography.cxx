/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_load.h>
#include <vil/vil_convert.h>
#include <vil/vil_crop.h>

#include <vnl/vnl_inverse.h>

#include <testlib/testlib_test.h>

#include <tracking/refine_homography.h>
#include <video_transforms/warp_image.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void test_refine_homog(const vil_image_view<double> &img,
                       vnl_double_3x3 &H)
{
  vil_image_view<double> warped(img.ni(), img.nj());
  vnl_double_3x3 Hinv = vnl_inverse<double>(H);
  warp_image(img, warped, Hinv);

  vnl_double_3x3 est_H;
  est_H.set_identity();

  refine_homography(warped, img, est_H, 4.0, 60, 4);
  est_H /= est_H.frobenius_norm();

  TEST_NEAR( "Homography difference (frobenius)", (est_H - H).frobenius_norm(), 0.0, 0.01);
}

} // end anonymous namespace


int test_refine_homography( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument!\n";
    return EXIT_FAILURE;
  }

  // load the input test image
  std::string dir = argv[1];
  std::string src_path = dir + "/ocean_city.png";
  vil_image_view<vxl_byte> input = vil_load(src_path.c_str());
  vil_image_view<double> img;
  vil_convert_cast<vxl_byte, double>(input, img);
  img.deep_copy(vil_crop(img, 270, 100, 150, 100));

  testlib_test_start( "refine_homography" );

  vnl_double_3x3 H;
  double angle = 0.1;
  H(0,0) = std::cos(angle); H(0,1) = -std::sin(angle); H(0,2) = -5.0;
  H(1,0) = std::sin(angle); H(1,1) = std::cos(angle); H(1,2) = -5.0;
  H(2,0) = 0.0; H(2,1) = 0.0; H(2,2) = 1.0;
  H /= H.frobenius_norm();

  test_refine_homog(img, H);

  return testlib_test_summary();
}
