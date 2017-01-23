/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <track_oracle/csv_tokenizer.h>

#include <string>
#include <sstream>
#include <vector>

#include <testlib/testlib_test.h>

#include <logger/logger.h>


using std::ostringstream;
using std::string;
using std::vector;

VIDTK_LOGGER( "test_csv_tokenizer" );

namespace { // anon

string
strip_leading_quotes( const string& s )
{
  size_t n = s.size();
  if ( n >= 2 )
  {
    if ( (s[0] == '"') && (s[n-1] == '"'))
    {
      return s.substr( 1, n-2 );
    }
  }
  return s;
}

void
test_string_array( const string& tag, const char* strings[] )
{
  string s("");
  string comma( "," );
  size_t len_true = 0;
  for (size_t i=0; strings[i] != 0; ++i)
  {
    s += string(strings[i]);
    if (strings[i+1] != 0)
    {
      s += comma;
    }
    ++len_true;
  }
  vector< string > tokens = vidtk::csv_tokenizer::parse( s );
  size_t len_found = tokens.size();
  {
    ostringstream oss;
    oss << tag << ": parse of '" << s << "' returned " << len_found << " tokens; expected " << len_true;
    TEST( oss.str().c_str(), len_found, len_true );
  }
  if ( len_true != len_found) return;

  for (size_t i=0; i<len_found; ++i)
  {
    ostringstream oss;
    string stripped_t( strip_leading_quotes( tokens[i] )), stripped_e( strip_leading_quotes( strings[i] ));
    oss << tag << ": token " << i << ": was '" << stripped_t << "'; expected '" << stripped_e << "'";
    TEST( oss.str().c_str(), stripped_t, stripped_e );
  }
}

} // anon

int test_csv_tokenizer( int, char *[] )
{
  testlib_test_start( "test_csv_tokenizer" );

  TEST( "strip-leading-quotes on 'foo'", strip_leading_quotes("foo"), "foo" );
  TEST( "strip-leading-quotes on '\"foo\"'", strip_leading_quotes("\"foo\""), "foo" );
  TEST( "strip-leading-quotes on ''", strip_leading_quotes(""), "" );
  TEST( "strip-leading-quotes on '\"\"'", strip_leading_quotes("\"\""), "" );
  TEST( "strip-leading-quotes on '\"x\"'", strip_leading_quotes("\"x\""), "x" );


  const char* no_string[] = {0};
  test_string_array( "no-string", no_string );
  const char* single_field[] = { "single field", 0 };
  test_string_array( "single-field", single_field) ;
  const char* two_fields[] = { "field 1", "field 2", 0 };
  test_string_array( "two-fields", two_fields );
  const char* three_w_blank[] = { "field 1", "", "field 2", 0};
  test_string_array( "three-w-blank", three_w_blank );

  // philosophical question: is an empty string zero tokens or a single empty token?
  // We'll go with zero tokens.
  {
    vector< string > tokens = vidtk::csv_tokenizer::parse( "" );
    ostringstream oss;
    oss << "one-all-blank: parse of '\"\"' returned " << tokens.size() << " tokens; expected 0";
    TEST( oss.str().c_str(), tokens.size(), 0 );
  }

  const char* two_all_blank[] = { "","",0 };
  test_string_array( "two-all-blank", two_all_blank );
  const char* four_all_blank[] = { "","","","",0 };
  test_string_array( "four-all-blank", four_all_blank );
  const char* quoted_comma[] = { "\"field,with,commas\"", "no comma", 0};
  test_string_array( "quoted-comma", quoted_comma );
  return testlib_test_summary();
}
