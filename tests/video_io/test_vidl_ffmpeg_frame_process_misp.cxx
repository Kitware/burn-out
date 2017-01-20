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

void test_frame_reading(std::string const& filename,
                         double ts_factor, double* ts_table,
                         unsigned int number_frames, unsigned int number_frames_cutoff,
                         unsigned int* pixel_table
                       )
{
  std::cout << "\n\nTesting frame reading on " << filename << "\n\n";

  vidl_ffmpeg_frame_process src( "src" );

  config_block blk = src.params();
  blk.set( "filename", filename );
  blk.set("time_type", "misp");
  blk.set("ts_scaling_factor", ts_factor);

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST_EQUAL("Numer of Frames", number_frames, src.nframes());

  unsigned int frame_cnt = 0;
  for (frame_cnt = 0; frame_cnt < number_frames_cutoff && src.step(); ++frame_cnt)
  {
    timestamp ts = src.timestamp();
    TEST_EQUAL( "Frame number", ts.frame_number(), frame_cnt );
    TEST( "Has timestamp", ts.has_time(), true );
    TEST_NEAR("Correct time", ts.time(), ts_table[frame_cnt] * ts_factor, 1e6);
    TEST( "Has image", bool(src.image()), true );
    vil_image_view<vxl_byte> const& img = src.image();

#define TEST_PIXEL(x, y, expected_value, index)                   \
  TEST_EQUAL("Pixel(" #x "," #y ") R ",                           \
      img(x, y, 0), expected_value[index]);                       \
  TEST_EQUAL("Pixel(" #x "," #y ") G ",                           \
      img(x, y, 1), expected_value[index + 1]);                   \
  TEST_EQUAL("Pixel(" #x "," #y ") B ",                           \
      img(x, y, 2), expected_value[index + 2]);

    TEST_PIXEL(30, 30, pixel_table, 3 * frame_cnt);
  }

  TEST_EQUAL("Stepped until cutoff", frame_cnt, number_frames_cutoff);
}


} // end anonymous namespace

int test_vidl_ffmpeg_frame_process_misp( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "vidl_ffmpeg_frame_process_misp" );

  const unsigned int number_frames_cutoff = 15;
  // These MISP timestamp are invalid as is, they become valid using the 1e-3
  // ts_scaling_factor to scale them down to microseconds.
  double ts_table[number_frames_cutoff + 1] =
  {
    1221514029416166700.0,
    1221514029416200200.0,
    1221514029416233500.0,
    1221514029416266800.0,
    1221514029416300300.0,
    1221514029416333600.0,
    1221514029416366800.0,
    1221514029416400400.0,
    1221514029416433700.0,
    1221514029416466900.0,
    1221514029416500500.0,
    1221514029416533800.0,
    1221514029416567000.0,
    1221514029416600600.0,
    1221514029416633900.0,
  };
  unsigned int pixel_table[3*(number_frames_cutoff + 1)] =
  {
    64, 91, 101,
    61, 98, 85,
    80, 111, 114,
    55, 92, 77,
    71, 97, 102,
    59, 98, 85,
    86, 114, 116,
    68, 102, 86,
    50, 87, 62,
    52, 89, 76,
    35, 69, 74,
    30, 74, 57,
    37, 68, 81,
    34, 78, 61,
    55, 83, 89,
  };

  test_frame_reading(argv[1], 1.0, ts_table, 8992, number_frames_cutoff, pixel_table);
  test_frame_reading(argv[1], 1e-3, ts_table, 8992, number_frames_cutoff, pixel_table);

  return testlib_test_summary();
}

#else // VIDL_HAS_FFMPEG

int test_vidl_ffmpeg_frame_process_misp( int /*argc*/, char* /*argv*/[] )
{
  std::cout << "vidl does not have ffmpeg. Not testing video reading"
           << std::endl;
  return EXIT_SUCCESS;
}

#endif // VIDL_HAS_FFMPEG
