/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/format_block.h>
#include <utilities/vsl/timestamp_io.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
replace_spaces( std::string& str )
{
  std::string::iterator it = str.begin();
  for( ; it != str.end(); ++it )
  {
    if( *it == ' ' )
    {
      *it = '+';
    }
  }
}


void
test( std::string const& source,
      std::string const& prefix,
      unsigned line_length,
      std::string const& expected )
{
  std::string output = vidtk::format_block( source, prefix, line_length );
  bool good = ( output == expected );
  if( !good )
  {
    replace_spaces( output );
    std::string exp_copy = expected;
    replace_spaces( exp_copy );
    std::cout << "\n\n\nSource/expected/generated (+ = space):\n---\n" << source
             << "\n---\n" << exp_copy
             << "\n---\n" << output
             << "\n---\n";
  }
  TEST( "Expected output", good, true );
}



} // end anonymous namespace

int test_format_block( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "format_block" );

  test( "The quick brown     fox jumped over the moon\n yes, the moon.\nWow.",
        "#-", 11,
        "#-The quick\n"
        "#-brown fox\n"
        "#-jumped\n"
        "#-over the\n"
        "#-moon\n"
        "#- yes, the\n"
        "#- moon.\n"
        "#-Wow.\n" );

  test( "Alongword\nAnotherlongword",
        "###", 5,
        "###Alongword\n"
        "###Anotherlongword\n" );

  test( "Alongword\nAnotherlongword",
        "###", 11,
        "###Alongword\n"
        "###Anotherlongword\n" );

  test( "Alongword\nAnotherlongword",
        "###", 12,
        "###Alongword\n"
        "###Anotherlongword\n" );

  test( "Alongword Anotherlongword",
        "###", 12,
        "###Alongword\n"
        "###Anotherlongword\n" );

  test( "the \n\n\ncat  ",
        "###", 12,
        "###the\n"
        "###\n"
        "###\n"
        "###cat\n" );

  test( "Alongword\n    Anotherlongword",
        "###", 12,
        "###Alongword\n"
        "###    Anotherlongword\n" );

  test( "Alongword\n    Anotherlongword",
        "###", 15,
        "###Alongword\n"
        "###    Anotherlongword\n" );

  test( "",
        "###", 15,
        "" );

  test( "  ",
        "###", 15,
        "###  \n" );

  test( "hello\n",
        "###", 15,
        "###hello\n" );

  return testlib_test_summary();
}
