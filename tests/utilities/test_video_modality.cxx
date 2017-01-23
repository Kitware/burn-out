/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/video_modality.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_enum_to_string()
{
  TEST("invalid (string test)", video_modality_to_string(VIDTK_INVALID), "INVALID_TYPE");
  TEST("VIDTK_COMMON (string test)", video_modality_to_string(VIDTK_COMMON), "Common");
  TEST("VIDTK_EO_N (string test)", video_modality_to_string(VIDTK_EO_N), "EO_NFOV");
  TEST("VIDTK_EO_M (string test)", video_modality_to_string(VIDTK_EO_M), "EO_MFOV");
  TEST("VIDTK_IR_N (string test)", video_modality_to_string(VIDTK_IR_N), "IR_NFOV");
  TEST("VIDTK_IR_M (string test)", video_modality_to_string(VIDTK_IR_M), "IR_MFOV");
  TEST("VIDTK_EO_FB (string test)", video_modality_to_string(VIDTK_EO_FB), "EO_FALLBACK");
  TEST("VIDTK_IR_FB (string test)", video_modality_to_string(VIDTK_IR_FB), "IR_FALLBACK");
  TEST("VIDTK_EO (string test)", video_modality_to_string(VIDTK_EO), "EO_?");
  TEST("VIDTK_IR (string test)", video_modality_to_string(VIDTK_IR), "IR_?");
  TEST("VIDTK_VIDEO_MODALITY_END (string test)", video_modality_to_string(VIDTK_VIDEO_MODALITY_END), "INVALID_TYPE");
}

void test_is_ir()
{
  TEST("invalid (is ir test)", is_video_modality_ir(VIDTK_INVALID), false);
  TEST("VIDTK_COMMON (is ir test)", is_video_modality_ir(VIDTK_COMMON), false);
  TEST("VIDTK_EO_N (is ir test)", is_video_modality_ir(VIDTK_EO_N), false);
  TEST("VIDTK_EO_M (is ir test)", is_video_modality_ir(VIDTK_EO_M), false);
  TEST("VIDTK_IR_N (is ir test)", is_video_modality_ir(VIDTK_IR_N), true);
  TEST("VIDTK_IR_M (is ir test)", is_video_modality_ir(VIDTK_IR_M), true);
  TEST("VIDTK_EO_FB (is ir test)", is_video_modality_ir(VIDTK_EO_FB), false);
  TEST("VIDTK_IR_FB (is ir test)", is_video_modality_ir(VIDTK_IR_FB), true);
  TEST("VIDTK_EO (is ir test)", is_video_modality_ir(VIDTK_EO), false);
  TEST("VIDTK_IR (is ir test)", is_video_modality_ir(VIDTK_IR), true);
  TEST("VIDTK_VIDEO_MODALITY_END (is ir test)", is_video_modality_ir(VIDTK_VIDEO_MODALITY_END), false);
}

void test_is_eo()
{
  TEST("invalid (is eo test)", is_video_modality_eo(VIDTK_INVALID), false);
  TEST("VIDTK_COMMON (is eo test)", is_video_modality_eo(VIDTK_COMMON), true);
  TEST("VIDTK_EO_N (is eo test)", is_video_modality_eo(VIDTK_EO_N), true);
  TEST("VIDTK_EO_M (is eo test)", is_video_modality_eo(VIDTK_EO_M), true);
  TEST("VIDTK_IR_N (is eo test)", is_video_modality_eo(VIDTK_IR_N), false);
  TEST("VIDTK_IR_M (is eo test)", is_video_modality_eo(VIDTK_IR_M), false);
  TEST("VIDTK_EO_FB (is eo test)", is_video_modality_eo(VIDTK_EO_FB), true);
  TEST("VIDTK_IR_FB (is eo test)", is_video_modality_eo(VIDTK_IR_FB), false);
  TEST("VIDTK_EO (is eo test)", is_video_modality_eo(VIDTK_EO), true);
  TEST("VIDTK_IR (is eo test)", is_video_modality_eo(VIDTK_IR), false);
  TEST("VIDTK_VIDEO_MODALITY_END (is eo test)", is_video_modality_eo(VIDTK_VIDEO_MODALITY_END), false);
}

void test_is_invalid()
{
  TEST("invalid (is invalid test)", is_video_modality_invalid(VIDTK_INVALID), true);
  TEST("VIDTK_COMMON (is invalid test)", is_video_modality_invalid(VIDTK_COMMON), false);
  TEST("VIDTK_EO_N (is invalid test)", is_video_modality_invalid(VIDTK_EO_N), false);
  TEST("VIDTK_EO_M (is invalid test)", is_video_modality_invalid(VIDTK_EO_M), false);
  TEST("VIDTK_IR_N (is invalid test)", is_video_modality_invalid(VIDTK_IR_N), false);
  TEST("VIDTK_IR_M (is invalid test)", is_video_modality_invalid(VIDTK_IR_M), false);
  TEST("VIDTK_EO_FB (is invalid test)", is_video_modality_invalid(VIDTK_EO_FB), false);
  TEST("VIDTK_IR_FB (is invalid test)", is_video_modality_invalid(VIDTK_IR_FB), false);
  TEST("VIDTK_EO (is invalid test)", is_video_modality_invalid(VIDTK_EO), false);
  TEST("VIDTK_IR (is invalid test)", is_video_modality_invalid(VIDTK_IR), false);
  TEST("VIDTK_VIDEO_MODALITY_END (is invalid test)", is_video_modality_invalid(VIDTK_VIDEO_MODALITY_END), true);
}

} // end anonymous namespace

int test_video_modality( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "ring_buffer_process" );

  test_enum_to_string();
  test_is_ir();
  test_is_eo();
  test_is_invalid();

  std::cout << VIDTK_COMMON << std::endl;

  return testlib_test_summary();
}
