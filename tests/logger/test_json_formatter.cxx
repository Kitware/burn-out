/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <logger/logger.h>
#include <logger/vidtk_mini_logger_formatter_json.h>
#include <logger/vidtk_logger_mini_logger.h>
#include <vcl_iostream.h>

#include <json.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace libjson;

// custom testing class
class my_json : public vidtk::logger_ns::vidtk_mini_logger_formatter_json
{
public:

  virtual void format_message(vcl_ostream& str)
  {
    vidtk::logger_ns::vidtk_mini_logger_formatter_json::format_message (_oss_);
  }


  vcl_stringstream _oss_;
};

#if 0 // used to debug the test

// ----------------------------------------------------------------
/**
 *
 *
 */
void ParseJSON(const JSONNode& n)
{
  JSONNode::const_iterator i = n.begin();

  while ( i != n.end() )
  {
    // recursively call ourselves to dig deeper into the tree
    if (i->type() == JSON_ARRAY || i->type() == JSON_NODE)
    {
      ParseJSON(*i);
    }

    // get the node name and value as a string
    std::string node_name = i->name();
    vcl_cout << "name: " << node_name << "  ->  "
             << i->as_string()
             << vcl_endl;

    //increment the iterator
    ++i;
  }
}

#endif

// ----------------------------------------------------------------
/* Test json formatter.
 *
 *
 */
void test_formatter()
{
  // create a new logger
  vidtk::vidtk_logger_sptr log =  vidtk::logger_manager::instance()->get_logger("json_logger");

  // If we are not using the mini-logger,
  ::vidtk::logger_ns::vidtk_logger_mini_logger * mlp =
    dynamic_cast< ::vidtk::logger_ns::vidtk_logger_mini_logger* >(log.ptr());
  if (mlp == 0) // down-cast failed
  {
    return; // not mini-logger
  }

  // Create new JSON formatter
  my_json * jfmt (new my_json() );

  TEST ( "Setting JSON formatter", set_mini_logger_formatter (log, jfmt ), true );

  // Generate formatted log message
  log->log_error("test json message", VIDTK_LOGGER_SITE);

  // print raw logger output
  // vcl_cout << jfmt->_oss_.str() << vcl_endl;

  JSONNode msg ( parse(jfmt->_oss_.str()) );
  JSONNode jn = msg.at("log_entry");

  TEST ("Logger name",        jn.at("logger").as_string(), "json_logger");
  TEST ("Logger level",       jn.at("level").as_string(), "ERROR");
  TEST ("Logger filename",    jn.at("filename").as_string(), "test_json_formatter.cxx");
  TEST ("Logger method name", jn.at("method_name").as_string(), "test_formatter");
  TEST ("Logger message",     jn.at("message").as_string(), "test json message");
}

} // end namespace

// ----------------------------------------------------------------
/** Main test driver for this file.
 *
 *
 */
int test_json_formatter(int argc, char * argv[])
{
  testlib_test_start( "json formatter");

  test_formatter();

  return testlib_test_summary();
}

