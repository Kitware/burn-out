/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_io/mask_reader_process.h>

#include <utilities/config_block.h>

#include <iostream>
#include <algorithm>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>

#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void
test_read( std::string const& dir )
{
  mask_reader_process reader( "reader" );

  config_block blk = reader.params();
  blk.set( "file", dir + "/ssa_mask.png" );

  TEST( "Set params", reader.set_params( blk ), true );
  TEST( "Initialize", reader.initialize(), true );

  TEST( "Step 0", reader.step(), true );
}

void
test_read_error( std::string const& dir )
{
  mask_reader_process reader( "reader" );

  config_block blk = reader.params();
  blk.set( "file", dir + "/not_there.png" );

  TEST( "Set params", reader.set_params( blk ), true);
  TEST( "Initialize", reader.initialize(), false );

  TEST( "Step 1", reader.step(), false );
}

} // end namespace


int test_mask_reader_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "mask_reader_process" );

  test_read( argv[1] );
  test_read_error( argv[1] );

  return testlib_test_summary();

}
