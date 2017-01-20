/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <iostream>
#include <sstream>
#include <vil/vil_image_view.h>

#include <utilities/tagged_data_reader_process.h>
#include <utilities/tagged_data_writer_process.h>
#include <utilities/video_metadata_util.h>

namespace
{

void test_disable()
{
  #define TEST_DISABLE(T) \
  {\
    vidtk::T t(#T); \
    vidtk::config_block blk = t.params(); \
    blk.set("enabled", "false"); \
    TEST("can set parameters", t.set_params(blk), true); \
    TEST("can init", t.initialize(), true); \
    TEST("can step", t.step(), true);\
  }

  TEST_DISABLE(tagged_data_reader_process)
  TEST_DISABLE(tagged_data_writer_process)
}

void test_config_errors()
{
  #define TEST_BAD_CONFIG(T) \
  {\
    vidtk::T t(#T); \
    vidtk::config_block blk = t.params(); \
    blk.set("enabled", "BOB"); \
    TEST("Return error when setting parameters", t.set_params(blk), false); \
  }

  TEST_BAD_CONFIG(tagged_data_reader_process)
  TEST_BAD_CONFIG(tagged_data_writer_process)

  #define TEST_BAD_FILE_NAME(T) \
  {\
    vidtk::T t(#T); \
    vidtk::config_block blk = t.params(); \
    blk.set("enabled", "true"); \
    blk.set("filename", "DOES_NOT_EXIST/DOES_NOT_EXIST.tag"); \
    TEST("can set parameters", t.set_params(blk), true); \
    TEST("can not init", t.initialize(), false); \
    TEST("can not step", t.step(), false);\
  }

  TEST_BAD_FILE_NAME(tagged_data_reader_process)
  TEST_BAD_FILE_NAME(tagged_data_writer_process)
}

void test_writing()
{
  vidtk::timestamp time(200,10);
  vidtk::timestamp time2(205,15);
  vidtk::video_metadata vm, vm2;
  std::stringstream test_meta("1256157138146 444 444 0 0 0 0 0 0 0 39.785857850005371006 -84.100535666192243411 "
  "39.785857475525553184 -84.096862203685418535 39.781291508076066066 "
  "-84.096862213843294853 39.781291882545168903 -84.100535676337230484 0 0 0 39.783573343260144384 -84.098697844090594344 ");
  TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  double gsd = 5, gsd2 = 6;
  vidtk::tagged_data_writer_process tw("writer");
  vidtk::config_block blk = tw.params();
  blk.set("enabled", "true");
  blk.set("filename", "test_file.tag");
  blk.set("timestamp", "true");
  TEST("set parameters should log error still returns true", tw.set_params(blk), true);
  tw.set_timestamp_connected(true);
  tw.set_force_video_metadata_enabled();
  tw.set_force_world_units_per_pixel_enabled();
  TEST("set parameters, when forced, does not check connectivity", tw.set_params(blk), true);
  tw.set_video_metadata_connected(true);
  tw.set_world_units_per_pixel_connected(true);
  TEST("can set parameters", tw.set_params(blk), true);
  TEST("can init", tw.initialize(), true);
  tw.set_input_timestamp(time);
  tw.set_input_video_metadata(vm);
  tw.set_input_world_units_per_pixel(gsd);
  TEST("can step", tw.step(), true);
  tw.set_input_timestamp(time2);
  tw.set_input_video_metadata(vm2);
  tw.set_input_world_units_per_pixel(gsd2);
  TEST("can step", tw.step(), true);
  //NOTE: tagged_data_writer_process does not clear out input
}

void comp_vm(std::string const& s, vidtk::video_metadata vm1, vidtk::video_metadata vm2)
{
#define TEST_EQUAL_FIELD(X) TEST_EQUAL((s + " " + #X).c_str(), vm1.X, vm2.X)
#define TEST_NEAR_FIELD(X) TEST_NEAR((s + " " + #X).c_str(), vm1.X, vm2.X, 1e-8)
#define TEST_GEO_NEAR_FIELD(X) \
  TEST_NEAR((s + " " + #X).c_str(), vm1.X.get_latitude(), vm2.X.get_latitude(), 1e-8); \
  TEST_NEAR((s + " " + #X).c_str(), vm1.X.get_longitude(), vm2.X.get_longitude(), 1e-8);
  TEST_EQUAL_FIELD ( is_valid() );

  // If they have the same valid state and one is invalid, then both
  // are invalid. Treat as equal.
  if (! vm1.is_valid() || ! vm2.is_valid()) return;

  TEST_EQUAL_FIELD ( timeUTC() );
  TEST_GEO_NEAR_FIELD ( platform_location() );
  TEST_NEAR_FIELD ( platform_altitude() );

  TEST_NEAR_FIELD ( platform_roll() );
  TEST_NEAR_FIELD ( platform_pitch() );
  TEST_NEAR_FIELD ( platform_yaw() );

  TEST_NEAR_FIELD ( sensor_roll() );
  TEST_NEAR_FIELD ( sensor_pitch() );
  TEST_NEAR_FIELD ( sensor_yaw() );

  TEST_GEO_NEAR_FIELD ( corner_ul() );
  TEST_GEO_NEAR_FIELD ( corner_ur() );
  TEST_GEO_NEAR_FIELD ( corner_ll() );
  TEST_GEO_NEAR_FIELD ( corner_lr() );

  TEST_NEAR_FIELD ( slant_range() );
  TEST_NEAR_FIELD ( sensor_horiz_fov() );
  TEST_NEAR_FIELD ( sensor_vert_fov() );

  TEST_GEO_NEAR_FIELD ( frame_center() );
}

void test_reading()
{
  vidtk::timestamp time(200,10);
  vidtk::timestamp time2(205,15);
  vidtk::video_metadata vm, vm2;
  std::stringstream test_meta("1256157138146 444 444 0 0 0 0 0 0 0 39.785857850005371006 -84.100535666192243411 "
  "39.785857475525553184 -84.096862203685418535 39.781291508076066066 "
  "-84.096862213843294853 39.781291882545168903 -84.100535676337230484 0 0 0 39.783573343260144384 -84.098697844090594344 ");
  TEST("Can decode string", ascii_deserialize(test_meta, vm).good(), true);
  double gsd = 5, gsd2 = 6;
  vidtk::tagged_data_reader_process tr("reader");
  vidtk::config_block blk = tr.params();
  blk.set("enabled", "true");
  blk.set("filename", "test_file.tag");
  TEST("can set parameters", tr.set_params(blk), true);
  TEST("can init", tr.initialize(), true);
  TEST("can step", tr.step(), true);
  TEST("timestamp", tr.get_output_timestamp(), time);
  comp_vm("video_metadata", tr.get_output_video_metadata(), vm);
  TEST_EQUAL("gsd", tr.get_output_world_units_per_pixel(), gsd);
  TEST("can step", tr.step(), true);
  TEST("timestamp", tr.get_output_timestamp(), time2);
  comp_vm("video_metadata", tr.get_output_video_metadata(), vm2);
  TEST_EQUAL("gsd", tr.get_output_world_units_per_pixel(), gsd2);
  TEST("can step", tr.step(), true);
  TEST("timestamp", tr.get_output_timestamp().is_valid(), false);
  TEST("can step", tr.step(), false);
}

}//namespace annon

int test_tag_reader_writer_process (int /*argc*/, char * /*argv*/[])
{
  test_disable();
  test_config_errors();

  //test reading and writing
  test_writing();
  test_reading();
  return testlib_test_summary();
}
