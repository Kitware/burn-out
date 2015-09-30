
#include <vcl_string.h>
#include <utilities/token_expansion.h>
#include <testlib/testlib_test.h>
#include <algorithm>

int test_token_expansion( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start("token_expansion");
  vcl_string no_tok = "/test/path/to/file";
  vcl_string result = vidtk::token_expansion::expand_token( no_tok );
  TEST("String without token pattern remains unchanged", no_tok == result, true);
  vcl_string wrong_token = "$ENV{/test/path/to/file";
  result = vidtk::token_expansion::expand_token( wrong_token );
  TEST("String with incorrect token pattern remains unchanged", wrong_token == result, true);

  vcl_string path_env = getenv("PATH");
  vcl_string final_correct = path_env + "/test/path/to/file.txt";
  vcl_string correct_token = "$ENV{PATH}/test/path/to/file.txt";
  result = vidtk::token_expansion::expand_token( correct_token );
  TEST("String with correct token pattern matches expected", final_correct == result, true);

  return testlib_test_summary();
}
