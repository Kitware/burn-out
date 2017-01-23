/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

#include <testlib/testlib_test.h>

#include <pipeline_framework/async_pipeline.h>

#include <video_properties/border_detection_process.h>

#include <iostream>
#include <algorithm>
#include <string>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


void
run_border_tests( const std::string& data_dir )
{
  // Fill in some dummy values, an image with a black border on the left
  // side and another with a white border on some parts of the right side
  vil_image_view<vxl_byte> blackb( 20, 20 );
  vil_image_view<vxl_byte> whiteb( 20, 20 );
  vil_image_view<vxl_byte> mixedb( 20, 20 );

  blackb.fill( 100 );
  whiteb.fill( 100 );
  mixedb.fill( 100 );

  for( unsigned i = 0; i < 20; i++ )
  {
    blackb( i, 0 ) = 0;
    mixedb( i, 0 ) = 0;
    whiteb( i, 19 ) = 255;
  }

  for( unsigned j = 0; j < 20; j++ )
  {
    blackb( 0, j ) = 0;
    whiteb( 19, j ) = 255;
    mixedb( 19, j ) = 255;
  }

  mixedb( 5, 1 ) = 0;
  mixedb( 5, 2 ) = 255;

  // Create a test process
  border_detection_process< vxl_byte > border_process ("border_detector");
  config_block default_params = border_process.params();

  // Test simple border detection (black)
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "black" );
    test_setup.set( "min_border_left", "0" );
    test_setup.set( "min_border_right", "0" );
    test_setup.set( "min_border_top", "0" );
    test_setup.set( "min_border_bottom", "0" );
    test_setup.set( "side_dilation", "0" );
    border_process.initialize();
    border_process.set_params( test_setup );

    border_process.set_source_gray_image( blackb );
    border_process.step();
    vil_image_view< bool > mask = border_process.border_mask();

    TEST( "Border Detection Test: Output Size", mask.ni() == 20 && mask.nplanes() == 1, true );
    TEST( "Border Detection Test: Black Border Value 1", mask(0,0), true );
    TEST( "Border Detection Test: Black Border Value 2", mask(1,1), false );
    TEST( "Border Detection Test: Black Border Value 3", mask(0,15), true );
    TEST( "Border Detection Test: Black Border Value 4", mask(19,19), false );
  }

  // Test simple border detection (fixed)
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "fixed" );
    test_setup.set( "min_border_left", "4" );
    test_setup.set( "min_border_right", "4" );
    test_setup.set( "min_border_top", "4" );
    test_setup.set( "min_border_bottom", "4" );
    border_process.initialize();
    border_process.set_params( test_setup );

    border_process.set_source_gray_image( blackb );
    border_process.step();
    vil_image_view< bool > mask = border_process.border_mask();

    TEST( "Border Detection Test: Fixed Value 1", mask(3,3), true );
    TEST( "Border Detection Test: Fixed Value 2", mask(4,4), false );
    TEST( "Border Detection Test: Fixed Value 3", mask(4,16), true );
    TEST( "Border Detection Test: Fixed Value 4", mask(4,15), false );
  }

  // Test simple border detection (auto)
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "auto" );
    test_setup.set( "min_border_left", "0" );
    test_setup.set( "min_border_right", "0" );
    test_setup.set( "min_border_top", "0" );
    test_setup.set( "min_border_bottom", "0" );
    test_setup.set( "side_dilation", "1" );
    border_process.initialize();
    border_process.set_params( test_setup );

    border_process.set_source_gray_image( blackb );
    border_process.step();
    vil_image_view< bool > mask = border_process.border_mask();

    TEST( "Border Detection Test: Auto Border Value 1", mask(0,0), true );
    TEST( "Border Detection Test: Auto Border Value 2", mask(1,1), true );
    TEST( "Border Detection Test: Auto Border Value 3", mask(3,3), false );
    TEST( "Border Detection Test: Auto Border Value 4", mask(18,18), false );
  }

  // Test simple border detection (white)
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "white" );
    test_setup.set( "min_border_left", "0" );
    test_setup.set( "min_border_right", "0" );
    test_setup.set( "min_border_top", "0" );
    test_setup.set( "min_border_bottom", "0" );
    test_setup.set( "side_dilation", "0" );
    border_process.initialize();
    border_process.set_params( test_setup );

    border_process.set_source_gray_image( whiteb );
    border_process.step();
    vil_image_view< bool > mask = border_process.border_mask();

    TEST( "Border Detection Test: White Border Value 1", mask(0,0), false );
    TEST( "Border Detection Test: White Border Value 2", mask(4,19), true );
    TEST( "Border Detection Test: White Border Value 3", mask(19,19), true );
    TEST( "Border Detection Test: White Border Value 4", mask(4,18), false );
  }

  // Test non-rectilinear black and white border detection
  {
    config_block test_setup = default_params;
    test_setup.set( "type", "bw" );
    border_process.initialize();
    border_process.set_params( test_setup );

    border_process.set_source_gray_image( mixedb );
    border_process.step();
    vil_image_view< bool > mask = border_process.border_mask();

    TEST( "Border Detection Test: Non-rect Border Value 1", mask(0,0), true );
    TEST( "Border Detection Test: Non-rect Border Value 2", mask(19,4), true );
    TEST( "Border Detection Test: Non-rect Border Value 3", mask(19,19), true );
    TEST( "Border Detection Test: Non-rect Border Value 4", mask(5,1), true );
    TEST( "Border Detection Test: Non-rect Border Value 5", mask(5,2), true );
    TEST( "Border Detection Test: Non-rect Border Value 6", mask(5,3), false );
  }

  // Test lair data
  {
    std::string lair_image_path = data_dir + "/lair-border-ex.png";
    std::string lair_gt_path = data_dir + "/lair-border-ex-gt.png";

    vil_image_view< vxl_byte > lair_image = vil_load( lair_image_path.c_str() );
    vil_image_view< vxl_byte > lair_gt = vil_load( lair_gt_path.c_str() );

    config_block test_setup = default_params;
    test_setup.set( "type", "bw" );

    border_process.initialize();
    border_process.set_params( test_setup );

    border_process.set_source_gray_image( lair_image );
    border_process.step();
    vil_image_view< bool > mask = border_process.border_mask();

    bool are_images_equal = true;

    for( unsigned j = 0; j < lair_gt.nj(); j++ )
    {
      for( unsigned i = 0; i < lair_gt.ni(); i++ )
      {
        if( !lair_gt(i,j) == mask(i,j) )
        {
          are_images_equal = false;
        }
      }
    }

    TEST( "Border Detection Test: Does Lair Mask Match GT?", are_images_equal, true );
  }
}

} // end anonymous namespace

int test_border_detection_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  std::string data_dir = argv[1];

  testlib_test_start( "border_detection" );

  run_border_tests( data_dir );

  return testlib_test_summary();
}
