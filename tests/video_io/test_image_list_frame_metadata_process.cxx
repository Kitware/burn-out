/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <testlib/testlib_test.h>

#include <video_io/image_list_frame_metadata_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_frame_time_count( std::string const& dir )
{
  std::cout << "\n\nTesting File\n\n";

  image_list_frame_metadata_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "file", dir+"/smallframe_file_meta.txt" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 10 );
  TEST( "Timestamp", src.timestamp().time(), 67 );

  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 11 );
  TEST( "Timestamp", src.timestamp().time(), 75 );

  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 12 );
  TEST( "Timestamp", src.timestamp().time(), 86 );

  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Timestamp", src.timestamp().time(), 94 );

  TEST( "Step 4", src.step(), false );
}

void
test_roi( std::string const& dir )
{
  std::cout << "\n\nTesting ROI\n\n";

  image_list_frame_metadata_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "file", dir+"/smallframe_file_meta.txt" );
  blk.set( "roi", "BOB" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), false );

  blk.set( "roi", "1x1+1+1" );
  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Image ni", src.ni(), 1 );
  TEST( "Image nj", src.nj(), 1 );

  TEST( "Step 0", src.step(), true );
  TEST( "Image Width", src.image().ni(), 1 );
  TEST( "Image Height", src.image().nj(), 1 );
}

void test_list_file_no_not_exist( std::string const & dir )
{
  std::cout << "\n\nTesting when file does not exist\n\n";

  image_list_frame_metadata_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "file", dir + "/DOES_NOT_EXIST.txt" );

  TEST( "Set params", src.set_params( blk ), false );
}

void
test_non_set_file( std::string const& /*file*/ )
{
  std::cout << "\n\nTesting when file is not set\n\n";

  image_list_frame_metadata_process<vxl_byte> src( "src" );

  config_block blk = src.params();

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), false );
}

void
test_file_meta_missmatch( std::string const& dir )
{
  std::cout << "\n\nTesting list file mismatch\n\n";

  image_list_frame_metadata_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "file", dir+"/smallframe_file_meta_missmatch.txt" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), false );
}

void
test_red_river_metadata_reader( std::string const& dir )
{
  std::cout << "\n\nTesting File\n\n";

  image_list_frame_metadata_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "file", dir+"/red_river_list.txt" );
  blk.set( "metadata_format", "red_river" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );
  TEST( "Step 0", src.step(), true );

  video_metadata md = src.metadata();
  TEST( "UTC time", md.timeUTC(), 1334677640480000 );
  TEST( "Platform roll", md.platform_roll(), -3.653 );
  TEST( "Platform pitch", md.platform_pitch(), -29.497 );
  TEST( "Platform yaw", md.platform_yaw(), 163.986 );
  TEST( "Platform latitude", md.platform_location().get_latitude(), 35.082600 );
  TEST( "Platform longitude", md.platform_location().get_longitude(), -106.502034 );
  TEST( "Platform altitude", md.platform_altitude(), (8662.11 - 5664.11) * 0.3048 );
  TEST( "Sensor yaw", md.sensor_yaw(), 1.75 );
  TEST( "Sensor pitch", md.sensor_pitch(), -21.34 );
  TEST( "Sensor roll", md.sensor_roll(), 0.00 );
  TEST( "Frame center latitude", md.frame_center().get_latitude(), 35.067321 );
  TEST( "Frame center longitude", md.frame_center().get_longitude(), -106.497584 );
  TEST( "Step 1", src.step(), true );
  md = src.metadata();
  TEST( "UTC time 1", md.timeUTC(), 1334677641980000 );
  TEST( "Platform roll 1", md.platform_roll(), -3.606 );
  TEST( "Platform pitch 1", md.platform_pitch(), -30.388 );
  TEST( "Platform yaw 1", md.platform_yaw(), 162.383 );
  TEST( "Platform latitude 1", md.platform_location().get_latitude(), 35.082352 );
  TEST( "Platform longitude 1", md.platform_location().get_longitude(), -106.502893 );
  TEST( "Platform altitude 1", md.platform_altitude(), (8666.11 - 5664.11) * 0.3048 );
  TEST( "Sensor yaw 1", md.sensor_yaw(), 1.80 );
  TEST( "Sensor pitch 1", md.sensor_pitch(), -21.79 );
}

} // end anonymous namespace

int test_image_list_frame_metadata_process( int argc, char* argv[] )
{
  if( argc < 3 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "test_image_list_frame_metadata_process" );

  test_frame_time_count( argv[2] );
  test_list_file_no_not_exist( argv[2] );
  test_non_set_file( argv[2] );
  test_roi( argv[2] );
  test_file_meta_missmatch( argv[2] );
  test_red_river_metadata_reader( argv[2] );

  return testlib_test_summary();
}
