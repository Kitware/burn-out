/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <testlib/testlib_test.h>

#include <vidl/vidl_config.h>

#if VIDL_HAS_FFMPEG

#include <video_io/vidl_ffmpeg_frame_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_frame_reading( std::string const& dir, std::string const& filename )
{
  std::cout << "\n\nTesting frame reading on " << filename << "\n\n";

  vidl_ffmpeg_frame_process src( "src" );

  config_block blk = src.params();
  blk.set( "filename", dir+"/" + filename );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  // 21 to have an extra buffer in case the reader fails.
  int expectedTop[21] = { 36, 39, 43, 46, 49, 53, 56, 60, 63, 67,
                          70, 74, 77, 80, 84, 87, 91, 94, 98, 101 };

  int expectedBot[21] = { 202, 199, 195, 192, 189, 185, 182, 178, 175, 171,
                          168, 165, 161, 158, 154, 151, 147, 144, 140, 137 };

  unsigned frame_cnt = 0;
  for( frame_cnt = 0; frame_cnt < 21 && src.step(); ++frame_cnt )
  {
    std::cout << "Count = " << frame_cnt << "\n";
    timestamp ts = src.timestamp();
    TEST_EQUAL( "Frame number", ts.frame_number(), frame_cnt );
    TEST( "Has timestamp", ts.has_time(), true );
    TEST_NEAR( "Correct time", ts.time(), ts.frame_number()*40000, 1e6 );
    TEST( "Has image", bool(src.image()), true );
    vil_image_view<vxl_byte> const& img = src.image();

#define TEST_PIXEL(x, y, expected_value)                   \
  TEST_EQUAL("Pixel(" #x "," #y ") R == " #expected_value, \
      img(x, y, 0), expected_value);                       \
  TEST_EQUAL("Pixel(" #x "," #y ") G == " #expected_value, \
      img(x, y, 1), expected_value);                       \
  TEST_EQUAL("Pixel(" #x "," #y ") B == " #expected_value, \
      img(x, y, 2), expected_value);

#define TEST_PIXELS(x1, y1, x2, y2)                                  \
  TEST_EQUAL("Pixel(" #x1 "," #y1 ") R == Pixel(" #x2 "," #y2 ") R", \
      img(x1, y1, 0), img(x2, y2, 0));                               \
  TEST_EQUAL("Pixel(" #x1 "," #y1 ") G == Pixel(" #x2 "," #y2 ") G", \
      img(x1, y1, 1), img(x2, y2, 1));                               \
  TEST_EQUAL("Pixel(" #x1 "," #y1 ") B == Pixel(" #x2 "," #y2 ") B", \
      img(x1, y1, 2), img(x2, y2, 2));                               \

    TEST_PIXELS(0, 0, 3, 4);
    TEST_PIXEL(0, 0, expectedTop[frame_cnt]);
    TEST_PIXELS(9, 11, 15, 15);
    TEST_PIXEL(15, 15, expectedBot[frame_cnt]);
  }

  TEST( "Last frame was frame 19", frame_cnt, 20 );
}


} // end anonymous namespace

int test_vidl_ffmpeg_frame_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "vidl_ffmpeg_frame_process" );

  // Before FFMpeg 2.6 one of these would fail. Now, we
  // expect both should pass. Testing both is slightly
  // redundant, but is preserved to protect against regression.
  test_frame_reading( argv[1], "small_synthetic.mpg" );
  test_frame_reading( argv[1], "small_synthetic.avi" );

  return testlib_test_summary();
}

#else // VIDL_HAS_FFMPEG

int test_vidl_ffmpeg_frame_process( int /*argc*/, char* /*argv*/[] )
{
  std::cout << "vidl does not have ffmpeg. Not testing video reading"
           << std::endl;
  return EXIT_SUCCESS;
}

#endif // VIDL_HAS_FFMPEG
