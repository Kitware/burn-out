/*ckwg +5
 * Copyright 2010,2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <logger/logger.h>
#include <logger/vidtk_logger_mini_logger.h>


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

  TEST ("Get logger level", log2->get_level(), vidtk::vidtk_logger::LEVEL_WARN );

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

  TEST ("Get logger level", log2->get_level(), vidtk::vidtk_logger::LEVEL_DEBUG );

}

void test_redirect()
{
#ifdef VIDTK_DEFAULT_LOGGER
#undef VIDTK_DEFAULT_LOGGER
#endif

#define VIDTK_DEFAULT_LOGGER log2

  std::ostringstream sstr;
  vidtk::vidtk_logger_sptr log2 =  vidtk::logger_manager::instance()->get_logger("main.logger.3");

  ::vidtk::logger_ns::vidtk_logger_mini_logger* lptr =
      dynamic_cast< ::vidtk::logger_ns::vidtk_logger_mini_logger* >(log2.ptr());

  // Return if mini logger not the active back end
  if ( !lptr )
  {
    return;
  }

  lptr->set_output_stream( &sstr );

  LOG_WARN( "Test-Message" );
  LOG_ERROR( "Test ERROR Message" );

  TEST( "Log to stream", sstr.str().find( "Test-Message" ) != std::string::npos, true );

#undef VIDTK_DEFAULT_LOGGER
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
  test_redirect();

  return testlib_test_summary();
}
