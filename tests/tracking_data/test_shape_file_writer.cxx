/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <tracking_data/io/track_reader.h>
#include <tracking_data/io/shape_file_writer.h>
#include <testlib/testlib_test.h>

using namespace vidtk;

int test_shape_file_writer( int argc, char* /* const*/ argv[] )
{
  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  {
    std::string kw18_string = argv[1];
    kw18_string += "/test_shape_file_writer.kw18";

    std::vector< vidtk::track_sptr > tracks;
    vidtk::track_reader reader( kw18_string );

    if ( ! reader.open() )
    {
      std::ostringstream oss;
      oss << "Failed to open '" << kw18_string << "'";
      TEST( oss.str().c_str(), false, true );
      return EXIT_FAILURE;
    }

    TEST("read kw18 stream", reader.read_all( tracks ), 2 );
    std::string fname = "test_shape";

    { //APIX
      vidtk::shape_file_writer shape(true);
      TEST("Make sure write fails if file not open", shape.write(tracks[0]), false);
      shape.open( fname );
      shape.set_is_utm(false);
      TEST("should write now", shape.write(tracks[0]), true);
      shape.set_is_utm(true);
      shape.set_utm_zone(16, true);
      shape << tracks[0];
      shape.close();
    }
    { //non-apix
      vidtk::shape_file_writer shape(false);
      TEST("Make sure write fails if file not open", shape.write(tracks[0]), false);
      shape.open( fname );
      shape.set_is_utm(false);
      TEST("should write now", shape.write(tracks[0]), true);
      shape.set_is_utm(true);
      shape.set_utm_zone(16, true);
      shape << tracks[0];
      shape.close();
    }

  }

  return testlib_test_summary();
}
