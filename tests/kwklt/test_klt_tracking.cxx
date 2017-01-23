/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <vcl_iomanip.h>

#include <pipeline_framework/sync_pipeline.h>
#include <utilities/config_block.h>
#include <utilities/timestamp.h>
#include <kwklt/klt_tracking_process.h>
#include <kwklt/klt_pyramid_process.h>

#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_drop_after_find()
{
  sync_pipeline p;

  klt_pyramid_process<vxl_byte> pyr("pyramid");
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
      unsigned char color = 255;
      switch (((i / checker_size) + (j / checker_size)) % 5)
      {
        case 1:
          color = 51;
          break;
        case 2:
          color = 102;
          break;
        case 3:
          color = 153;
          break;
        case 4:
          color = 204;
      }
      img(i, j) = color * (((i / checker_size) + (j / checker_size)) % 2);
    }
  }
  pyr.set_image(img);
  ts.set_frame_number(1);
  trk.set_timestamp(ts);

  TEST("execute", p.execute(), process::SUCCESS);
  TEST_EQUAL("active_tracks", trk.active_tracks().size(), feature_count);
  TEST_EQUAL("created_tracks", trk.created_tracks().size(), feature_count);
  TEST_EQUAL("terminated_tracks", trk.terminated_tracks().size(), 0);

  for (size_t i = 0; i < width; ++i)
  {
    for (size_t j = 0; j < height; ++j)
    {
      unsigned char color = 255;
      switch (((i / checker_size) + (j / checker_size)) % 5)
      {
        case 1:
          color = 51;
          break;
        case 2:
          color = 102;
          break;
        case 3:
          color = 153;
          break;
        case 4:
          color = 204;
      }
      // Invert the checkerboard with a "+ 1"
      img(i, j) = color * (((i / checker_size) + (j / checker_size) + 1) % 2);
    }
  }
  pyr.set_image(img);
  ts.set_frame_number(2);
  trk.set_timestamp(ts);

  TEST("execute", p.execute(), process::SUCCESS);
  TEST_EQUAL("active_tracks", trk.active_tracks().size(), feature_count);
  TEST_EQUAL("created_tracks", trk.created_tracks().size(), feature_count);
  TEST_EQUAL("terminated_tracks", trk.terminated_tracks().size(), feature_count);
}

//real images of fixed translation
void test_real_images( vcl_string const& dir )
{
  sync_pipeline p;

  klt_pyramid_process<vxl_byte> pyr("pyramid");
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
  c.set("tracking:feature_count", "1000");

  TEST("set_params", p.set_params(c), true);
  TEST("initialize", p.initialize(), true);

  vil_image_view<vxl_byte> img;
  timestamp ts;
  //load image 1
  img = vil_load( (dir + "/fix_movement_0.png").c_str() );
  ts.set_frame_number(1);

  pyr.set_image(img);
  trk.set_timestamp(ts);

  TEST("execute", p.execute(), process::SUCCESS);
  TEST_EQUAL("active_tracks", trk.active_tracks().size(), 1000);
  TEST_EQUAL("created_tracks", trk.created_tracks().size(), 1000);
  TEST_EQUAL("terminated_tracks", trk.terminated_tracks().size(), 0);

  {
    vcl_vector<klt_track_ptr> const& trks =  trk.active_tracks();
    for(vcl_vector<klt_track_ptr>::const_iterator iter = trks.begin(); iter != trks.end(); ++iter)
    {
      TEST_EQUAL("track is correct size", (*iter)->size(), 1);
      TEST("Track is starts", (*iter)->is_start(), true);
    }
  }

  img = vil_load( (dir + "/fix_movement_1.png").c_str() );
  ts.set_frame_number(2);

  pyr.set_image(img);
  trk.set_timestamp(ts);

  TEST("execute", p.execute(), process::SUCCESS);
  TEST_EQUAL("Sum of active and terminated is correct", trk.active_tracks().size() + trk.terminated_tracks().size(), 1000);
  TEST_EQUAL("No tracks were created", trk.created_tracks().size(), 0);
  double average_distance = 0.;
  double count = 0.;
  {
    vcl_vector<klt_track_ptr> const& trks =  trk.active_tracks();
    for(vcl_vector<klt_track_ptr>::const_iterator iter = trks.begin(); iter != trks.end(); ++iter)
    {
      TEST_EQUAL("track is correct size", (*iter)->size(), 2);
      TEST("Not start", (*iter)->is_start(), false);
      klt_track::point_t hpt = (*iter)->point();
      klt_track_ptr tail = (*iter)->tail();
      TEST("Tail is not null", tail != NULL, true);
      klt_track::point_t tpt = tail->point();
      average_distance += vcl_sqrt((hpt.x-tpt.x)*(hpt.x-tpt.x) + (hpt.y-tpt.y)*(hpt.y-tpt.y));
      count++;
    }
  }
  TEST_NEAR("Average distance between points is as expected", average_distance/count, 14.0927232874901538, 1e-5);
}

void test_klt_track()
{
  klt_track::point_ pt1;
  pt1.x = 5;
  pt1.y = 7;
  pt1.frame = 10;

  klt_track::point_ pt2;
  pt2.x = 6;
  pt2.y = 8;
  pt2.frame = 11;
  TEST("Test point equal (equal)", pt1 == pt1, true);
  TEST("Test point equal (not equal)", pt1 == pt2, false);
  klt_track_ptr track1 = klt_track::extend_track(pt1);
  klt_track_ptr track2 = klt_track::extend_track(pt2);
  TEST("Track is start", track1->is_start(), true);
  TEST_EQUAL("Track is of correct size", track1->size(), 1);
  TEST("Test track equal 1 state (equal)", *track1 == *track1, true);
  TEST("Test track equal 1 state (not equal)", *track1 == *track2, false);

  klt_track::point_ pt3;
  pt3.x = 22;
  pt3.y = 34;
  pt3.frame = 12;
  klt_track_ptr track1_1 = klt_track::extend_track(pt3,track1);
  klt_track_ptr track2_1 = klt_track::extend_track(pt3,track2);
  TEST("Track is start", track1_1->is_start(), false);
  TEST_EQUAL("Track is of correct size", track1_1->size(), 2);
  TEST("Tail is what is expected", track1_1->tail(), track1);
  TEST("Test track equal 2 states (equal)", *track1_1 == *track1_1, true);
  TEST("Test track equal 2 states (not equal)", *track1_1 == *track2_1, false);
  klt_track_ptr track3 = klt_track::extend_track(pt3);
  TEST("Test track equal difference number of states ", *track1_1 == *track3, false);

  track_sptr trk = convert_from_klt_track(track1_1);
  TEST_EQUAL("Track is of correct size", trk->history().size(), 2);
  TEST_EQUAL("First point is x correct", trk->history()[0]->loc_[0], 5);
  TEST_EQUAL("First point is y correct", trk->history()[0]->loc_[1], 7);
  TEST_EQUAL("Second point is x correct", trk->history()[1]->loc_[0], 22);
  TEST_EQUAL("Second point is y correct", trk->history()[1]->loc_[1], 34);
}

void test_process_error_checking()
{
  {
    klt_tracking_process trk("tracking");
    config_block c = trk.params();
    TEST("set_params defaults work", trk.set_params(c), true);
    c.set("impl", "BAD");
    TEST("set_params set bad impl", trk.set_params(c), false);
    c.set("impl", "klt");
    TEST("set_params works with impl of klt", trk.set_params(c), true);
    c.set("min_feature_count_percent", "1000");
    TEST("set_params checks to make sure min_feature_count_percent is reasonable", trk.set_params(c), false);
    c.set("min_feature_count_percent", "0.8");
    TEST("set_params checks to make sure min_feature_count_percent is reasonable", trk.set_params(c), true);
    c.set("window_width", "10");
    c.set("window_height", "10");
    TEST("set_params test even windows size", trk.set_params(c), true);
    TEST("set_params test even windows size", trk.initialize(), true);
  }
  //test disable
  {
    klt_tracking_process trk2("tracking");
    config_block c = trk2.params();
    c.set("disabled", "BOB");
    TEST("Bad value", trk2.set_params(c), false);
    c.set("disabled", "true");
    TEST("Can set disable", trk2.set_params(c), true);
    TEST("initialize", trk2.initialize(), true);
    TEST("Disable Returns false", trk2.step(), false);
  }
}

} // end namespace

// ----------------------------------------------------------------
/** Main test driver for this file.
 *
 *
 */
int test_klt_tracking(int argc, char * argv[])
{
  testlib_test_start( "kwklt API");

  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  test_drop_after_find();
  test_real_images(argv[1]);
  test_klt_track();
  test_process_error_checking();

  return testlib_test_summary();
}
