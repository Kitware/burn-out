/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/string_reader_process.h>
#include <vul/vul_file.h>

#include <string>
#include <iostream>

namespace {

void test_list( std::string const& dir )
{
  {
    vidtk::string_reader_process src("string_reader_test");
    vidtk::config_block block =  src.params();
    block.set( "type", "file" );

    TEST( "Set params, no config", src.set_params( block ), false );
  }

  {
    vidtk::string_reader_process src("string_reader_test");
    vidtk::config_block block =  src.params();
    block.set( "type", "file" );

    TEST( "Initialize, no file", src.initialize(), false );
  }

  {
    vidtk::string_reader_process src("string_reader_test");
    vidtk::config_block block =  src.params();
    block.set( "type", "file" );
    block.set( "file:filename", dir + "/junk-xx" );

    TEST( "Set params", src.set_params( block ), true );
    TEST( "Initialize, bad file name", src.initialize(), false );
  }

  vidtk::string_reader_process src("string_reader_test");
  vidtk::config_block block =  src.params();
  block.set( "type", "file" );
  block.set( "file:filename", dir + "/smallframe_file.txt" );

  TEST( "Set params", src.set_params( block ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );

  std::string file = src.str();
  TEST( "Step 0 - name", vul_file::basename( src.str()), "smallframe010_06.7.pgm" );

  TEST( "Step 1", src.step(), true );

  file = src.str();
  TEST( "Step 1 - name", vul_file::basename( src.str()), "smallframe011_07.5.pgm" );
}


void test_dir( std::string const& dir )
{
  vidtk::string_reader_process src("string_reader_test");
  vidtk::config_block block =  src.params();
  block.set( "type", "dir" );
  block.set( "dir:glob", dir + "/smallframe*.pgm" );

  TEST( "Set params", src.set_params( block ), true );
  TEST( "Initialize", src.initialize(), true );

  TEST( "Step 0", src.step(), true );

  std::string file = src.str();
  TEST( "Step 0 - name", vul_file::basename( src.str()), "smallframe010_06.7.pgm" );

  TEST( "Step 1", src.step(), true );

  file = src.str();
  TEST( "Step 1 - name", vul_file::basename( src.str()), "smallframe011_07.5.pgm" );
}

} // end namespace


int test_string_reader_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "string_reader_process" );

  test_list( argv[1] );
  test_dir( argv[1] );

  return testlib_test_summary();
}
