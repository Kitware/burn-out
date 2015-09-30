/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <logger/vidtk_mini_logger_formatter_json.h>

namespace vidtk {
namespace logger_ns {

vidtk_mini_logger_formatter_json
::vidtk_mini_logger_formatter_json()
{

}


vidtk_mini_logger_formatter_json
::~vidtk_mini_logger_formatter_json()
{

}

void vidtk_mini_logger_formatter_json
::format_message(vcl_ostream& str)
{
  str << "{ \"log_entry\" : {" << vcl_endl
      << "    \"system_name\" : \"" << get_system_name() << "\"," << vcl_endl
      << "    \"application\" : \"" << get_application_name() << "\"," << vcl_endl
      << "    \"application_instance\" : \"" << get_application_instance_name() << "\"," << vcl_endl
      << "    \"logger\" : \"" << get_realm() << "\"," << vcl_endl
      << "    \"pid\" : \"" << get_pid() << "\"," << vcl_endl
      << "    \"level\" : \"" << get_level() << "\"," << vcl_endl
      << "    \"path\" : \"" << get_file_path() << "\"," << vcl_endl
      << "    \"filename\" : \"" << get_file_name() << "\"," << vcl_endl
      << "    \"line_number\" : " << get_line_number() << "," << vcl_endl
      << "    \"class_name\" : \"" << get_class_name() << "\"," << vcl_endl
      << "    \"method_name\" : \"" << get_method_name() << "\"," << vcl_endl
      << "    \"message\" : \"" << get_message() << "\"" << vcl_endl
      << "} }"  << vcl_endl;
}


} // end namespace
} // end namespace
