/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <string>
#include <utilities/token_expansion.h>
#include <testlib/testlib_test.h>
#include <algorithm>

int test_token_expansion( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start("token_expansion");
  std::string no_tok = "/test/path/to/file";
  std::string result = vidtk::token_expansion::expand_token( no_tok );
  TEST("String without token pattern remains unchanged", no_tok == result, true);
  std::string wrong_token = "$ENV{/test/path/to/file";
  result = vidtk::token_expansion::expand_token( wrong_token );
  TEST("String with incorrect token pattern remains unchanged", wrong_token == result, true);

  std::string path_env = getenv("PATH");
  std::string final_correct = path_env + "/test/path/to/file.txt";
  std::string correct_token = "$ENV{PATH}/test/path/to/file.txt";
  result = vidtk::token_expansion::expand_token( correct_token );
  TEST("String with correct token pattern matches expected", final_correct == result, true);

  std::string missing_token = "$ENV{missing_tok}";
  result = vidtk::token_expansion::expand_token( missing_token );
  TEST("Well-formed token that doesn't exit in env returns original", missing_token == result, true);

  std::string trailing_text = missing_token + "junk";
  result = vidtk::token_expansion::expand_token( trailing_text );
  TEST("Well-formed token with data afterwards that doesn't exit in env returns original", trailing_text == result, true);

  final_correct = path_env + "/test/path/to/file.txt/" + missing_token;
  std::string mixed_token = "$ENV{PATH}/test/path/to/file.txt/" + missing_token;
  result = vidtk::token_expansion::expand_token( mixed_token );
  TEST("String with correct and incorrect token pattern matches expected", final_correct == result, true);

  final_correct = missing_token + path_env;
  std::string mixed_token2 = missing_token + "$ENV{PATH}";
  result = vidtk::token_expansion::expand_token( mixed_token2 );
  TEST("String with incorrect and correct token pattern matches expected", final_correct == result, true);

  final_correct = "/test/path/" + path_env + "/to/file.txt";
  correct_token = "/test/path/$ENV{PATH}/to/file.txt";
  result = vidtk::token_expansion::expand_token( correct_token );
  TEST("String with token in the middle matches expected", final_correct == result, true);

  return testlib_test_summary();
}
