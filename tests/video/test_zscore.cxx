/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_sstream.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_crop.h>
#include <vil/vil_math.h>
#include <vil/vil_convert.h>
#include <vil/vil_transpose.h>
#include <vil/algo/vil_threshold.h>
#include <vnl/vnl_random.h>
#include <vnl/vnl_erf.h>
#include <testlib/testlib_test.h>

#include <video/zscore_image.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


float add_noise(float val)
{
  static vnl_random rnd(0ul);
  return val + rnd.normal();
}


void create_test_image(vil_image_view<float>& diff_image,
                       vil_image_view<bool>& true_mask)
{
  diff_image.set_size(200, 200, 1);
  diff_image.fill(0.0f);

  // create a few boxes with non zero values
  vil_crop(diff_image, 10, 8, 30, 5).fill(3.0);
  vil_crop(diff_image, 70, 20, 10, 15).fill(-2.0);
  vil_crop(diff_image, 50, 5, 50, 7).fill(-4.0);
  vil_crop(diff_image, 55, 6, 50, 6).fill(3.0);
  vil_crop(diff_image, 100, 10, 150, 3).fill(4.0);
  vil_crop(diff_image, 150, 2, 30, 3).fill(5.0);
  vil_crop(diff_image, 150, 2, 150, 15).fill(-3.0);

  // create a ground truth mask
  vil_convert_cast(diff_image, true_mask);

  // shift, scale, and add additive Gaussian noise
  vil_math_scale_and_offset_values(diff_image, 3.0, 1.5);
  vil_transform(diff_image, add_noise);
}


void img_neq(bool b1, bool &b2)
{
  b2 = (b1 != b2);
}

// count the number of mask pixels that differ
unsigned count_diff_binary(const vil_image_view<bool>& im1,
                           const vil_image_view<bool>& im2)
{
  unsigned num_diff = 0;
  vil_image_view<bool> work;
  work.deep_copy(im2);
  vil_transform2(im1, work, img_neq);
  vil_math_sum(num_diff, work, 0);
  return num_diff;
}


void test_old_vs_new(const vil_image_view<float>& diff, float thresh, unsigned step)
{
  // the new, vil-based method
  vil_image_view<bool> mask1;
  {
    // standardize the diff image in place
    vil_image_view<float> zdiff;
    zdiff.deep_copy(diff);

    zscore_global(zdiff, step);
    vil_threshold_outside(zdiff, mask1, -thresh, thresh);
  }

  // the old method
  vil_image_view<bool> mask2;
  zscore_threshold_above(diff, mask2, thresh, step);

  vcl_stringstream ss;
  ss << "zscore old==new (z-thresh: "<<thresh<<", step: "<<step<<")";
  TEST( ss.str().c_str(), vil_image_view_deep_equality(mask1, mask2), true);
}

void test_global_zscore(const vil_image_view<float>& diff_image,
                        const vil_image_view<bool>& true_mask)
{
  // compare the old method to the new one with several different parameters
  test_old_vs_new(diff_image, 2.5, 1);
  test_old_vs_new(diff_image, 2.5, 2);
  test_old_vs_new(diff_image, 2.0, 1);
  test_old_vs_new(diff_image, 2.0, 2);

  // compare the new method to the ground truth
  float zthresh = 2.5;
  vil_image_view<bool> mask;
  vil_image_view<float> zdiff;
  zdiff.deep_copy(diff_image);
  zscore_global(zdiff);
  vil_threshold_outside(zdiff, mask, -zthresh, zthresh);
  unsigned num_errors = count_diff_binary(true_mask, mask);

  // allow no more than 4 times the expected number of errors given the noise.
  unsigned error_thresh = static_cast<unsigned>(4 * vnl_erfc(zthresh)
                                                * diff_image.ni() * diff_image.nj());
  vcl_cout << "expecting less than " << error_thresh
           << " errors, found " << num_errors << vcl_endl;
  TEST("z-score thresh (new) similar to truth", num_errors < error_thresh, true);

  // compare the old method to the ground truth
  vil_image_view<bool> mask2;
  zscore_threshold_above(diff_image, mask2, zthresh);
  num_errors = count_diff_binary(true_mask, mask2);

  vcl_cout << "expecting less than " << error_thresh
           << " errors, found " << num_errors << vcl_endl;
  TEST("z-score thresh (old) similar to truth", num_errors < error_thresh, true);

#if 0
  // save images for debugging
  vil_image_view<vxl_byte> out_true_mask, out_mask, out_mask2, out_diff;
  vil_convert_stretch_range(true_mask, out_true_mask);
  vil_convert_stretch_range(mask, out_mask);
  vil_convert_stretch_range(mask2, out_mask2);
  vil_convert_stretch_range(diff_image, out_diff);
  vil_save(out_true_mask, "out_true_mask.png");
  vil_save(out_mask, "out_mask_new.png");
  vil_save(out_mask2, "out_mask_old.png");
  vil_save(out_diff, "out_diff.png");
#endif

}


void test_local_zscore(const vil_image_view<float>& diff_image,
                       const vil_image_view<bool>& true_mask)
{
  // when the box is larger than the image, the results should equal the global method
  {
    vil_image_view<float> z_loc_diff, z_global_diff;
    z_loc_diff.deep_copy(diff_image);
    z_global_diff.deep_copy(diff_image);
    unsigned radius = (vcl_max(diff_image.ni(), diff_image.nj()) + 1) / 2;
    zscore_local_box(z_loc_diff, radius);
    zscore_global(z_global_diff, 1);

    TEST("Local z-score with large radius equals global",
         vil_image_view_deep_equality(z_loc_diff, z_global_diff), true);
  }

  // results should be invariant over step sizes
  {
    vil_image_view<float> diff_copy, diff_small, diff_small_copy;
    diff_copy.deep_copy(diff_image);
    diff_small = vil_crop(diff_copy, 10,100, 20,150);
    diff_small_copy.deep_copy(diff_small);
    zscore_local_box(diff_small, 20);
    zscore_local_box(diff_small_copy, 20);
    TEST("Local z-score works with virtual views",
         vil_image_view_deep_equality(diff_small, diff_small_copy), true);
  }

  // results should be invariant over transpose
  {
    vil_image_view<float> diff_tall;
    diff_tall.deep_copy(vil_crop(diff_image, 0,40, 0,diff_image.nj()));
    vil_image_view<float> diff_wide;
    diff_wide.deep_copy(vil_transpose(diff_tall));
    zscore_local_box(diff_tall, 5);
    zscore_local_box(diff_wide, 5);
    double ssd = vil_math_ssd(diff_tall, vil_transpose(diff_wide), double());
    ssd /= diff_tall.ni() * diff_tall.nj();
    TEST_NEAR("Local z-score has same value under transpose", ssd, 0.0, 1e-8 );
  }

  // results should be invariant over transpose
  // when using narrow images with large radius
  // the code uses a special implementation when 2*radius+1 is greater than ni or nj
  {
    vil_image_view<float> diff_tall;
    diff_tall.deep_copy(vil_crop(diff_image, 0,40, 0,diff_image.nj()));
    vil_image_view<float> diff_wide;
    diff_wide.deep_copy(vil_transpose(diff_tall));
    zscore_local_box(diff_tall, 25);
    zscore_local_box(diff_wide, 25);
    TEST("Local z-score has same value under transpose with large radius",
         vil_image_view_deep_equality(diff_tall, vil_transpose(diff_wide)), true);
  }

  // local ROI z-scores should be independent of chosen AOI (except near boundaries)
  {
    vil_image_view<float> diff_crop1, diff_crop2, common_crop1, common_crop2;
    diff_crop1.deep_copy(vil_crop(diff_image, 10,180, 0,150));
    diff_crop2.deep_copy(vil_crop(diff_image, 30,170, 20,170));
    zscore_local_box(diff_crop1, 10);
    zscore_local_box(diff_crop2, 10);
    common_crop1 = vil_crop(diff_crop1, 30,140, 30,110);
    common_crop2 = vil_crop(diff_crop2, 10,140, 10,110);
    double ssd = vil_math_ssd(common_crop1, common_crop2, double());
    ssd /= common_crop1.ni() * common_crop2.nj();
    TEST_NEAR("Local z-score independent of ROI", ssd, 0.0, 1e-8 );
  }

  vil_image_view<float> zdiff;
  zdiff.deep_copy(diff_image);
  zscore_local_box(zdiff, 50);
  vil_image_view<bool> mask;
  float zthresh = 3.0;
  vil_threshold_outside(zdiff, mask, -zthresh, zthresh);

#if 0
  vil_image_view<vxl_byte> out_zdiff, out_mask;
  vil_convert_stretch_range(zdiff, out_zdiff);
  vil_convert_stretch_range(mask, out_mask);
  vil_save(out_zdiff, "out_zdiff.png");
  vil_save(out_mask, "out_mask_local.png");
#endif
}


} // end anonymous namespace

int test_zscore( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "zscore" );

  vil_image_view<float> diff_image;
  vil_image_view<bool> true_mask;
  create_test_image(diff_image, true_mask);

  test_global_zscore(diff_image, true_mask);

  test_local_zscore(diff_image, true_mask);

  return testlib_test_summary();
}
