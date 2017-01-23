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

#include <video_io/image_list_frame_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_frame_count( std::string const& dir )
{
  std::cout << "\n\nTesting frame number increment\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "glob", dir+"/smallframe*.pgm" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 0 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 1 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 2 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 3 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 4", src.step(), false );
}


void
test_parse_frame_number( std::string const& dir )
{
  std::cout << "\n\nTesting frame number parsing\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "glob", dir+"/smallframe*.pgm" );
  blk.set( "parse_frame_number", dir+"/smallframe%03u_%*f.pgm" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 10 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 11 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 12 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 4", src.step(), false );
}

void
test_parse_base_frame_number( std::string const& dir )
{
  std::cout << "\n\nTesting base frame number\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "glob", dir+"/smallframe*.pgm" );
  blk.set( "base_frame_number", "BOB" );
  TEST( "Set params", src.set_params( blk ), false );
  blk.set( "base_frame_number", "20" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 20 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 21 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 22 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 23 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 4", src.step(), false );

  TEST( "Seek to begining", src.seek(20), true );
  TEST( "Frame number", src.timestamp().frame_number(), 20 );
  TEST( "Step after seek", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 21 );

  TEST( "Seek to before", src.seek(10), false );
  TEST( "Seek to after", src.seek(24), false );
}


void
test_parse_both( std::string const& dir )
{
  std::cout << "\n\nTesting frame number parsing\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "glob", dir+"/smallframe*.pgm" );
  blk.set( "parse_frame_number", dir+"/smallframe%03u_%*f.pgm" );
  blk.set( "parse_timestamp", dir+"/smallframe%*03u_%lf.pgm" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 10 );
  TEST( "Time", src.timestamp().time(), 6.7 );

  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 11 );
  TEST( "Time", src.timestamp().time(), 7.5 );

  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 12 );
  TEST( "Time", src.timestamp().time(), 8.6 );

  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 13 );
  TEST( "Time", src.timestamp().time(), 9.4 );

  TEST( "Step 4", src.step(), false );
}

void
test_roi( std::string const& dir )
{
  std::cout << "\n\nTesting frame number parsing\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "glob", dir+"/smallframe*.pgm" );
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


void
test_parse_time( std::string const& dir )
{
  std::cout << "\n\nTesting ROI setting\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "glob", dir+"/smallframe*.pgm" );
  blk.set( "parse_timestamp", dir+"/smallframe%*03u_%lf.pgm" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 0 );
  TEST( "Time", src.timestamp().time(), 6.7 );

  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 1 );
  TEST( "Time", src.timestamp().time(), 7.5 );

  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 2 );
  TEST( "Time", src.timestamp().time(), 8.6 );

  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 3 );
  TEST( "Time", src.timestamp().time(), 9.4 );

  TEST( "Step 4", src.step(), false );
}

void test_list_file_no_not_exist( std::string const & dir )
{
  std::cout << "\n\nTesting frame number increment\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "file", dir+"/DOES_NOT_EXIST.txt" );

  TEST( "Set params", src.set_params( blk ), false );
}

void
test_file( std::string const& dir )
{
  std::cout << "\n\nTesting frame number increment\n\n";

  image_list_frame_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "file", dir+"/smallframe_file.txt" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 0 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 1", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 1 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 2", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 2 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 3", src.step(), true );
  TEST( "Frame number", src.timestamp().frame_number(), 3 );
  TEST( "No timestamp", src.timestamp().has_time(), false );

  TEST( "Step 4", src.step(), false );
}

} // end anonymous namespace

int test_image_list_frame_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "image_list_frame_process" );

  test_frame_count( argv[1] );
  test_parse_frame_number( argv[1] );
  test_parse_time( argv[1] );
  test_parse_both( argv[1] );
  test_list_file_no_not_exist( argv[1] );
  test_file( argv[2] );
  test_parse_base_frame_number( argv[1] );
  test_roi( argv[1] );

  return testlib_test_summary();
}
