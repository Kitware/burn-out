/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/paired_buffer.h>
#include <utilities/paired_buffer_process.h>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <vxl_config.h>
#include <algorithm>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

bool test_frame( unsigned f,
                 paired_buffer< timestamp, vil_image_view< vxl_byte > > const& buff,
                 bool gt,
                 bool is_sorted=true )
{
  timestamp ts;
  ts.set_frame_number( f );
  std::cout<< "Find frame " << f << " in buffer...";
  bool res;
  vil_image_view< vxl_byte > const& im = buff.find_datum( ts, res, is_sorted );
  TEST( "testing type const& datum ", res , gt );
  if( res )
  {
    return im( 10, 10 ) == vxl_byte( f );
  }
  return false;
}

void test_basic()
{
  paired_buffer_process< timestamp, vil_image_view< vxl_byte > > buff_proc( "test_paired_buffer" );

  int length = 4;
  config_block blk = buff_proc.params();
  blk.set( "length", length );
  TEST( "Set length", buff_proc.set_params( blk ), true );
  TEST( "Initialize buffer", buff_proc.initialize(), true );

  timestamp ts;

  for( unsigned i = 0; i < static_cast< unsigned >( length * 2 ); ++i )
  {
    vil_image_view< vxl_byte > im( 20, 30 );
    im( 10, 10 ) = i;
    ts.set_frame_number( i );

    buff_proc.set_next_key( ts );
    buff_proc.set_next_datum( im );
    TEST( " Step successful?", buff_proc.step(), true );
  }

  paired_buffer< timestamp, vil_image_view< vxl_byte > > const& buff = buff_proc.buffer();

  for( unsigned i = 0; i < static_cast< unsigned >( length * 2); ++i )
  {
    bool gt = false;
    if( i >= unsigned( length ) )
      gt = true;
    TEST( "Right pixel value", test_frame( i, buff, gt ), gt );
  }

  TEST( "Test find_datum w/o sort 1. ", test_frame( 0, buff, false, false ), false );
  TEST( "Test find_datum w/o sort 2. ", test_frame( 7, buff, true, false ), true );

  // Test non-const accessors.
  paired_buffer< timestamp, vil_image_view< vxl_byte > > buff2 = buff_proc.buffer();

  ts.set_frame_number( 7 );
  bool res;
  vil_image_view< vxl_byte > &im = buff2.find_datum( ts, res, true );
  im(0,0) = 1; // to test non-const datum.
  TEST( "Non-const find_datum works.", res , true );

} // test_basic()

} // end anonymous namespace

int test_paired_buffer_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "paired buffer process" );

  test_basic();

  return testlib_test_summary();
}
