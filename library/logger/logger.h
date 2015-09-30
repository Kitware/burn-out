/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _LOGGER__API_H_
#define _LOGGER__API_H_

#include <logger/logger_manager.h>
#include <logger/location_info.h>
#include <vcl_sstream.h>


// ----------------------------------------------------------------
/** @page Logger Logger Documentation

<P>The vidtk logger (class vidtk_logger) provides a interface to an
underlying log implementation. Log4cxx is the default implementation,
which is why this interface looks the way it does. Alternate loggers
can be instantiated as needed for specific applications, products, or
projects. These alternate logger inplementations are supplied by a
factory class and can provide any functionality consistent with the
vidtk_logger interface.</P>

<P>The logger_manager class is a singleton and is accessed through the
vidtk::logger_manager::instance() pointer. Individual logger objects
(derived from vidtk_logger class) can be created before the
logger_manager is initialized, but not used.  This allows loggers to
be created by static initializers, but be sure to initialize the
log_manager as soon as practical in the application, since the logger
will fail if log messages are generated before initialization.</P>

<P>The easiest way to applying the logger is to use the macros in the
logger/logger.h file. The \c VIDTK_LOGGER("name"); macro creates a \b
static logger with the specified name that is available within its
enclosing scope and is used by the \c LOG_*() macros.  These log
macros automatically determine the source file location (file name,
line number, method name) and make it available to the logger.</p>

@sa vidtk_logger


<h2>Internal Operation</h2>

<p>During construction, the logger_manager instantiates a factory class
based on the selected underlying logger.  This factory class creates
logger (vidtl_logger) objects for use by the application. Creating a
logger in the static constructor phase will instantiate the
logger_manager.</p>

<P>The ability to support alternate underlying logger implementations is
designed to make this logging component easy(er) to transport to
projects that have a specific (not log4cxx) logging implementation
requirement.  Currently it looks like some code changes are required
to remove our log4cxx support to the point where it is not required by
the linker.</p>


<h2>Configuration</h2>

The logger can be configured (via CMake) to include log4cxx. If
included, then it is the default logger implementation.  If log4cxx is
not enabled, then the only implementation is the mini-logger.

In the grand scheme, alternate loggers can be specified with the \c
VIDTK_LOGGER_FACTORY environment variable.  Currently the only other
logger type (factory) is \c MINI_LOGGER which just displays log
messages on the console.

The main function \c argv and \c argc parameters are passed into the
logger initialization method. This method looks for the following
optional parameters from the command line.<br>

--logger-app <name> - specifies the application name. Overrides the name in argv[0].<br>
--logger-app-instance <name> - specifies the aplication isntance name. This is useful
in multi-process applications where there is more than one instance of a program.<br>
--logger-config <name> - specifies the name of the logger configuration file.
The syntax and semantics of this file depends on which logger factory is active.<br>

For the log4cxx version --

If no configuration file command line parameter is specified, the
environment variable "LOG4CXX_CONFIGURATION" is checked for the
configuration file name.

If a configuration file has not been specified, as above, the logger
looks for the file "log4cxx.properties" in the current directory.

If still no configuration file can be found, then a default
configuration is used, which generally does not do what you really
want.


<h2>Example</h2>

\code
#include <logger/logger.h>
#include <vcl_iostream.h>

// Create static default logger for log macros
VIDTK_LOGGER("main-test");

int main(int argc, char *argv[])
{

  // Initialize the logger component from command line parameters.
  vidtk::logger_manager::instance()->initialize(argc, argv);

  LOG_ERROR("first message" << " from here");

  LOG_ASSERT( false, "assertion blown" ); (logged at FATAL level)
  LOG_FATAL("fatal message");
  LOG_ERROR("error message");
  LOG_WARN ("warning message");
  LOG_INFO ("info message");
  LOG_DEBUG("debug message");
  LOG_TRACE("trace message");

  // A logger can be explicitly created if needed.
  vidtk_logger_sptr log2 =  logger_manager::instance()->get_logger("main.logger2");

  log2->set_level(vidtk_logger::LEVEL_WARN);

  vcl_cout << "Current log level "
           << log2->get_level_string (log2->get_level())
           << vcl_endl;

  log2->log_fatal("direct logger call");
  log2->log_error("direct logger call");
  log2->log_warn ("direct logger call");
  log2->log_info ("direct logger call");
  log2->log_debug("direct logger call");
  log2->log_trace("direct logger call");

  return 0;
}
\endcode

 */

#define VIDTK_DEFAULT_LOGGER __vidtk_logger__


/// @todo remove when log.h is totally removed
#ifdef log_info
#undef log_debug
#undef log_info
#undef log_warning
#undef log_error
#undef log_fatal_error
#undef log_assert
#endif

// ----------------------------------------------------------------
/** Instantiate a named logger.
 *
 *
 */
#define VIDTK_LOGGER(R) static ::vidtk::vidtk_logger_sptr VIDTK_DEFAULT_LOGGER = \
    ::vidtk::logger_manager::instance()->get_logger("vidtk."  R)


/**
Logs a message with the FATAL level. The logger defined by the
VIDTK_LOGGER macro is used to process the log message.

@param msg the message string to log.
*/
#define LOG_FATAL(msg) do {                                             \
if (VIDTK_DEFAULT_LOGGER->is_fatal_enabled()) {                         \
vcl_stringstream _oss_; _oss_ << msg;                                   \
  VIDTK_DEFAULT_LOGGER->log_fatal(_oss_.str(), VIDTK_LOGGER_SITE); }    \
} while(0)

/**
If COND is non-zero, log a message at the FATAL level.

@param cond the condition which should be met to pass the assertion
@param msg the message string to log.
*/
#define LOG_ASSERT(cond,msg) do {                                       \
if (VIDTK_DEFAULT_LOGGER->is_fatal_enabled()) {                         \
  int R = (cond); if (R == 0 ) {                                        \
  vcl_stringstream _oss_; _oss_ << "ASSERTION FAILED: " << msg;         \
  VIDTK_DEFAULT_LOGGER->log_fatal(_oss_.str(), VIDTK_LOGGER_SITE); }}   \
} while(0)

/**
Logs a message with the ERROR level. The logger defined by the
VIDTK_LOGGER macro is used to process the log message.

@param msg the message string to log.
*/
#define LOG_ERROR(msg) do {                                             \
if (VIDTK_DEFAULT_LOGGER->is_error_enabled()) {                         \
vcl_stringstream _oss_; _oss_ << msg;                                   \
  VIDTK_DEFAULT_LOGGER->log_error(_oss_.str(), VIDTK_LOGGER_SITE); }    \
} while(0)

/**
Logs a message with the WARN level. The logger defined by the
VIDTK_LOGGER macro is used to process the log message.

@param msg the message string to log.
*/
#define LOG_WARN(msg) do {                                              \
if (VIDTK_DEFAULT_LOGGER->is_warn_enabled()) {                          \
vcl_stringstream _oss_; _oss_ << msg;                                   \
  VIDTK_DEFAULT_LOGGER->log_warn(_oss_.str(), VIDTK_LOGGER_SITE); }     \
} while(0)

/**
Logs a message with the INFO level. The logger defined by the
VIDTK_LOGGER macro is used to process the log message.

@param msg the message string to log.
*/
#define LOG_INFO(msg) do {                                              \
if (VIDTK_DEFAULT_LOGGER->is_info_enabled()) {                          \
vcl_stringstream _oss_; _oss_ << msg;                                   \
  VIDTK_DEFAULT_LOGGER->log_info(_oss_.str(), VIDTK_LOGGER_SITE); }     \
} while(0)

/**
Logs a message with the DEBUG level. The logger defined by the
VIDTK_LOGGER macro is used to process the log message.

@param msg the message string to log.
*/
#define LOG_DEBUG(msg) do {                                             \
if (VIDTK_DEFAULT_LOGGER->is_debug_enabled()) {                         \
vcl_stringstream _oss_; _oss_ << msg;                                   \
  VIDTK_DEFAULT_LOGGER->log_debug(_oss_.str(), VIDTK_LOGGER_SITE); }    \
} while(0)

/**
Logs a message with the TRACE level. The logger defined by the
VIDTK_LOGGER macro is used to process the log message.

@param msg the message string to log.
*/
#define LOG_TRACE(msg) do {                                             \
if (VIDTK_DEFAULT_LOGGER->is_trace_enabled()) {                         \
vcl_stringstream _oss_; _oss_ << msg;                                   \
  VIDTK_DEFAULT_LOGGER->log_trace(_oss_.str(), VIDTK_LOGGER_SITE); }    \
} while(0)


// Log a message if condition is true
#define LOG_FATAL_IF(condition, x)  do { if (condition) { LOG_FATAL(x); }  } while (0)
#define LOG_ERROR_IF(condition, x)  do { if (condition) { LOG_ERROR(x); }  } while (0)
#define LOG_WARN_IF(condition,  x)  do { if (condition) { LOG_WARN(x); }  } while (0)
#define LOG_INFO_IF(condition,  x)  do { if (condition) { LOG_INFO(x); }  } while (0)
#define LOG_DEBUG_IF(condition, x)  do { if (condition) { LOG_DEBUG(x); }  } while (0)
#define LOG_TRACE_IF(condition, x)  do { if (condition) { LOG_TRACE(x); }  } while (0)

// Test for debugging level being enabled
#define IS_FATAL_ENABLED() (VIDTK_DEFAULT_LOGGER->is_fatal_enabled())
#define IS_ERROR_ENABLED() (VIDTK_DEFAULT_LOGGER->is_error_enabled())
#define IS_WARN_ENABLED()  (VIDTK_DEFAULT_LOGGER->is_warn_enabled())
#define IS_INFO_ENABLED()  (VIDTK_DEFAULT_LOGGER->is_info_enabled())
#define IS_DEBUG_ENABLED() (VIDTK_DEFAULT_LOGGER->is_debug_enabled())
#define IS_TRACE_ENABLED() (VIDTK_DEFAULT_LOGGER->is_trace_enabled())


#endif /* _LOGGER__API_H_ */
