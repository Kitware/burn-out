/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <vil/vil_save.h>
#include <testlib/testlib_test.h>
#include <fstream>
#include <ios>

#include <vidl/vidl_config.h>

#if VIDL_HAS_FFMPEG
#include <video_io/vidl_ffmpeg_frame_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_klv_metadata(const std::string &filename, const std::string &metafile)
{
  std::cout << "\n\nTesting klv reading on " << filename << "\n\n";

  std::ifstream infile(metafile.c_str());

  vidl_ffmpeg_frame_process src( "src" );

  config_block blk = src.params();
  blk.set( "filename", filename );
  blk.set( "klv_type", "0104" );
  blk.set( "time_type", "klv" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  src.step();
  src.step();

  vxl_uint_64 ts;
  infile >> ts;

  TEST_NEAR( "Back interpolation of TS", static_cast<double>(ts), src.timestamp().time(), 1 );

  const unsigned int lastframe = 79;
  for (unsigned int i = 0; i < lastframe && src.step(); i++);

  const video_metadata &vm = src.metadata();
  std::string x;
  vxl_uint_64 meta_ts;
  double platform_location_lat, platform_location_lon, platform_alt, platform_roll,
         platform_pitch, platform_yaw, slant_range, sensor_horiz_fov, sensor_vert_fov,
         frame_center_lat, frame_center_lon;

  infile  >> x >> x >> meta_ts >> platform_location_lat
         >> platform_location_lon >> platform_alt >> platform_roll
         >> platform_pitch >> platform_yaw >> x >> x >> x
         >> x >> x >> x >> x >> x >> x >> x >> x >> slant_range >> sensor_horiz_fov
         >> sensor_vert_fov >> frame_center_lat >> frame_center_lon;

  infile.close();

  TEST("time metadata", vm.timeUTC() == meta_ts, true);
  TEST("platform lat", vm.platform_location().get_latitude() == platform_location_lat, true);
  TEST("platform lon", vm.platform_location().get_longitude() == platform_location_lon, true);
  TEST("platform alt", vm.platform_altitude() == platform_alt, true);
  TEST("platform roll", vm.platform_roll() == platform_roll, true);
  TEST("platform pitch", vm.platform_pitch() == platform_pitch, true);
  TEST("platform yaw", vm.platform_yaw() == platform_yaw, true);
  TEST("slant range", vm.slant_range() == slant_range, true);
  TEST("sensor hFOV", vm.sensor_horiz_fov() == sensor_horiz_fov, true);
  TEST("sensor vFOV", vm.sensor_vert_fov() == sensor_vert_fov, true);
  TEST("frm center lat", vm.frame_center().get_latitude() == frame_center_lat, true);
  TEST("frm center lon", vm.frame_center().get_longitude() == frame_center_lon, true);
}


} // end anonymous namespace
#endif

int test_vidl_ffmpeg_frame_process_klv( int /*argc*/, char* argv[] )
{
  testlib_test_start( "vidl_ffmpeg_frame_process_klv" );

#if VIDL_HAS_FFMPEG
  test_klv_metadata( argv[1], argv[2]);

  return testlib_test_summary();
#else
  (void)argv;
  std::cout << "vidl does not have ffmpeg. Not testing video reading"
           << std::endl;
  return EXIT_SUCCESS;
#endif // VIDL_HAS_FFMPEG
}
