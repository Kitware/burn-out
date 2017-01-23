/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <object_detectors/text_parser.h>

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>

#include <testlib/testlib_test.h>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <algorithm>
#include <string>

namespace
{

using namespace vidtk;

void test_text_parser( std::string template_dir, std::string image_path )
{
  vil_image_view<vxl_byte> image = vil_load( image_path.c_str() );

  if( !image )
  {
    // Assume we don't have png support so this test is invalid, exit.
    return;
  }

  // Test standalone model class
  {
    text_parser_model_group<vxl_byte> models;

    TEST( "Test model loading", models.load_templates( template_dir ), true );
    TEST( "Test model count", models.templates.size(), 5 );
  }

  // Test parser class in template matching mode
  {
    text_parser<vxl_byte> parser;
    text_parser_settings settings;

    settings.model_directory = template_dir;
    settings.algorithm = text_parser_settings::TEMPLATE_MATCHING;
    settings.vertical_jitter = 1;

    TEST( "TM - Test initial configure", parser.configure( settings ), true );

    text_parser_instruction<vxl_byte> instruction;
    instruction.region = text_parser_instruction<vxl_byte>::region_t( 0, 1, 0, 1 );

    std::string output = parser.parse_text( image, instruction );

    TEST( "TM - Output size", output.size(), 7 );
    TEST( "TM - Output contents", output, "ABADBED" );
  }
}

} // end anonymous namespace

int test_text_parser( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Test must be supplied a data directory" << std::endl;
    return 1;
  }

  std::string data_dir = argv[1];

  testlib_test_start( "text_parser" );

  test_text_parser( data_dir + "/character_templates",
                    data_dir + "/simple_text_image.png" );

  return testlib_test_summary();
}
