/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_algorithm.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <testlib/testlib_test.h>

#include <vidl/vidl_config.h>

#if VIDL_HAS_FFMPEG

#include <video/vidl_ffmpeg_frame_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_frame_reading( vcl_string const& dir, vcl_string const& filename )
{
  vcl_cout << "\n\nTesting frame reading on " << filename << "\n\n";

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
    vcl_cout << "Count = " << frame_cnt << "\n";
    TEST( "Frame number", src.timestamp().frame_number(), frame_cnt );
    TEST( "No timestamp", src.timestamp().has_time(), false );
    TEST( "Has image", bool(src.image()), true );
    vil_image_view<vxl_byte> const& img = src.image();
    vcl_cout << "    pixel(0,0) = "
             << int(img(0,0,0)) << " "
             << int(img(0,0,1)) << " "
             << int(img(0,0,2)) << "\n";
    vcl_cout << "    pixel(3,4) = "
             << int(img(3,4,0)) << " "
             << int(img(3,4,1)) << " "
             << int(img(3,4,2)) << "\n";
    vcl_cout << "    pixel(9,11) = "
             << int(img(9,11,0)) << " "
             << int(img(9,11,1)) << " "
             << int(img(9,11,2)) << "\n";
    vcl_cout << "    pixel(15,15) = "
             << int(img(15,15,0)) << " "
             << int(img(15,15,1)) << " "
             << int(img(15,15,2)) << "\n";
    TEST( "Pixel (0,0)=Pixel (3,4)",
          ( img(0,0,0)==img(3,4,0) &&
            img(0,0,1)==img(3,4,1) &&
            img(0,0,2)==img(3,4,2) ),
          true );
    TEST( "Pixel (0,0)=expected value",
          ( img(0,0,0)==expectedTop[frame_cnt] &&
            img(0,0,1)==expectedTop[frame_cnt] &&
            img(0,0,2)==expectedTop[frame_cnt] ),
          true );
    TEST( "Pixel (9,11)=Pixel (15,15)",
          ( img(9,11,0)==img(15,15,0) &&
            img(9,11,1)==img(15,15,1) &&
            img(9,11,2)==img(15,15,2) ),
          true );
    TEST( "Pixel (15,15)=expected value",
          ( img(15,15,0)==expectedBot[frame_cnt] &&
            img(15,15,1)==expectedBot[frame_cnt] &&
            img(15,15,2)==expectedBot[frame_cnt] ),
          true );
  }

  TEST( "Last frame was frame 19", frame_cnt, 20 );
}


} // end anonymous namespace

int test_vidl_ffmpeg_frame_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    vcl_cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "vidl_ffmpeg_frame_process" );

  test_frame_reading( argv[1], "small_synthetic.mpg" );

  // Fixme: Don't test this yet, because the raw video reader seems to be
  // broken.
  //test_frame_reading( argv[1], "small_synthetic.avi" );

  return testlib_test_summary();
}

#else // VIDL_HAS_FFMPEG

int test_vidl_ffmpeg_frame_process( int argc, char* argv[] )
{
  vcl_cout << "vidl does not have ffmpeg. Not testing video reading"
           << vcl_endl;
  return EXIT_SUCCESS;
}

#endif // VIDL_HAS_FFMPEG
