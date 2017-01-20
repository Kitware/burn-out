/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/string_to_vector.h>

#include <string>
#include <vector>

int test_string_to_vector( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "string_to_vector" );

  {
    std::string test1( "This is a great test." );
    std::string test2( "      10   1 , 3\n,,, 4" );
    std::string test3( " I love testing things. " );
    std::string test4( "10 0.111 0.2 0.3 4 " );

    std::vector<std::string> out1;
    std::vector<int> out2;
    std::vector<std::string> out3;
    std::vector<double> out4;

    bool ret1 = vidtk::string_to_vector( test1, out1 );
    bool ret2 = vidtk::string_to_vector( test2, out2 );
    bool ret3 = vidtk::string_to_vector( test3, out3 );
    bool ret4 = vidtk::string_to_vector( test4, out4 );

    TEST( "Return value 1", ret1, true );
    TEST( "Return value 2", ret2, true );
    TEST( "Return value 3", ret3, true );
    TEST( "Return value 4", ret4, true );

    TEST( "String 1 split size", out1.size(), 5 );
    TEST( "String 2 split size", out2.size(), 4 );
    TEST( "String 3 split size", out3.size(), 4 );
    TEST( "String 4 split size", out4.size(), 5 );

    TEST( "Parsed string 1 value", out1[1], "is" );
    TEST( "Parsed string 2 value", out2[2], 3 );
    TEST( "Parsed string 3 value", out3[0], "I" );
    TEST( "Parsed string 4 value", out4[3], 0.3 );

    ret1 = vidtk::string_to_vector( test1, out4 );
    ret2 = vidtk::string_to_vector( test4, out2 );

    TEST( "Invalid parse return value 1", ret1, false );
    TEST( "Invalid parse return value 2", ret2, false );
  }

  return testlib_test_summary();
}
