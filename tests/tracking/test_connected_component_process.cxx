/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_iomanip.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/vil_load.h>
#include <vil/algo/vil_threshold.h>

#include <tracking/world_connected_component_process.cxx>

using namespace vidtk;

void test_connectivity( vcl_string g_data_dir )
{
  //Confirm objects caught with 4-conn and 8-conn timing
  world_connected_component_process ccp_4_conn( "proc_ccp_4_conn" );
  world_connected_component_process ccp_8_conn( "proc_ccp_8_conn" );

  config_block blk_4_conn = ccp_4_conn.params();
  config_block blk_8_conn = ccp_8_conn.params();

  blk_8_conn.set( "connectivity", 8 );
  ccp_8_conn.set_params( blk_8_conn );

  blk_4_conn.set( "connectivity", 4 );
  ccp_4_conn.set_params( blk_4_conn );

  vil_image_view<vxl_byte> img;
  vil_image_view<bool> binary_4_friendly, binary_8_friendly;

  vcl_vector< image_object_sptr > objs_4_conn, objs_8_conn;

  img = vil_load( ( g_data_dir+"/connectivity_test_8_friendly.png" ).c_str() );
  vil_threshold_above ( img, binary_8_friendly, (vxl_byte)10 );

  img = vil_load( ( g_data_dir+"/connectivity_test_4_friendly.png" ).c_str() );
  vil_threshold_above ( img, binary_4_friendly, (vxl_byte)10 );

  //First, an 8-conn friendly image
  ccp_8_conn.set_fg_image( binary_8_friendly );
  ccp_8_conn.step();
  objs_8_conn = ccp_8_conn.objects();

  ccp_4_conn.set_fg_image( binary_8_friendly );
  ccp_4_conn.step();
  objs_4_conn = ccp_4_conn.objects();

  TEST( "8_conn object count", objs_8_conn.size(), 8 );
  TEST( "4_conn object count", objs_4_conn.size(), 16 );

  //Now, a 4-conn friendly image
  ccp_8_conn.set_fg_image( binary_4_friendly );
  ccp_8_conn.step();
  objs_8_conn = ccp_8_conn.objects();

  ccp_4_conn.set_fg_image( binary_4_friendly );
  ccp_4_conn.step();
  objs_4_conn = ccp_4_conn.objects();

  TEST( "8_conn object count", objs_8_conn.size(), 7 );
  TEST( "4_conn object count", objs_4_conn.size(), 7 );
}

int test_connected_component_process( int argc, char* argv[] )
{
  testlib_test_start( "connected_component_process" );

  vcl_cout << "Test connected component connectivity speed\n";

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }

  vcl_string g_data_dir;
  g_data_dir = argv[1];

  test_connectivity( g_data_dir );

  return testlib_test_summary();
}
