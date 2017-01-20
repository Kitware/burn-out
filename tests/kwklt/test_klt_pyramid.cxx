/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <kwklt/klt_pyramid_process.h>
#include <kwklt/klt_util.h>

#include <klt/klt.h>

#include <vil/vil_image_view.h>

#include <vcl_sstream.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_pyramid_creation()
{
  int const width = 720;
  int const height = 480;
  int const levels = 4;
  int const subsampling = 4;
  float const sigma = 0.9;
  float const init_sigma = 0.7;

  vil_image_view<vxl_byte> vil_img(width, height);

  _KLT_Pyramid klt_pyr = _KLTCreatePyramid(width, height, subsampling, levels);

  vil_pyramid_image_view<float> vil_pyr = vidtk::create_klt_pyramid(vil_img, levels, subsampling, sigma, init_sigma);

  _KLT_Pyramid conv_pyr = vidtk::klt_pyramid_convert(vil_pyr);

  TEST_EQUAL("subsampling", klt_pyr->subsampling, conv_pyr->subsampling);
  TEST_EQUAL("levels", klt_pyr->nLevels, conv_pyr->nLevels);

  for (int i = 0; i < levels; ++i)
  {
    vcl_stringstream sstr;

    sstr << "cols in level " << i;

    TEST_EQUAL(sstr.str().c_str(), klt_pyr->ncols[i], conv_pyr->ncols[i]);
    sstr.str("");

    sstr << "rows in level " << i;

    TEST_EQUAL(sstr.str().c_str(), klt_pyr->nrows[i], conv_pyr->nrows[i]);
    sstr.str("");

    sstr << "cols in image " << i;

    TEST_EQUAL(sstr.str().c_str(), klt_pyr->img[i]->ncols, conv_pyr->img[i]->ncols);
    sstr.str("");

    sstr << "rows in image " << i;

    TEST_EQUAL(sstr.str().c_str(), klt_pyr->img[i]->nrows, conv_pyr->img[i]->nrows);
  }

  _KLTFreePyramid(conv_pyr);
  _KLTFreePyramid(klt_pyr);
}

template<class PIXEL>
void
test_klt_pyramid_process()
{
  //Test disable
  {
    klt_pyramid_process<PIXEL> kpp("pyramid");
    config_block c = kpp.params();
    c.set("disabled", "true");
    TEST("Set disable", kpp.set_params(c), true);
    TEST("can init when disabled", kpp.initialize(), true);
    TEST("can step when disabled", kpp.step(), true);
  }
  //Test bad config values
  {
    klt_pyramid_process<PIXEL> kpp("pyramid");
    config_block c = kpp.params();
    c.set("disabled", "BOB");
    TEST("Set bad value", kpp.set_params(c), false);
  }
  //Test no image passed
  {
    klt_pyramid_process<PIXEL> kpp("pyramid");
    config_block c = kpp.params();
    TEST("Set params, no image", kpp.set_params(c), true);
    TEST("can init, no image", kpp.initialize(), true);
    TEST("can step, no image", kpp.step(), false);
  }
  //Test no image passed
  {
    vil_image_view<PIXEL> vil_img(720, 480);
    klt_pyramid_process<PIXEL> kpp("pyramid");
    config_block c = kpp.params();
    TEST("Set params, with image", kpp.set_params(c), true);
    TEST("can init, with image", kpp.initialize(), true);
    kpp.set_image(vil_img);
    TEST("can step, with image", kpp.step(), true);
    vil_pyramid_image_view<float> pyr = kpp.image_pyramid();
    vil_pyramid_image_view<float> gx = kpp.image_pyramid_gradx();
    vil_pyramid_image_view<float> gy = kpp.image_pyramid_grady();
#define TEST_PYRAMID(X) \
    TEST_EQUAL("correct number of levels", X.nlevels(), 2); \
    TEST_EQUAL("Level 0 correct width", X(0u).ni(), 720); \
    TEST_EQUAL("Level 0 correct height", X(0u).nj(), 480); \
    TEST_EQUAL("Level 1 correct width", X(1u).ni(), 180); \
    TEST_EQUAL("Level 1 correct height", X(1u).nj(), 120)

    TEST_PYRAMID(pyr);
    TEST_PYRAMID(gx);
    TEST_PYRAMID(gy);
  }
}

} // end namespace

// ----------------------------------------------------------------
/** Main test driver for this file.
 *
 *
 */
int test_klt_pyramid(int /*argc*/, char * /*argv*/[])
{
  testlib_test_start( "kwklt API");

  test_pyramid_creation();
  test_klt_pyramid_process<vxl_byte>();
  test_klt_pyramid_process<vxl_uint_16>();

  return testlib_test_summary();
}
