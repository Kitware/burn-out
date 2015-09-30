/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <logger/vidtk_logger.h>
#include <logger/logger_manager.h>
#include <logger/vidtk_logger_mini_logger.h>
#include <logger/vidtk_mini_logger_formatter_legacy.h>


#ifdef USE_LOG4CXX
  #include <logger/vidtk_logger_log4cxx.h>
  #include <log4cxx/patternlayout.h>
  #include <log4cxx/consoleappender.h>
#endif

::vidtk::vidtk_logger_sptr _legacy_logger_(0);

// ----------------------------------------------------------------
/** Legacy logger
 *
 * This class represents the legacy logger used to support the old
 * vidtk logging API.  It is instantiated as a single static instance
 * that is used by the expanded macro code from utilities/log.h
 *
 * This implementation provides a customized logger that tries to
 * replicate the look and feel of the old log messages.  They look
 * more like the old version of log4cxx is enabled. Otherwise, you get
 * the default output format from the mini-logger.
 */

class LegacyLogger
{
public:
  LegacyLogger()
  {
    // initialize the logger
    ::vidtk::logger_manager::instance()->initialize(0,0);

    // initialize the logger pointer
    _legacy_logger_ = ::vidtk::logger_manager::instance()->get_logger("vidtk.LegacyLogger");

    // If we are using the mini-logger, then apply the legacy formatter
    ::vidtk::logger_ns::vidtk_logger_mini_logger * mlp =
        dynamic_cast< ::vidtk::logger_ns::vidtk_logger_mini_logger* >(_legacy_logger_.ptr());
    if (mlp != 0) // down-cast worked o.k.
    {
      mlp->register_formatter ( new ::vidtk::logger_ns::vidtk_mini_logger_formatter_legacy() );
      mlp->set_level (::vidtk::vidtk_logger::LEVEL_DEBUG);
    }

  }

};


LegacyLogger s_llogger;  // create now to initialize the logger

