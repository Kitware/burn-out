/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/blank_line_filter.h>
#include <sstream>
#include <iostream>
#include <boost/iostreams/filtering_stream.hpp>
#include <utilities/shell_comments_filter.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {



}  // end namespace


int test_stream_filter( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "stream_filter" );

  std::stringstream buffer;

  buffer << "hello\n"
         << "   space\n"
         << "    \n"
         << "\n"
         << "\t\n"
         << "\r\n"          // DOS line endings
         << "\ttab\n"
         << "done\n"
         << "# comment\n"
         << "\t# comment\n"
         << "    # comment2\n"
         << "donedone"; // no newline

  std::string truth[] = {
    "hello", "   space", "\ttab", "done", "donedone" };
  int idx(0);


  boost::iostreams::filtering_istream in_stream;
  in_stream.push (vidtk::blank_line_filter());
  in_stream.push (vidtk::shell_comments_filter());
  in_stream.push (buffer);

  std::string tmp;
  while (std::getline(in_stream, tmp))
  {
    // std::cout << "Line from filter: |" << tmp << "|\n";
    TEST ("Line as expected",tmp, truth[idx++] );
    if (idx > 4) break;
  }

  TEST("number of lines", idx, 5);

  return testlib_test_summary();
}
