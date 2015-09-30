/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_LOG_H__
#define _VIDTK_LOG_H__

#include <logger/vidtk_logger.h>
#include <logger/location_info.h>
#include <vcl_cstdlib.h>
#include <vcl_sstream.h>


// NOTE: could also implement _legacy_logger_ as a singleton
// If we had a good singleton implementation.

extern ::vidtk::vidtk_logger_sptr _legacy_logger_;

//
// This header file implements an adapter wrapper around the new logging
// that supports the old logging API.
//

/// Log a debugging message.
///
/// \deprecated Use LOG_DEBUG() from logger/logger.h
///
/// These messages are intended for short term debugging, and
/// shouldn't remain in the code forever.
///
/// See the documentation of log.h for more information.
#define log_debug(X) do {                                       \
if (_legacy_logger_->is_debug_enabled()) {                      \
vcl_stringstream _oss_; _oss_ << X;                             \
  _legacy_logger_->log_debug(_oss_.str(), VIDTK_LOGGER_SITE); } \
} while(0)


/// Log an informational message.
///
/// \deprecated Use LOG_INFO() from logger/logger.h
///
/// These messages are intended for somewhat infrequent,
/// not-too-verbose informational messages, such as progress on long
/// computations.
///
/// See the documentation of log.h for more information.
#define log_info( X ) do {                                      \
if (_legacy_logger_->is_info_enabled()) {                       \
vcl_stringstream _oss_; _oss_ << X;                             \
  _legacy_logger_->log_info(_oss_.str(), VIDTK_LOGGER_SITE); }  \
} while(0)

/// Log a warning message.
///
/// \deprecated Use LOG_WARN() from logger/logger.h
///
/// See the documentation of log.h for more information.
#define log_warning( X ) do {                                   \
if (_legacy_logger_->is_warn_enabled()) {                       \
vcl_stringstream _oss_; _oss_ << X;                             \
  _legacy_logger_->log_warn(_oss_.str(), VIDTK_LOGGER_SITE); }  \
} while(0)

/// Log an error message.
///
/// \deprecated Use LOG_ERROR() from logger/logger.h
///
/// See the documentation of log.h for more information.
#define log_error( X )  do {                                    \
if (_legacy_logger_->is_error_enabled()) {                      \
vcl_stringstream _oss_; _oss_ << X;                             \
  _legacy_logger_->log_error(_oss_.str(), VIDTK_LOGGER_SITE); } \
} while(0)

/// Log an error message and abort.
///
/// \deprecated Use LOG_FATAL() from logger/logger.h
///
/// See the documentation of log.h for more information.
#define log_fatal_error( X )  do {                              \
if (_legacy_logger_->is_fatal_enabled()) {                      \
vcl_stringstream _oss_; _oss_ << X;                             \
  _legacy_logger_->log_fatal(_oss_.str(), VIDTK_LOGGER_SITE); } \
vcl_abort();                                                    \
} while(0)


/// \def log_assert(COND,MSG)
///
/// \deprecated Use macro from logger/logger.h
///
/// Similar to the standard \c assert macro.  In a non-release build,
/// if COND is non-zero, calls log_fatal_error on MSG (which logs the
/// error and aborts the program).  In a release build, it does
/// nothing.

#ifndef NDEBUG
#  define log_assert( COND, MSG ) do { int R = ( COND ); if( R == 0 ) { log_fatal_error( MSG << vcl_endl ); } } while(0)
#else
#  define log_assert( COND, MSG )
#endif

#endif /* _VIDTK_LOG_H__ */
