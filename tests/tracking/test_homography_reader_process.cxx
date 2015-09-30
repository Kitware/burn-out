/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_sstream.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_3x3.h>
#include <vnl/vnl_inverse.h>

#include <boost/bind.hpp>
#include <tracking/klt_tracker_process.h>
#include <utilities/homography_reader_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


vcl_string g_data_dir;


void
test_homog_valid( vidtk::homography_reader_process& hp,
                  double h00, double h01, double h02,
                  double h10, double h11, double h12,
                  double h20, double h21, double h22 )
{
  vnl_double_3x3 trueH;
  trueH(0,0) = h00;  trueH(0,1) = h01;  trueH(0,2) = h02;
  trueH(1,0) = h10;  trueH(1,1) = h11;  trueH(1,2) = h12;
  trueH(2,0) = h20;  trueH(2,1) = h21;  trueH(2,2) = h22;
  vnl_double_3x3 trueHinv = vnl_inverse( trueH );

  vnl_double_3x3 H = hp.image_to_world_homography().get_matrix();
  vnl_double_3x3 Hinv = hp.world_to_image_homography().get_matrix();

  TEST_NEAR( "image->world",
             (H/H(2,2) - trueH/trueH(2,2)).fro_norm(),
             0, 1e-6 );

  TEST_NEAR( "world->image",
             (Hinv/Hinv(2,2) - trueHinv/trueHinv(2,2)).fro_norm(),
             0, 1e-6 );
}


void
test_read_textfile()
{
  vcl_cout << "\n\n\nTest reading the simple text file\n\n";

  using namespace vidtk;

  homography_reader_process hp( "homog" );

  config_block blk = hp.params();
  blk.set( "textfile", g_data_dir+"homog_textfile.homog" );

  vcl_cout << "Homography reader\n";
  TEST( "Set parameters", hp.set_params( blk ), true );
  TEST( "Initialize", hp.initialize(), true );

  {
    TEST( "Step 1", hp.step(), true );
    test_homog_valid( hp,
                      1, 0, 1,
                      0, 1, 2,
                      0, 0, 0.5 );
  }

  {
    TEST( "Step 2", hp.step(), true );
    test_homog_valid( hp,
                      9, 2, 3,
                      4, 5, 6,
                      7, 8, 1 );
  }

  {
    TEST( "Step 3", hp.step(), true );
    test_homog_valid( hp,
                      8, 2, 3,
                      4, 5, 6,
                      6, 7, 1 );
  }

  {
    TEST( "No step 4", hp.step(), false );
  }
}





} // end anonymous namespace

int test_homography_reader_process( int argc, char* argv[] )
{
  testlib_test_start( "homography reader process" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    g_data_dir = argv[1];
    g_data_dir += "/";

    test_read_textfile();
  }

  return testlib_test_summary();
}
