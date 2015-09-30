/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_MINI_LOGGER_FORMATTER_LEGACY_H_
#define _VIDTK_MINI_LOGGER_FORMATTER_LEGACY_H_

#include <logger/vidtk_mini_logger_formatter.h>

namespace vidtk {
namespace logger_ns {

// ----------------------------------------------------------------
/** Legacy formatter for mini logger.
 *
 * This formatter generates a printeble log message that is compatible
 * with the historic VIDTK log macro usage. It does not append a
 * new-line to the end fo the message because, historically, the "\n"
 * is included in the message text.
 */
class vidtk_mini_logger_formatter_legacy
  : public vidtk_mini_logger_formatter
{
public:
  vidtk_mini_logger_formatter_legacy();
  virtual ~vidtk_mini_logger_formatter_legacy();

  virtual void format_message(vcl_ostream& str);

}; // end class vidtk_mini_logger_formatter_legacy

} // end namespace
} // end namespace

#endif /* _VIDTK_MINI_LOGGER_FORMATTER_LEGACY_H_ */
