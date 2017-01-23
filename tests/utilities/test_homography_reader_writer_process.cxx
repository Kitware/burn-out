/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_3x3.h>
#include <vnl/vnl_inverse.h>

#include <boost/bind.hpp>
#include <utilities/homography_reader_process.h>
#include <utilities/homography_writer_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

std::string g_data_dir;

void
test_homog_valid( vnl_double_3x3 H,
                  vnl_double_3x3 trueH )
{
  TEST_NEAR( "Test homography",
             (H/H(2,2) - trueH/trueH(2,2)).fro_norm(),
             0, 1e-6 );
}

void
test_homog_valid( homography_reader_process& hp,
                  timestamp time1, timestamp time2,
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

  test_homog_valid(H, trueH);
  test_homog_valid(Hinv, trueHinv);

  H = hp.image_to_world_vidtk_homography_plane().get_transform().get_matrix();
  Hinv = hp.world_to_image_vidtk_homography_plane().get_transform().get_matrix();
  test_homog_valid(H, trueH);
  std::cout << hp.image_to_world_vidtk_homography_plane() << std::endl;
  std::cout << trueHinv << std::endl;
  std::cout << hp.world_to_image_vidtk_homography_plane() << std::endl;
  test_homog_valid(Hinv, trueHinv);
  H = hp.image_to_world_vidtk_homography_image().get_transform().get_matrix();
  Hinv = hp.world_to_image_vidtk_homography_image().get_transform().get_matrix();
  test_homog_valid(H, trueH);
  test_homog_valid(Hinv, trueHinv);
  if(time1.is_valid())
  {
#define TEST_TIME( M, T1, T2 ) \
    TEST_EQUAL( M " Correct frame ", T1.frame_number(), T2.frame_number());\
    TEST_NEAR( M " Correct time", T1.time(), T2.time(), 1e-8)
    TEST_TIME("time test 1",hp.image_to_world_vidtk_homography_plane().get_source_reference(), time1);
    TEST_TIME("time test 2",hp.world_to_image_vidtk_homography_plane().get_dest_reference(), time1);
    TEST_TIME("time test 3",hp.image_to_world_vidtk_homography_image().get_source_reference(), time1);
    TEST_TIME("time test 4",hp.image_to_world_vidtk_homography_image().get_dest_reference(), time2);
    TEST_TIME("time test 5",hp.world_to_image_vidtk_homography_image().get_source_reference(), time2);
    TEST_TIME("time test 6",hp.world_to_image_vidtk_homography_image().get_dest_reference(), time1);
#undef TEST_TIME
  }
}


void
test_read_textfile()
{
  std::cout << "\n\n\nTest reading the simple text file\n\n";

  homography_reader_process hp( "homog" );

  config_block blk = hp.params();
  blk.set( "textfile", g_data_dir+"homog_textfile.homog" );
  blk.set( "version", "1" );

  std::cout << "Homography reader\n";
  TEST( "Set parameters", hp.set_params( blk ), true );
  TEST( "Initialize", hp.initialize(), true );

  {
    TEST( "Step 1", hp.step(), true );
    test_homog_valid( hp, timestamp(), timestamp(),
                      1, 0, 1,
                      0, 1, 2,
                      0, 0, 0.5 );
  }

  {
    TEST( "Step 2", hp.step(), true );
    test_homog_valid( hp, timestamp(), timestamp(),
                      9, 2, 3,
                      4, 5, 6,
                      7, 8, 1 );
  }

  {
    TEST( "Step 3", hp.step(), true );
    test_homog_valid( hp, timestamp(), timestamp(),
                      8, 2, 3,
                      4, 5, 6,
                      6, 7, 1 );
  }

  {
    TEST( "No step 4", hp.step(), false );
  }
}

void
test_read_bad_config()
{
  {
    homography_reader_process hp( "homog" );
    config_block blk = hp.params();
    blk.set( "textfile", g_data_dir+"homog_textfile.homog/DOES_NOT_EXIST.txt" );
    blk.set( "version", "1" );
    TEST( "Set parameters", hp.set_params( blk ), true );
    TEST( "Initialize", hp.initialize(), false );
  }
  {
    homography_reader_process hp( "homog" );
    config_block blk = hp.params();
    blk.set( "textfile", g_data_dir+"homog_textfile.homog" );
    blk.set( "version", "100" );
    TEST( "Set parameters", hp.set_params( blk ), false );
  }
  {
    homography_writer_process hp( "homog" );
    config_block blk = hp.params();
    blk.set( "output_filename", g_data_dir+"homog_textfile.homog/DOES_NOT_EXIST.txt" );
    blk.set( "disabled", "false" );
    TEST( "Set parameters", hp.set_params( blk ), true );
    TEST( "Initialize", hp.initialize(), false );
  }
  {
    homography_writer_process hp( "homog" );
    config_block blk = hp.params();
    blk.set( "output_filename", "test.homog" );
    blk.set( "disabled", "BOB" );
    TEST( "Set parameters", hp.set_params( blk ), false );
  }
}

void
test_empty_file()
{
  homography_reader_process hp( "homog" );
  config_block blk = hp.params();
  blk.set( "version", "1" );
  TEST( "Set parameters", hp.set_params( blk ), true );
  TEST( "Initialize", hp.initialize(), true );
  timestamp ts(100, 1000);
  hp.set_timestamp(ts);
  TEST( "Step", hp.step(), true );
}

void test_homog_writer()
{
  {
    homography_writer_process hp( "homog" );
    config_block blk = hp.params();
    blk.set( "output_filename", "test_homog.txt" );
    blk.set( "disabled", "false" );
    blk.set( "append", "false" );
    TEST( "Set parameters", hp.set_params( blk ), true );
    TEST( "Initialize", hp.initialize(), true );
    vnl_double_3x3 H;
    vgl_h_matrix_2d<double> H2d;
    timestamp ts;
    H(0,0) = 1;  H(0,1) = 0;  H(0,2) = 0;
    H(1,0) = 0;  H(1,1) = 1;  H(1,2) = 0;
    H(2,0) = 0;  H(2,1) = 0;  H(2,2) = 1;
    ts = timestamp(10, 15);
    H2d.set(H);
    hp.set_source_homography(H2d);
    hp.set_source_timestamp(ts);
    TEST( "Step", hp.step(), true );
    H(0,0) = 1.5;  H(0,1) = 0;    H(0,2) = 2;
    H(1,0) = 0;    H(1,1) = 3.5;  H(1,2) = 2.5;
    H(2,0) = 0;    H(2,1) = 0;    H(2,2) = 1;
    ts = timestamp(12, 20);
    H2d.set(H);
    image_to_image_homography itwh;
    itwh.set_transform(H2d);
    itwh.set_source_reference(ts);
    itwh.set_dest_reference(timestamp(10, 15));
    hp.set_source_timestamp(ts);
    hp.set_source_vidtk_homography(itwh);
    TEST( "Step", hp.step(), true );
  }
  {
    homography_writer_process hp( "homog" );
    config_block blk = hp.params();
    blk.set( "disabled", "true" );
    TEST( "Set parameters", hp.set_params( blk ), true );
    TEST( "Initialize", hp.initialize(), true );
    TEST( "Initialize", hp.step(), false );
  }
}

void test_homog_reader_2()
{
  homography_reader_process hp( "homog" );

  config_block blk = hp.params();
  blk.set( "textfile","test_homog.txt" );
  blk.set( "version", "2" );

  std::cout << "Homography reader\n";
  TEST( "Set parameters", hp.set_params( blk ), true );
  TEST( "Initialize", hp.initialize(), true );

  TEST( "Step 1", hp.step(), true );
  test_homog_valid( hp, timestamp(10, 15), timestamp(10, 15),
                    1, 0, 0,
                    0, 1, 0,
                    0, 0, 1 );
  TEST( "Step 2", hp.step(), true );
  test_homog_valid( hp, timestamp(12, 20), timestamp(10, 15),
                    1.5, 0, 2,
                    0, 3.5, 2.5,
                    0, 0, 1 );
  TEST( "No step 3", hp.step(), false );
}


} // end anonymous namespace

int test_homography_reader_writer_process( int argc, char* argv[] )
{
  testlib_test_start( "homography reader and writer processes" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    g_data_dir = argv[1];
    g_data_dir += "/";

    test_read_textfile();
    test_homog_writer();
    test_empty_file();
    test_homog_reader_2();
    test_read_bad_config();
  }

  return testlib_test_summary();
}
