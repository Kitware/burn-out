/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <exception>
#include <fstream>
#include <testlib/testlib_test.h>
#include <vul/vul_file.h>

#include <utilities/config_block.h>
#include <utilities/string_reader_process.h>

// ----------------------------------------------------------------
/** Main test routine.
 *
 *
 */
int test_file_string_reader_process( int argc, char* argv[] )
{
  testlib_test_start( "test_file_string_reader_process" );

  if ( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    std::string data_dir( argv[1] );
    std::string fname = data_dir + "/test_strings_file.txt";
    vidtk::string_reader_process fsr( "fsrp" );
    TEST( "No config initialize", fsr.initialize(), false );
    vidtk::config_block blk = fsr.params();
    blk.set( "type", "file" );
    blk.set( "file:filename", "DOES NOT EXIST" );
    TEST( "Configure", fsr.set_params( blk ), true );
    TEST( "TEST no file", fsr.initialize(), false );
    blk.set( "file:filename", fname );
    TEST( "Configure", fsr.set_params( blk ), true );
    TEST( "File exists", fsr.initialize(), true );
    TEST( "Step", fsr.step(), true );
    TEST( "Expected string", fsr.str(), "test1" );
    TEST( "Step", fsr.step(), true );
    TEST( "Expected string", fsr.str(), "test2" );
    TEST( "Step", fsr.step(), true );
    TEST( "Expected string", fsr.str(), "test3 3" );
    TEST( "No more valid strings", fsr.step(), false );
  }

  return testlib_test_summary();
}
