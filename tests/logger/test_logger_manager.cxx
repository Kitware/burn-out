/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <logger/logger.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void test_manager()
{
  char * argv[] = {
    (char *) "test_program_name",
    (char *) "--logger-app-instance", (char *) "instance-one",
    (char *) "app-arg", 0
  };


  vidtk::logger_manager * the_one = vidtk::logger_manager::instance();

  vidtk::logger_manager::instance()->initialize(4, argv);

  TEST ("Application name", vidtk::logger_manager::instance()->get_application_name(),
        "test_program_name" );

  TEST ("Application instance name", vidtk::logger_manager::instance()->get_application_instance_name(),
        "instance-one" );

  TEST ("Factory name", vidtk::logger_manager::instance()->get_factory_name(),
        "mini logger" );

  the_one->set_system_name("my system");
  TEST ("System name", vidtk::logger_manager::instance()->get_system_name(),
        "my system" );

  the_one->set_application_name("new-name");
  TEST ("Setting application name", vidtk::logger_manager::instance()->get_application_name(),
        "new-name" );

  TEST ("Singleton instance", the_one, vidtk::logger_manager::instance() );
}



  void test_create_logger()
{
  vidtk:: vidtk_logger_sptr log2 =  vidtk::logger_manager::instance()->get_logger("main.logger2");

  TEST ("Logger name", log2->get_name(), "main.logger2" );

}

} // end namespace

// ----------------------------------------------------------------
/** Main test driver for this file.
 *
 *
 */
int test_logger_manager(int /*argc*/, char * /*argv*/[])
{
  testlib_test_start( "logger manager");

  test_manager();
  test_create_logger();

  return testlib_test_summary();
}
