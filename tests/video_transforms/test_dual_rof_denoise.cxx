/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <vil/vil_math.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>
#include <vil/vil_convert.h>
#include <testlib/testlib_test.h>

#include <video_transforms/dual_rof_denoise.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

template <typename T>
struct traits
{
  static vcl_string name() {return "<unknown>";}
};

template <>
struct traits<float>
{
  static vcl_string name() {return "<float>";}
};

template <>
struct traits<double>
{
  static vcl_string name() {return "<double>";}
};


// test the R.O.F. denoising function and compare to a saved result.
template <typename T>
void
test_denoising( const vil_image_view<vxl_byte>& input,
                const vil_image_view<vxl_byte>& truth,
                double eps )
{
  vil_image_view<T> float_input, float_output;
  vil_convert_cast(input, float_input);

  dual_rof_denoise(float_input, float_output, 100, T(10.0));

  vil_image_view<vxl_byte> output;
  vil_convert_cast(float_output, output);

  // compute average number of grey levels different
  double diff = vil_math_image_abs_difference(output, truth);
  diff /= truth.ni() * truth.nj();

  // The vil_save call below was use to create the ground truth image
  // uncomment this line if you need to create an updated ground truth image.
  //vil_save(output, "ocean_city_denoised.png");

  TEST_NEAR( ("Denoised image " + traits<T>::name()).c_str(),
             diff, 0.0, eps);
}

// test the weighted R.O.F. denoising function and compare to a saved result.
// The weight at each pixel in the range [0, 1] controls the amount of denoising.
// A weight of 0 means no denoising and a weight of 1 mean full denoising
// (as in the unweighted case).  This test cases generates a weight image
// that is 1 in the center and then drops off with a Gaussian distribution
// to a minimum value of exp(-5) in the corners.
template <typename T>
void
test_weighted_denoising( const vil_image_view<vxl_byte>& input,
                         const vil_image_view<vxl_byte>& truth,
                         double eps )
{
  vil_image_view<T> float_input, float_output;
  vil_convert_cast(input, float_input);

  // construct the Gaussian weight image
  const unsigned ni=input.ni(), nj=input.nj();
  vil_image_view<T> weights(ni, nj, 1);
  T sigma2 = static_cast<T>((ni*ni + nj*nj)/20.0);
  for( unsigned j=0; j<nj; ++j )
  {
    for( unsigned i=0; i<ni; ++i )
    {
      T di = T(i) - ni/2;
      T dj = T(j) - nj/2;
      weights(i,j) = exp((-di*di - dj*dj)/sigma2);
    }
  }

  dual_rof_weighted_denoise(float_input, weights, float_output, 100, T(100.0));

  vil_image_view<vxl_byte> output;
  vil_convert_cast(float_output, output);

  // compute average number of grey levels different
  double diff = vil_math_image_abs_difference(output, truth);
  diff /= ni*nj;

  // The vil_save call below was use to create the ground truth image
  // uncomment this line if you need to create an updated ground truth image.
  //vil_save(output, "ocean_city_weighted_denoised.png");

  TEST_NEAR( ("Weighted denoised image " + traits<T>::name()).c_str(),
             diff, 0.0, eps);
}


} // end anonymous namespace

int test_dual_rof_denoise( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    vcl_cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  // load the input test image and the expected output images
  vcl_string dir = argv[1];
  vcl_string src_path = dir + '/' + "ocean_city.png";
  vcl_string denoise_truth_path = dir + '/' + "ocean_city_denoised.png";
  vcl_string weight_denoise_truth_path = dir + '/' + "ocean_city_weighted_denoised.png";

  vil_image_view<vxl_byte> input = vil_load(src_path.c_str());
  vil_image_view<vxl_byte> denoise_truth = vil_load(denoise_truth_path.c_str());
  vil_image_view<vxl_byte> weight_denoise_truth = vil_load(weight_denoise_truth_path.c_str());


  testlib_test_start( "vidl_dual_rof_denoise" );

  vcl_cout << "\n\nTesting denoising on " << src_path << "\n\n";
  test_denoising<double>(input, denoise_truth, 0.0);
  test_denoising<float>(input, denoise_truth, 1e-5);

  vcl_cout << "\n\nTesting weighed denoising on " << src_path << "\n\n";
  test_weighted_denoising<double>(input, weight_denoise_truth, 0.0);
  test_weighted_denoising<float>(input, weight_denoise_truth, 1e-5);

  return testlib_test_summary();
}
