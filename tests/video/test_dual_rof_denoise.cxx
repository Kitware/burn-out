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

#include <video/dual_rof_denoise.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_denoising( vcl_string const& dir,
                vcl_string const& src_file,
                vcl_string const& truth_file )
{
  vcl_cout << "\n\nTesting denoising on " << src_file << "\n\n";

  vcl_string src_path = dir + '/' + src_file;
  vil_image_view<vxl_byte> input = vil_load(src_path.c_str());
  vcl_string truth_path = dir + '/' + truth_file;
  vil_image_view<vxl_byte> truth = vil_load(truth_path.c_str());

  vil_image_view<double> float_input, float_output;
  vil_convert_cast(input, float_input);

  dual_rof_denoise(float_input, float_output, 100, 10.0);

  vil_image_view<vxl_byte> output;
  vil_convert_cast(float_output, output);
  //vil_save(byte_out, (dir + "/ocean_city_denoised.png").c_str());

  TEST( "Denoised image", vil_image_view_deep_equality(output, truth), true );
}


} // end anonymous namespace

int test_dual_rof_denoise( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    vcl_cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "vidl_dual_rof_denoise" );

  test_denoising( argv[1], "ocean_city.png", "ocean_city_denoised.png" );

  return testlib_test_summary();
}


