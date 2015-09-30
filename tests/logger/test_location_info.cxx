/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */



#include <testlib/testlib_test.h>
#include <logger/location_info.h>
// #include <vcl_iostream> // used for test development and verification


// Put everything in a namespace so that different tests
// won't conflict.
namespace test_location_info_ns {

class test_class {
 public:
  static void test_loc(vcl_string /*s*/)
  {
    VIDTK_LOGGER_SITE;

    ::vidtk::logger_ns::location_info  loc(__FILE__,  __VIDTK_LOGGER_FUNC__,  __LINE__);
    int line = __LINE__;

    // vcl_cerr << "*** logger-func: " << __VIDTK_LOGGER_FUNC__ << "\n";
    // vcl_cerr << "*** class name: " << loc.get_class_name() << "\n";
    // vcl_cerr << "*** method name: " << loc.get_method_name() << "\n";

    TEST ("test file name", loc.get_file_name(), "test_location_info.cxx");
    TEST ("test line number", loc.get_line_number(), line-1);
    TEST ("test method name", loc.get_method_name(), "test_loc" );
    TEST ("test class name", loc.get_class_name(), "test_location_info_ns::test_class");
  }

};


  void test_loc_variants()
  {
    { // test 1
      ::vidtk::logger_ns::location_info  loc(__FILE__,  "void foo1()",  __LINE__);

      // vcl_cerr << "\n*** class name: " << loc.get_class_name() << "\n";
      // vcl_cerr << "*** method name: " << loc.get_method_name() << "\n";

      TEST ("test method name", loc.get_method_name(), "foo1" );
      TEST ("test class name", loc.get_class_name(), "");
    }

    { // test 2
      ::vidtk::logger_ns::location_info  loc(__FILE__,  "foo::foo2()",  __LINE__);

      // vcl_cerr << "\n*** class name: " << loc.get_class_name() << "\n";
      // vcl_cerr << "*** method name: " << loc.get_method_name() << "\n";

      TEST ("test method name", loc.get_method_name(), "foo2" );
      TEST ("test class name", loc.get_class_name(), "foo");
    }

    { // test 2a
      ::vidtk::logger_ns::location_info  loc(__FILE__,  "double foo::foo2a()",  __LINE__);

      // vcl_cerr << "\n*** class name: " << loc.get_class_name() << "\n";
      // vcl_cerr << "*** method name: " << loc.get_method_name() << "\n";

      TEST ("test method name", loc.get_method_name(), "foo2a" );
      TEST ("test class name", loc.get_class_name(), "foo");
      TEST ("test signature", loc.get_signature(), "double foo::foo2a()" );
    }

    { // test 3
      ::vidtk::logger_ns::location_info  loc(__FILE__,  "::ns1::foo::obj * foo3()",  __LINE__);

      // vcl_cerr << "\n*** class name: " << loc.get_class_name() << "\n";
      // vcl_cerr << "*** method name: " << loc.get_method_name() << "\n";

      TEST ("test method name", loc.get_method_name(), "foo3" );
      TEST ("test class name", loc.get_class_name(), "");
    }

    { // test 4
      ::vidtk::logger_ns::location_info  loc(__FILE__,  "ns1::foo::obj * class::foo4()",  __LINE__);

      // vcl_cerr << "\n*** class name: " << loc.get_class_name() << "\n";
      // vcl_cerr << "*** method name: " << loc.get_method_name() << "\n";

      TEST ("test method name", loc.get_method_name(), "foo4" );
      TEST ("test class name", loc.get_class_name(), "class");
    }

    { // test 5
      ::vidtk::logger_ns::location_info  loc(__FILE__,  "::ns1::foo::obj * ::class::foo5()",  __LINE__);

      // vcl_cerr << "\n*** class name: " << loc.get_class_name() << "\n";
      // vcl_cerr << "*** method name: " << loc.get_method_name() << "\n";

      TEST ("test method name", loc.get_method_name(), "foo5" );
      TEST ("test class name", loc.get_class_name(), "::class");
    }

  }

 void test_loc_def()
  {
    ::vidtk::logger_ns::location_info  loc;

    TEST ("test default file name", loc.get_file_name(),vidtk::logger_ns::location_info::NA);
    TEST ("test default method name", loc.get_method_name(), "?" );
    TEST ("test line number", loc.get_line_number(), -1);
  }

 } // end namespace

using namespace test_location_info_ns;


// ----------------------------------------------------------------
/** Main test driver for this file.
 *
 *
 */
int test_location_info(int /*argc*/, char * /*argv*/[])
{
  testlib_test_start( "logger location");

  test_class::test_loc(" ");
  test_loc_variants();
  test_loc_def();

  return testlib_test_summary();
}
