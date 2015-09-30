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

void test_levels()
{
  vidtk::vidtk_logger_sptr log2 =  vidtk::logger_manager::instance()->get_logger("main.logger2");

  log2->set_level(vidtk::vidtk_logger::LEVEL_WARN);

  TEST ("Trace level enabled", log2->is_trace_enabled(), false );
  TEST ("Debug level enabled", log2->is_debug_enabled(), false );
  TEST ("Info level enabled", log2->is_info_enabled(), false );
  TEST ("Warn level enabled", log2->is_warn_enabled(), true );
  TEST ("Error level  enabled", log2->is_error_enabled(), true );
  TEST ("Fatal level enabled", log2->is_fatal_enabled(), true );

  TEST ("Logger name", log2->get_name(), "main.logger2" );

  TEST ("Get legger level", log2->get_level(), vidtk::vidtk_logger::LEVEL_WARN );

  log2->set_level(vidtk::vidtk_logger::LEVEL_DEBUG);

  TEST ("Trace level enabled", log2->is_trace_enabled(), false );
  TEST ("Debug level enabled", log2->is_debug_enabled(), true );
  TEST ("Info level enabled", log2->is_info_enabled(), true );
  TEST ("Warn level enabled", log2->is_warn_enabled(), true );
  TEST ("Error level  enabled", log2->is_error_enabled(), true );
  TEST ("Fatal level enabled", log2->is_fatal_enabled(), true );

  // a compiler check, mostly
  VIDTK_LOGGER("logger_test");
  LOG_ASSERT( true, "This should compile." );

  TEST ("Get legger level", log2->get_level(), vidtk::vidtk_logger::LEVEL_DEBUG );

}

} // end namespace

// ----------------------------------------------------------------
/** Main test driver for this file.
 *
 *
 */
int test_logger(int /*argc*/, char * /*argv*/[])
{
  testlib_test_start( "logger API");

  test_levels();

  return testlib_test_summary();
}
