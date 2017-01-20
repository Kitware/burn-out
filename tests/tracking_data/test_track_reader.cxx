/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/io/track_reader.h>
#include <testlib/testlib_test.h>

namespace {  // anon

// ----------------------------------------------------------------
void test_bad_file( char * path )
{
  std::string xmlPath( path );
  xmlPath += "/missing_file.txt";
  vidtk::track_reader reader( xmlPath );
  TEST( "Opening invalid file", reader.open(), false );
}


// ----------------------------------------------------------------
void test_interleave( char* path )
{
  std::string xmlPath( path );
  xmlPath += "/kw18-test.trk";

  {
    vidtk::track_reader reader( xmlPath );
    if ( ! reader.open() )
    {
      std::ostringstream oss;
      oss << "Failed to open '" << xmlPath << "'";
      TEST( oss.str().c_str(), false, true );
      return;
    }

    vidtk::track::vector_t tracks;
    unsigned ts;

    // test mixing reading methods
    TEST( "Read next", reader.read_next_terminated( tracks, ts ), true );
    TEST( "Read all", reader.read_all( tracks ), 0 );
  }

  {
    vidtk::track_reader reader( xmlPath );
    if ( ! reader.open() )
    {
      std::ostringstream oss;
      oss << "Failed to open '" << xmlPath << "'";
      TEST( oss.str().c_str(), false, true );
      return;
    }

    vidtk::track::vector_t tracks;
    unsigned ts;

    // test mixing reading methods
    TEST( "Read all", reader.read_all( tracks ), 301 );
    TEST( "Read next", reader.read_next_terminated( tracks, ts ), false );
  }
}

} // end namespace


// ----------------------------------------------------------------
int test_track_reader( int argc, char *argv[] )
{
  testlib_test_start( "test track reader" );
  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  test_bad_file( argv[1] );
  test_interleave( argv[1] );

  return testlib_test_summary();
}
