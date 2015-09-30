/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>
#include <vil/vil_convert.h>
#include <testlib/testlib_test.h>

#include <video/gaussian_pyramid_builder.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


template<typename T, typename gradT>
void
test_gaussian_pyramid_builder( vcl_string const& dir,
                const vil_image_view<vxl_byte> input,
                const vil_image_view<vxl_byte> truth_gp,
                const vil_image_view<vxl_byte> truth_gradp)
{
  vil_image_view<T> T_input, T_output_gp;
  vil_image_view<gradT> output_gradp;
  vil_convert_cast(input, T_input);
  vil_image_view<vxl_byte> output;

  vcl_vector<vil_image_view<T> > pyramid;
  gaussian_pyramid_builder gpb(4, 2, 1.0);
  gpb.build_pyramid<T>(T_input, pyramid);
  tile_pyramid(pyramid, T_output_gp);
  vil_convert_cast(T_output_gp, output);
  double numpixels = truth_gp.ni() * truth_gp.nj();
  double result = vil_math_ssd<vxl_byte, double>(truth_gp, output, double())/numpixels;
  TEST_NEAR( "Testing Gaussian pyramid", result, 0.0, 1.0);
  //vil_save(output, (dir + "/ocean_city_pyramid_b.png").c_str());

  vcl_vector<vil_image_view<gradT> > gradpyramid;
  gpb.build_pyramid<T, gradT>(T_input, pyramid, gradpyramid);

  tile_pyramid(pyramid, T_output_gp);
  vil_convert_cast(T_output_gp, output);
  result = vil_math_ssd<vxl_byte, double>(truth_gp, output, double())/numpixels;
  TEST_NEAR( "Testing Gaussian pyramid (from gradient)", result, 0.0, 1.0);

  tile_pyramid(gradpyramid, output_gradp);
  vil_convert_stretch_range(output_gradp, output);
  numpixels = truth_gradp.ni()*truth_gradp.nj();
  result = vil_math_ssd<vxl_byte, double>(truth_gradp, output, double())/numpixels;
  TEST_NEAR( "Testing gradient pyramid", result, 0.0, 1.0);
  //vil_save(output, (dir + "/ocean_city_pyramid_grad_b.png").c_str());
}

} // end anonymous namespace

int test_gaussian_pyramid_builder( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    vcl_cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "vidl_gaussian_pyramid" );

  vcl_cout << "\nTesting Gaussian pyramid on ocean_city.png\n";

  vcl_string dir(argv[1]);
  vcl_string src_path = dir + "/ocean_city.png";
  vil_image_view<vxl_byte> input = vil_load(src_path.c_str());
  vcl_string truth_path_gp = dir + "/ocean_city_pyramid.png";
  vil_image_view<vxl_byte> truth_gp = vil_load(truth_path_gp.c_str());
  vcl_string truth_path_gradp = dir + "/ocean_city_pyramid_grad.png";
  vil_image_view<vxl_byte> truth_gradp = vil_load(truth_path_gradp.c_str());

  vcl_cout << "DOUBLE:\n";
  test_gaussian_pyramid_builder<double, double>(dir, input, truth_gp, truth_gradp);
  vcl_cout << "\nVXL_BYTE:\n";
  test_gaussian_pyramid_builder<vxl_byte, double>(dir, input, truth_gp, truth_gradp);

  return testlib_test_summary();
}
