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

#include <video_io/frame_metadata_decoder_process.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_frame_time_count_1( std::string const& dir )
{
  std::cout << "\n\nTesting meta format 1 (relative path)\n\n";
  // process should find the metadata files with a path relative to the image
  // directory.

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "img_frame_regex", ".*smallframe([0-9]+)_[0-9]+.[0-9].pgm" );
  blk.set( "metadata_location", "./" );
  blk.set( "metadata_filename_format", "smallframe%03d_.*\\.meta" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  src.set_input_image_file( dir + "/smallframe010_06.7.pgm" );
  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 10 );
  TEST( "Timestamp", src.timestamp().time(), 67 );

  src.set_input_image_file( dir + "/smallframe011_07.5.pgm" );
  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 11 );
  TEST( "Timestamp", src.timestamp().time(), 75 );

  src.set_input_image_file( dir + "/smallframe012_08.6.pgm" );
  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 12 );
  TEST( "Timestamp", src.timestamp().time(), 86 );

  src.set_input_image_file( dir + "/smallframe013_09.4.pgm" );
  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Timestamp", src.timestamp().time(), 94 );

  TEST( "Step 4", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Timestamp", src.timestamp().time(), 94 );
}

void
test_frame_time_count_2( std::string const& dir )
{
  std::cout << "\n\nTesting meta format 2 (relative path w/ dir)\n\n";
  // another relative test, testing use of relative directories also

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "img_frame_regex", ".*smallframe([0-9]+)_[0-9]+.[0-9].pgm" );
  blk.set( "metadata_location", "fr_meta_decoder_test" );
  blk.set( "metadata_filename_format", "smallframe%03d_.*\\.meta" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  src.set_input_image_file( dir + "/smallframe010_06.7.pgm" );
  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 10 );
  TEST( "Timestamp", src.timestamp().time(), 67 );

  src.set_input_image_file( dir + "/smallframe011_07.5.pgm" );
  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 11 );
  TEST( "Timestamp", src.timestamp().time(), 75 );

  src.set_input_image_file( dir + "/smallframe012_08.6.pgm" );
  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 12 );
  TEST( "Timestamp", src.timestamp().time(), 86 );

  src.set_input_image_file( dir + "/smallframe013_09.4.pgm" );
  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Timestamp", src.timestamp().time(), 94 );

  TEST( "Step 4", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Timestamp", src.timestamp().time(), 94 );
}

void
test_frame_time_count_3( std::string const& dir )
{
  std::cout << "\n\nTesting meta format 3 (abs path)\n\n";
  // process should find metadata given a directory that is specified with an
  // absolute path.

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "img_frame_regex", ".*smallframe([0-9]+)_[0-9]+.[0-9].pgm" );
  blk.set( "metadata_location", dir + "/fr_meta_decoder_test" );
  blk.set( "metadata_filename_format", "smallframe%03d_.*\\.meta" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  src.set_input_image_file( dir + "/smallframe010_06.7.pgm" );
  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 10 );
  TEST( "Timestamp", src.timestamp().time(), 67 );

  src.set_input_image_file( dir + "/smallframe011_07.5.pgm" );
  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 11 );
  TEST( "Timestamp", src.timestamp().time(), 75 );

  src.set_input_image_file( dir + "/smallframe012_08.6.pgm" );
  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 12 );
  TEST( "Timestamp", src.timestamp().time(), 86 );

  src.set_input_image_file( dir + "/smallframe013_09.4.pgm" );
  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Timestamp", src.timestamp().time(), 94 );

  TEST( "Step 4", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Timestamp", src.timestamp().time(), 94 );
}

void
test_roi( std::string const& dir )
{
  std::cout << "\n\nTesting ROI\n\n";
  // testing that setting the ROI actually does what we want it to

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "img_frame_regex", ".*smallframe([0-9]+)_.*\\.pgm" );
  blk.set( "metadata_location", "./" );
  blk.set( "metadata_filename_format", "smallframe%03d_.*\\.meta" );
  blk.set( "roi", "BOB" );

  TEST( "Set params", src.set_params( blk ), true );
  // ROI is invalid, this should fail
  TEST( "Initialize", src.initialize(), false );

  blk.set( "roi", "1x2+0+1" );
  TEST( "Set params", src.set_params( blk ), true );
  // valid ROI should cause this to pass
  TEST( "Initialize", src.initialize(), true );

  // Components of ROI string should have been properly set
  TEST( "Image ni", src.ni(), 1 );
  TEST( "Image nj", src.nj(), 2 );

  src.set_input_image_file( dir + "/smallframe010_06.7.pgm" );
  TEST( "Step 0", src.step(), true );
  TEST( "Image Width", src.image().ni(), 1 );
  TEST( "Image Height", src.image().nj(), 2 );
}

void
test_non_set_port()
{
  std::cout << "\n\nTesting when input is not set\n\n";

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  // with no input image set on the port, this should fail
  TEST( "Step 0", src.step(), false );
}

void
test_file_meta_missmatch( std::string const& dir )
{
  frame_metadata_decoder_process<vxl_byte> src( "src" );
  // Test error handling when not finding a matching metadata file for a given
  // source image file.

  config_block blk = src.params();
  blk.set( "img_frame_regex", ".*smallframe([0-9]+)_.*\\.pgm" );
  blk.set( "metadata_location", "fr_meta_decoder_test_mismatch" );
  blk.set( "metadata_filename_format", "smallframe%03d_.*\\.meta" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  // metadata file is located in configured directory, should find it
  src.set_input_image_file( dir + "/smallframe011_07.5.pgm" );
  TEST( "Step 0", src.step(), true );

  // correct matching metadata file not in configured directory, should fail`
  src.set_input_image_file( dir + "/smallframe013_09.4.pgm" );
  TEST( "Step 1", src.step(), false );
}

void
test_red_river_metadata_reader( std::string const& dir )
{
  std::cout << "\n\nTesting red river metadata parsing\n\n";
  // testing that metadata parsing happened correctly

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "img_frame_regex", "red_river_test0_([0-9]+).jpg" );
  blk.set( "metadata_location", "./" );
  blk.set( "metadata_filename_format", "red_river_Meta_%06d.txt" );
  blk.set( "metadata_format", "red_river" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  src.set_input_image_file( dir + "/red_river_test0_000000.jpg" );
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

  src.set_input_image_file( dir + "/red_river_test0_000001.jpg" );
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

void test_incorrect_md_format()
{
  std::cout << "\n\nTesting incorrect metadata format\n\n";
  // testing specifying an incorrect metadata type is handled correctly

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "img_frame_regex", ".*smallframe([0-9]+)_[0-9]+.[0-9].pgm" );
  blk.set( "metadata_location", "./" );
  blk.set( "metadata_filename_format", "smallframe%03d_.*\\.meta" );
  blk.set( "metadata_format", "not_a_matching_type" );

  TEST( "Set params", src.set_params( blk ), false );
}

void test_incorrect_red_river_fov()
{
  std::cout << "\n\nTesting incorrect red_river hfov parameter\n\n";
  // testing error catch for an incorrect hfov parameter in red_river mode and
  // not in klv mode.

  frame_metadata_decoder_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "img_frame_regex", ".*smallframe([0-9]+)_[0-9]+.[0-9].pgm" );
  blk.set( "metadata_location", "./" );
  blk.set( "metadata_filename_format", "smallframe%03d_.*\\.meta" );

  //// Test incorrect H_FOV
  // testing below threshold
  blk.set( "red_river_aspect_box", "-30 1 1");

  // should pass because we're not in red_river mode
  blk.set( "metadata_format", "klv" );
  TEST( "Set params - klv", src.set_params( blk ), true );

  // Should fail with format being red river now
  blk.set( "metadata_format", "red_river" );
  TEST( "Set params - red river", src.set_params( blk ), false );

   //*** The test for above horizontal threshold is no longer valid given the new
  // calculations. The limit of arctan(x) with x->inf = pi/2, thus red_river_hfov_
  // can't get above 180 deg

  //// Test incorrect V_FOV
  // testing below threshold
  blk.set( "red_river_aspect_box", "1 -30 1" );

  // should pass because we're not in red_river mode
  blk.set( "metadata_format", "klv" );
  TEST( "Set params - klv", src.set_params( blk ), true );

  // Should fail with format being red river now
  blk.set( "metadata_format", "red_river" );
  TEST( "Set params - red river", src.set_params( blk ), false );

  // testing above threshold
   //*** The test for above vertical threshold is no longer valid given the new
  // calculations. The limit of arctan(x) with x->inf = pi/2, thus red_river_vfov_
  // can't get above 180 deg
}

} // end anonymous namespace

int test_frame_metadata_decoder_process( int argc, char* argv[] )
{
  if( argc < 3 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "test_frame_metadata_decoder_process" );

  test_frame_time_count_1( argv[1] );
  test_frame_time_count_2( argv[1] );
  test_frame_time_count_3( argv[1] );
  test_roi( argv[1] );
  test_non_set_port();
  test_file_meta_missmatch( argv[1] );
  test_red_river_metadata_reader( argv[1] );
  test_incorrect_md_format();
  test_incorrect_red_river_fov();

  return testlib_test_summary();
}
