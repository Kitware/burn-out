/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/gsd_file_source.h>

using namespace vidtk;

namespace // anon
{

void test_empty_file()
{
  gsd_file_source_t gsd;

  {
    std::stringstream ss;
    ss << "";
    TEST( "Initializing on an empty stream fails", gsd.initialize( ss ), false );
  }
  {
    std::stringstream ss;
    ss << "\n    \n";
    TEST( "Initializing on a blank stream fails", gsd.initialize( ss ), false );
  }
  {
    std::stringstream ss;
    ss << "#\n  #\n  #comments!\n\n";
    TEST( "Initializing on a blank stream with comments fails", gsd.initialize( ss ), false );
  }
  {
    std::stringstream ss;
    ss << "This is not a gsd source file format";
    TEST( "Initializing on garbage fails", gsd.initialize( ss ), false );
  }
}

void test_uninitialized_traps()
{
  gsd_file_source_t gsd;
  bool caught = false;
  try
  {
    std::cout << gsd.get_gsd( timestamp( 100.0, 10 )) << " ...shouldn't see that...";
  }
  catch ( std::runtime_error &e )
  {
    caught = true;
  }
  TEST( "Probing an uninitialized gsd file source throws", caught, true );
}

void test_default_only()
{
  {
    gsd_file_source_t gsd;
    std::stringstream ss;
    ss << "# default only\n  gsd using framenumber default 0.512";
    TEST( "Initializing on a default-only stream succeeds", gsd.initialize( ss ), true );
    TEST( "Default-only returns default on frame", gsd.get_gsd( timestamp(/* ts */ 100.0, /* frame */ 10 )), 0.512 );
    TEST( "Default-only returns default on time-only", gsd.get_gsd( timestamp( 100.0 )), 0.512 );
  }
}

void test_frame_ranges()
{
  std::stringstream ss;
  ss << "gsd using framenumber default 0.612\n"
     << "\n"
     << "# shot 1\n"
     << "10 20  1.612\n"
     << "\n"
     << "# shot 2\n"
     << "   21 31  2.612";

  gsd_file_source_t gsd;
  TEST( "Initializing with frame range succeeds", gsd.initialize( ss ), true );
  int frames[] = { 5, 10, 15, 20, 21, 25, 31, 32, -1 };
  double expected_gsds[] = { 0.612, 1.612, 1.612, 1.612, 2.612, 2.612, 2.612, 0.612, -1 };
  for (size_t i=0; frames[i] != -1; ++i)
  {
    timestamp ts( 100.0, frames[i] );
    double v = gsd.get_gsd( ts );
    std::ostringstream oss;
    oss << "Frame-range test: frame " << frames[i] << " has GSD of " << v << "; expected " << expected_gsds[i];
    TEST( oss.str().c_str(), v, expected_gsds[i] );
  }
}

void test_inverted_frame_ranges()
{
  std::stringstream ss;
  ss << "gsd using framenumber default 0.612\n"
     << "\n"
     << "# inverted frame range should fail\n"
     << "20 10  1.612\n";

  gsd_file_source_t gsd;
  TEST( "Initializing with inverted frame range fails", gsd.initialize( ss ), false );
}

void test_usec_ranges()
{
  std::stringstream ss;
  ss << "gsd using ts_usecs default 0.777\n"
     << "\n"
     << "# shot 1\n"
     << "500 500  1.777\n"
     << "\n"
     << "# shot 2\n"
     << "1000 1200  2.777";

  gsd_file_source_t gsd;
  TEST( "Initializing with ts_usecs succeeds", gsd.initialize( ss ), true );
  int usecs[] = { 0, 499, 500, 501, 999, 1000, 1001, 1199, 1200, 1201, -1 };
  double expected_gsds[] = { 0.777, 0.777, 1.777, 0.777, 0.777, 2.777, 2.777, 2.777, 2.777, 0.777, -1 };
  for (size_t i=0; usecs[i] != -1; ++i)
  {
    timestamp ts( usecs[i], i );
    double v = gsd.get_gsd( ts );
    std::ostringstream oss;
    oss << "Frame-range test: usecs " << usecs[i] << " has GSD of " << v << "; expected " << expected_gsds[i];
    TEST( oss.str().c_str(), v, expected_gsds[i] );
  }
}

void test_sec_ranges()
{
  std::stringstream ss;
  ss << "gsd using ts_secs default 0.888\n"
     << "\n"
     << "# shot 1\n"
     << "0.000500 0.000500  1.888\n"
     << "\n"
     << "# shot 2\n"
     << "0.001000 0.001200  2.888";

  gsd_file_source_t gsd;
  TEST( "Initializing with ts_secs succeeds", gsd.initialize( ss ), true );
  int secs[] = { 0, 499, 500, 501, 999, 1000, 1001, 1199, 1200, 1201, -1 };
  double expected_gsds[] = { 0.888, 0.888, 1.888, 0.888, 0.888, 2.888, 2.888, 2.888, 2.888, 0.888, -1 };
  for (size_t i=0; secs[i] != -1; ++i)
  {
    timestamp ts( secs[i] );
    double v = gsd.get_gsd( ts );
    std::ostringstream oss;
    oss << "Frame-range test: secs " << secs[i] / 1.0e6 << " has GSD of " << v << "; expected " << expected_gsds[i];
    TEST( oss.str().c_str(), v, expected_gsds[i] );
  }
}

} // anon

int test_gsd_file_source( int /* argc */, char * /*argv */ [] )
{
  testlib_test_start( "gsd_file_source" );

  test_empty_file();
  test_uninitialized_traps();
  test_inverted_frame_ranges();
  test_default_only();
  test_frame_ranges();
  test_usec_ranges();
  test_sec_ranges();

  return testlib_test_summary();
}
