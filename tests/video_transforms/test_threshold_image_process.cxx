/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vil/vil_image_view.h>
#include <testlib/testlib_test.h>

#include <video_transforms/threshold_image_process.h>
#include <utilities/config_block.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;


void
test_default_config()
{
  vcl_cout << "\n\nTest default parameters are correct\n\n\n";

  threshold_image_process<vxl_byte> proc( "proc" );

  config_block blk = proc.params();
  blk.set( "threshold", 127 );
  TEST( "Configure", proc.set_params( blk ), true );
}


void
test_persist_output()
{
  vcl_cout << "\n\nTest threshold with persist\n\n\n";

  threshold_image_process<vxl_byte> proc( "proc" );

  config_block blk = proc.params();
  blk.set( "persist_output", "true" );
  blk.set( "threshold", 127 );

  TEST( "Configure", proc.set_params( blk ), true );
  TEST( "Initialize", proc.initialize(), true );

  vil_image_view<vxl_byte> src_img( 2, 2);
  src_img( 0, 0 ) = 0;
  src_img( 0, 1 ) = 160;
  src_img( 1, 0 ) = 64;
  src_img( 1, 1 ) = 255;

  proc.set_source_image( src_img );
  TEST( "Step 1", proc.step(), true );
  vil_image_view<bool> output = proc.thresholded_image();
  TEST( "Output size okay", true,
        output.ni() == 2 && output.nj() == 2 && output.nplanes() == 1 );

  vcl_cout << "Output=\n"
           << int(output(0,0)) << " " << int(output(0,1)) << "\n"
           << int(output(1,0)) << " " << int(output(1,1)) << "\n";

  TEST( "Output data okay", true,
        output(0,0) == false && output(0,1) == true &&
        output(1,0) == false && output(1,1) == true );



  src_img( 0, 0 ) = 0;
  src_img( 0, 1 ) = 64;
  src_img( 1, 0 ) = 160;
  src_img( 1, 1 ) = 255;

  proc.set_source_image( src_img );
  TEST( "Step 2", proc.step(), true );
  output = proc.thresholded_image();

  TEST( "Output size okay", true,
        output.ni() == 2 && output.nj() == 2 && output.nplanes() == 1 );

  vcl_cout << "Output=\n"
           << int(output(0,0)) << " " << int(output(0,1)) << "\n"
           << int(output(1,0)) << " " << int(output(1,1)) << "\n";

  TEST( "Output data okay", true,
        output(0,0) == false && output(0,1) == false &&
        output(1,0) == true  && output(1,1) == true );


  // Make sure this data is not used any more.
  src_img.fill( 0 );

  TEST( "Step 3, with no source", proc.step(), true );
  output = proc.thresholded_image();

  TEST( "Output size okay", true,
        output.ni() == 2 && output.nj() == 2 && output.nplanes() == 1 );

  vcl_cout << "Output=\n"
           << int(output(0,0)) << " " << int(output(0,1)) << "\n"
           << int(output(1,0)) << " " << int(output(1,1)) << "\n";

  TEST( "Output data okay", true,
        output(0,0) == false && output(0,1) == false &&
        output(1,0) == true  && output(1,1) == true );
}


void
test_no_persist_output()
{
  vcl_cout << "\n\nTest threshold without persist\n\n\n";

  threshold_image_process<vxl_byte> proc( "proc" );

  config_block blk = proc.params();
  blk.set( "persist_output", "false" );
  blk.set( "threshold", 127 );

  TEST( "Configure", proc.set_params( blk ), true );
  TEST( "Initialize", proc.initialize(), true );

  vil_image_view<vxl_byte> src_img( 2, 2);
  src_img( 0, 0 ) = 0;
  src_img( 0, 1 ) = 160;
  src_img( 1, 0 ) = 64;
  src_img( 1, 1 ) = 255;

  proc.set_source_image( src_img );
  TEST( "Step 1", proc.step(), true );
  vil_image_view<bool> output = proc.thresholded_image();
  TEST( "Output size okay", true,
        output.ni() == 2 && output.nj() == 2 && output.nplanes() == 1 );

  vcl_cout << "Output=\n"
           << int(output(0,0)) << " " << int(output(0,1)) << "\n"
           << int(output(1,0)) << " " << int(output(1,1)) << "\n";

  TEST( "Output data okay", true,
        output(0,0) == false && output(0,1) == true &&
        output(1,0) == false && output(1,1) == true );



  src_img( 0, 0 ) = 0;
  src_img( 0, 1 ) = 64;
  src_img( 1, 0 ) = 160;
  src_img( 1, 1 ) = 255;

  proc.set_source_image( src_img );
  TEST( "Step 2", proc.step(), true );
  output = proc.thresholded_image();

  TEST( "Output size okay", true,
        output.ni() == 2 && output.nj() == 2 && output.nplanes() == 1 );

  vcl_cout << "Output=\n"
           << int(output(0,0)) << " " << int(output(0,1)) << "\n"
           << int(output(1,0)) << " " << int(output(1,1)) << "\n";

  TEST( "Output data okay", true,
        output(0,0) == false && output(0,1) == false &&
        output(1,0) == true  && output(1,1) == true );


  // Make sure this data is not used any more.
  src_img.fill( 0 );

  TEST( "Step 3, with no source", proc.step(), false );
}

void
test_percentiles_output()
{
  vcl_cout << "\n\nTest thresholding with percentiles\n\n\n";

  threshold_image_process<vxl_byte> proc( "proc" );

  config_block blk = proc.params();
  blk.set( "type", "percentiles" );
  blk.set( "percentiles", "0.30,0.90" );

  TEST( "Configure", proc.set_params( blk ), true );
  TEST( "Initialize", proc.initialize(), true );

  vil_image_view<vxl_byte> src_img( 2, 2);
  src_img( 0, 0 ) = 0;
  src_img( 0, 1 ) = 160;
  src_img( 1, 0 ) = 64;
  src_img( 1, 1 ) = 255;

  proc.set_source_image( src_img );
  TEST( "Step 1", proc.step(), true );
  vil_image_view<bool> output = proc.thresholded_image();
  TEST( "Output size okay", true,
        output.ni() == 2 && output.nj() == 2 && output.nplanes() == 2 );

  vcl_cout << "Output=\n"
           << int(output(0,0,1)) << " " << int(output(0,1,1)) << "\n"
           << int(output(1,0,1)) << " " << int(output(1,1,1)) << "\n";

  TEST( "Output plane 1 data okay", true,
        output(0,0,0) == false && output(0,1,0) == true &&
        output(1,0,0) == true && output(1,1,0) == true );

  TEST( "Output plane 2 data okay", true,
        output(0,0,1) == false && output(0,1,1) == false &&
        output(1,0,1) == false && output(1,1,1) == true );
}

} // end anonymous namespace

int test_threshold_image_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "threshold_image_process" );

  test_default_config();
  test_persist_output();
  test_no_persist_output();
  test_percentiles_output();

  return testlib_test_summary();
}
