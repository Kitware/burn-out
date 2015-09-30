/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <pipeline/sync_pipeline.h>
#include <utilities/config_block.h>
#include <utilities/timestamp.h>
#include <kwklt/klt_tracking_process.h>
#include <kwklt/klt_pyramid_process.h>

#include <vil/vil_image_view.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_drop_after_find()
{
  sync_pipeline p;

  klt_pyramid_process pyr("pyramid");
  p.add(&pyr);

  klt_tracking_process trk("tracking");
  p.add(&trk);

  p.connect(pyr.image_pyramid_port(),
            trk.set_image_pyramid_port());
  p.connect(pyr.image_pyramid_gradx_port(),
            trk.set_image_pyramid_gradx_port());
  p.connect(pyr.image_pyramid_grady_port(),
            trk.set_image_pyramid_grady_port());

  config_block c = p.params();
  c.set("tracking:impl", "klt");
  c.set("tracking:feature_count", "10");

  TEST("set_params", p.set_params(c), true);
  TEST("initialize", p.initialize(), true);

  size_t const checker_size = 10;
  size_t const width = 100;
  size_t const height = 100;
  size_t const feature_count = 10;
  vil_image_view<vxl_byte> img(width, height);
  timestamp ts;

  // Create a checkerboard image.
  for (size_t i = 0; i < width; ++i)
  {
    for (size_t j = 0; j < height; ++j)
    {
      img(i, j) = 255 * (((i / checker_size) + (j / checker_size)) % 2);
    }
  }
  pyr.set_image(img);
  ts.set_frame_number(1);
  trk.set_timestamp(ts);

  TEST("execute", p.execute(), true);
  TEST("active_tracks", trk.active_tracks().size(), feature_count);
  TEST("created_tracks", trk.created_tracks().size(), feature_count);
  TEST("terminated_tracks", trk.terminated_tracks().size(), 0);

  for (size_t i = 0; i < width; ++i)
  {
    for (size_t j = 0; j < height; ++j)
    {
      // Invert the checkerboard with a "+ 1"
      img(i, j) = 255 * (((i / checker_size) + (j / checker_size) + 1) % 2);
    }
  }
  pyr.set_image(img);
  ts.set_frame_number(2);
  trk.set_timestamp(ts);

  TEST("execute", p.execute(), true);
  TEST("active_tracks", trk.active_tracks().size(), feature_count);
  TEST("created_tracks", trk.created_tracks().size(), feature_count);
  TEST("terminated_tracks", trk.terminated_tracks().size(), feature_count);
}

} // end namespace

// ----------------------------------------------------------------
/** Main test driver for this file.
 *
 *
 */
int test_klt_tracking(int /*argc*/, char * /*argv*/[])
{
  testlib_test_start( "kwklt API");

  test_drop_after_find();

  return testlib_test_summary();
}
