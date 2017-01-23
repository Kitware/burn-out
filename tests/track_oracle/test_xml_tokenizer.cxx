/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <map>
#include <sstream>

#include <track_oracle/xml_tokenizer.h>
#include <testlib/testlib_test.h>
#include <logger/logger.h>

VIDTK_LOGGER("test_xml_tokenizer");

using std::vector;
using std::string;
using std::map;
using std::ostringstream;
using vidtk::xml_tokenizer;


namespace // anon
{

void test_tokenizer( const string& dir )
{
  map< string, size_t > tests;
  tests[ "track_oracle_xml_token_test.0.txt" ] = 0;
  tests[ "track_oracle_xml_token_test.1.txt" ] = 1;
  tests[ "track_oracle_xml_token_test.2a.txt" ] = 2;
  tests[ "track_oracle_xml_token_test.2b.txt" ] = 2;
  tests[ "track_oracle_xml_token_test.2c.txt" ] = 2;
  tests[ "track_oracle_xml_token_test.2d.txt" ] = 2;
  tests[ "track_oracle_xml_token_test.7.txt" ] = 7;
  tests[ "track_oracle_xml_token_test.9.txt" ] = 9;

  for (map<string, size_t>::const_iterator i = tests.begin(); i != tests.end(); ++i)
  {
    vector<string> t = xml_tokenizer::first_n_tokens( dir+"/"+i->first, i->second+1 );
    ostringstream oss;
    oss << "String '" << i->first << "': found " << t.size() << " tokens; expected " << i->second;
    TEST( oss.str().c_str(), t.size(), i->second );
  }
}

} // anon

int test_xml_tokenizer( int argc, char *argv[] )
{
  if(argc < 2)
  {
    LOG_ERROR("Need the data directory as argument\n");
    return EXIT_FAILURE;
  }

  test_tokenizer( argv[1] );
  return testlib_test_summary();
}
