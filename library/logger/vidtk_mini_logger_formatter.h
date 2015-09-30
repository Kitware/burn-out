/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _VIDTK_MINI_LOGGER_FORMATTER_H_
#define _VIDTK_MINI_LOGGER_FORMATTER_H_

#include <vcl_string.h>
#include <vcl_ostream.h>

namespace vidtk {
namespace logger_ns {


class formatter_impl;


// ----------------------------------------------------------------
/** Base formatter class
 *
 * This class represents the base class for formatting log messages.
 */
class vidtk_mini_logger_formatter
{
public:
  vidtk_mini_logger_formatter();
  virtual ~vidtk_mini_logger_formatter();

  virtual void format_message(vcl_ostream& str) = 0;

  formatter_impl * get_impl();


protected:

  vcl_string const& get_application_instance_name() const;
  vcl_string const& get_application_name() const;
  vcl_string const& get_system_name() const;
  vcl_string const& get_realm() const;
  vcl_string get_level() const;
  vcl_string get_pid() const;

  vcl_string get_file_name() const;
  vcl_string get_file_path() const;
  vcl_string get_line_number() const;
  vcl_string get_class_name() const;
  vcl_string get_method_name() const;

  vcl_string const& get_message() const;

private:
  formatter_impl * m_impl;


}; // end class vidtk_mini_logger_formatter

} // end namespace
} // end namespace


#endif /* _VIDTK_MINI_LOGGER_FORMATTER_H_ */
