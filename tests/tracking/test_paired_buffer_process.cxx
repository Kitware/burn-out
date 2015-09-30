/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/paired_buffer_process.h>
#include <tracking/image_object.h>
#include <utilities/timestmap.h>
#include <vcl_algorithm.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

void test_basic()
{

  paired_buffer_process< timestamp, vcl_vector< image_object > > buff_proc( "test_paired_buffer" );

  int length = 4;
  config_block blk = buff_proc.params();
  blk.set( "length", length );
  TEST( "Set length", buff_proc.set_params( blk ), true );

  //vcl_vector< image_object > frame_objs;
  //vcl_vector< typename buff_proc::key_datum_pair > uf_objs;
  //timestamp ts;
  //typename buff_proc::paired_buffer_t f_objs;

  //for( unsigned i = 0; i < static_cast< unsigned >( length * 2 ); ++i )
  //{
  //  image_object_sptr obj = new image_object();
  //  obj->area_ = i + 10;
  //  vcl_vector< image_object > objs;
  //  objs.push_back( obj );

  //  obj = new image_object();
  //  obj->area_ = i + 20;
  //  objs.push_back( obj );

  //  ts.set_frame_number( i );

  //  typename buff_proc::key_datum_pair next_pair( ts, objs );
  //  buff_proc.set_next_pair( next_pair );

  //  buff_proc.set_updated_frame_objects( uf_objs );

  //  frame_objs[ ts ] = objs;

  //  buff_proc.step();

  //  f_objs = buff_proc.frame_objects();
  //  uf_objs = f_objs;

  //  TEST( "Out frame objects size is correct.",
  //    f_objs.size() == vcl_min( length, int(i+1) ), true );

  //  //TEST( "Sorted timestamp. ", /*uf_objs->*/first == ts, true );

  //  bool is_first_frame_same = f_objs.begin()->first.frame_number()
  //    == vcl_max( int(0), int(i) - length + 1 );
  //  TEST( "Buffer max length test. ", is_first_frame_same, true );

  //  bool end_obj_is_20p = ( uf_objs[ ts ].size() == 2 )
  //                     && ( uf_objs[ ts ][ 1 ]->area_ == i + 20);
  //  TEST( "Check current frame obj.", end_obj_is_20p, true );

  //  if( i > 0 )
  //  {
  //    timestamp last_ts;
  //    last_ts.set_frame_number( i-1 );

  //    bool end_obj_is_30p = ( uf_objs[ last_ts ].size() == 3 )
  //                       && ( uf_objs[ last_ts ][ 2 ]->area_ == i + 29 );
  //    TEST( "Check last frame modified obj.", end_obj_is_30p, true );
  //  }

  //  // TODO: Fix the last frame overflow bug!
  //  image_object_sptr new_obj = new image_object;
  //  new_obj->area_ = i + 30;
  //  uf_objs[ ts ].push_back( new_obj );
  //}

} // test_basic()

} // end anonymous namespace

int test_paired_buffer_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "paired buffer process" );

  test_basic();

  return testlib_test_summary();
}
