/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <logger/vidtk_mini_logger_formatter_legacy.h>

namespace vidtk {
namespace logger_ns {

vidtk_mini_logger_formatter_legacy::
vidtk_mini_logger_formatter_legacy()
{

}


vidtk_mini_logger_formatter_legacy::
~vidtk_mini_logger_formatter_legacy()
{

}

void vidtk_mini_logger_formatter_legacy::
format_message(vcl_ostream& str)
{
  // Format this message on the stream
  str << get_level() << " "
      << get_message();
}


} // end namespace
} // end namespace
