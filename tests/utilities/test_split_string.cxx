/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <string>
#include <testlib/testlib_test.h>

#include <utilities/split_string_process.h>

int test_split_string( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "split_string_process" );

  {
    vidtk::split_string_process p("splitter");
    vidtk::config_block blk = p.params();
    blk.set<std::string>("pattern", "([^:]*):(.*)");
    TEST("split_string_process::set_params", p.set_params(blk), true);

    p.set_input("Hello:World");
    TEST("\'Hello:World\' step", p.step(), true);
    TEST("\'Hello:World\' group1", p.group1(), "Hello");
    TEST("\'Hello:World\' group2", p.group2(), "World");

    p.set_input("Hello : World");
    TEST("\'Hello : World\' step", p.step(), true);
    TEST("\'Hello : World\' group1", p.group1(), "Hello ");
    TEST("\'Hello : World\' group2", p.group2(), " World");

    p.set_input("HelloWorld");
    TEST("\'HelloWorld\' step", p.step(), false);
  }

  return testlib_test_summary();
}
