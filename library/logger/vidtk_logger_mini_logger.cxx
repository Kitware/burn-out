/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <logger/vidtk_logger_mini_logger.h>

#include <logger/vidtk_mini_logger_formatter.h>
#include <logger/vidtk_mini_logger_formatter_int.h>

#include <logger/logger_manager.h>
#include <logger/location_info.h>
#include <vcl_iostream.h>
#include <vpl/vpl.h>

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>


namespace vidtk {
namespace logger_ns {


vidtk_logger_mini_logger
::vidtk_logger_mini_logger (char const* realm)
  :vidtk_logger(realm),
   m_logLevel(vidtk_logger::LEVEL_TRACE),
   m_formatter(0)
{

}


vidtk_logger_mini_logger
::~vidtk_logger_mini_logger()
{
  delete m_formatter;
}


// ----------------------------------------------------------------
/* Test current log level.
 *
 *
 */
bool vidtk_logger_mini_logger
::is_fatal_enabled() const
{
  return (m_logLevel <= vidtk_logger::LEVEL_FATAL);
}


bool vidtk_logger_mini_logger
::is_error_enabled() const
{
  return (m_logLevel <= vidtk_logger::LEVEL_ERROR);
}


bool vidtk_logger_mini_logger
::is_warn_enabled() const
{
  return (m_logLevel <= vidtk_logger::LEVEL_WARN);
}


bool vidtk_logger_mini_logger
::is_info_enabled() const
{
  return (m_logLevel <= vidtk_logger::LEVEL_INFO);
}


  bool vidtk_logger_mini_logger
::is_debug_enabled() const
{
  return (m_logLevel <= vidtk_logger::LEVEL_DEBUG);
}


bool vidtk_logger_mini_logger
::is_trace_enabled() const
{
  return (m_logLevel <= vidtk_logger::LEVEL_TRACE);
}

// ----------------------------------------------------------------
/* get / set log level
 *
 *
 */
void vidtk_logger_mini_logger
::set_level( vidtk_logger::log_level_t lvl)
{
  m_logLevel = lvl;
}


vidtk_logger::log_level_t vidtk_logger_mini_logger
::get_level () const
{
  return m_logLevel;
}


// ----------------------------------------------------------------
/** Get logging stream.
 *
 * This method returns the stream used to write log messages.
 */
vcl_ostream& vidtk_logger_mini_logger
::get_stream()
{
  return vcl_cerr;
}


// ----------------------------------------------------------------
/* Log messages
 *
 *
 */

void vidtk_logger_mini_logger
::log_fatal (vcl_string const & msg)
{
  if (is_fatal_enabled())
  {
    log_message (LEVEL_FATAL, msg);
  }
}


void vidtk_logger_mini_logger
::log_fatal (vcl_string const & msg, vidtk::logger_ns::location_info const & location)
{
  if (is_fatal_enabled())
  {
    log_message (LEVEL_FATAL, msg, location);
  }
}


void vidtk_logger_mini_logger
::log_error (vcl_string const & msg)
{
  if (is_error_enabled())
  {
    log_message (LEVEL_ERROR, msg);
  }
}


void vidtk_logger_mini_logger
::log_error (vcl_string const & msg, vidtk::logger_ns::location_info const & location)
{
  if (is_error_enabled())
  {
    log_message (LEVEL_ERROR, msg, location);
  }
}


void vidtk_logger_mini_logger
::log_warn (vcl_string const & msg)
{
  if (is_warn_enabled())
  {
    log_message (LEVEL_WARN, msg);
  }
}


void vidtk_logger_mini_logger
::log_warn (vcl_string const & msg, vidtk::logger_ns::location_info const & location)
{
  if (is_warn_enabled())
  {
    log_message (LEVEL_WARN, msg, location);
  }
}



void vidtk_logger_mini_logger
::log_info (vcl_string const & msg)
{
  if (is_info_enabled())
  {
    log_message (LEVEL_INFO, msg);
  }

}


void vidtk_logger_mini_logger
::log_info (vcl_string const & msg, vidtk::logger_ns::location_info const & location)
{
  if (is_info_enabled())
  {
    log_message (LEVEL_INFO, msg, location);
  }
}



void vidtk_logger_mini_logger
::log_debug (vcl_string const & msg)
{
  if (is_debug_enabled())
  {
    log_message (LEVEL_DEBUG, msg);
  }
}


void vidtk_logger_mini_logger
::log_debug (vcl_string const & msg, vidtk::logger_ns::location_info const & location)
{
  if (is_debug_enabled())
  {
    log_message (LEVEL_DEBUG, msg, location);
  }
}



void vidtk_logger_mini_logger
::log_trace (vcl_string const & msg)
{
  if (is_trace_enabled())
  {
    log_message (LEVEL_TRACE, msg);
  }
}


void vidtk_logger_mini_logger
::log_trace (vcl_string const & msg, vidtk::logger_ns::location_info const & location)
{
  if (is_trace_enabled())
  {
    log_message (LEVEL_TRACE, msg, location);
  }
}


// ----------------------------------------------------------------
/** Generic message writer.
 *
 *
 */
void vidtk_logger_mini_logger
::log_message ( vidtk_logger::log_level_t level, vcl_string const& msg)
{
  // If a formatter is specified, then use it.
  if (0 != m_formatter)
  {
    log_message (level, msg, *m_formatter);
    return;
  }


  boost::lock_guard<boost::mutex> lock(m_lock);

  // Format this message on the stream
  get_stream() << get_level_string(level) << " " << msg << "\n";
}


// ----------------------------------------------------------------
/** Format log message.
 *
 *
 */
void vidtk_logger_mini_logger
::log_message ( vidtk_logger::log_level_t level, vcl_string const& msg,
              vidtk::logger_ns::location_info const & location)
{
  // If a formatter is specified, then use it.
  if (0 != m_formatter)
  {
    log_message (level, msg, location, *m_formatter);
    return;
  }


  boost::lock_guard<boost::mutex> lock(m_lock);

  // Format this message on the stream
  get_stream() << get_level_string(level) << " " << msg << "\n";
}


// ----------------------------------------------------------------
/** Register log message formatter object
 *
 *
 */
void vidtk_logger_mini_logger
::register_formatter (vidtk_mini_logger_formatter * fmt)
{
  // delete any existing formatter.
  delete m_formatter;

  m_formatter = fmt;

  fmt->get_impl()->m_parent = m_parent;
  fmt->get_impl()->m_logger = this;
}


// ----------------------------------------------------------------
/** Print log message using supplied formatter.
 *
 *
 */
void vidtk_logger_mini_logger
::log_message ( vidtk_logger::log_level_t level, vcl_string const& msg,
              vidtk::logger_ns::vidtk_mini_logger_formatter & formatter)
{
  boost::lock_guard<boost::mutex> lock(m_lock);

  formatter.get_impl()->m_level = level;
  formatter.get_impl()->m_message = &msg;
  formatter.get_impl()->m_location = 0;
  formatter.get_impl()->m_realm = &m_loggingRealm;

  formatter.format_message (get_stream() );
}


// ----------------------------------------------------------------
/** Print log message using supplied formatter.
 *
 *
 */
void vidtk_logger_mini_logger
::log_message ( vidtk_logger::log_level_t level, vcl_string const& msg,
              vidtk::logger_ns::location_info const & location,
              vidtk::logger_ns::vidtk_mini_logger_formatter & formatter)
{
  boost::lock_guard<boost::mutex> lock(m_lock);

  formatter.get_impl()->m_level = level;
  formatter.get_impl()->m_message = &msg;
  formatter.get_impl()->m_location = &location;
  formatter.get_impl()->m_realm = &m_loggingRealm;

  formatter.format_message (get_stream() );
}


} // end namespace
} // end namespace
