/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <iomanip>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vxl_config.h>
#include <vil/vil_load.h>
#include <vil/algo/vil_threshold.h>
#include <tracking_data/image_object.h>

#include <object_detectors/connected_component_process.cxx>

using namespace vidtk;

void test_basic_connected_components( std::string g_data_dir )
{
  std::cout << "\n\n\nTest connected components and image_object setting\n\n";

  connected_component_process ccp( "connected_components" );

  vil_image_view<vxl_byte> objs_byte_mask = vil_load( (g_data_dir+"/conn_comp_objects.png").c_str() );
  TEST( "Loaded mask", ! objs_byte_mask, false );

  vil_image_view<bool> objs_mask;
  vil_threshold_above( objs_byte_mask, objs_mask, static_cast<vxl_byte>(1) );

  config_block blk = ccp.params();

  blk.set( "min_size", "3" );
  blk.set( "max_size", "500" );

  TEST( "Set parameters", ccp.set_params( blk ), true );
  TEST( "Initialize", ccp.initialize(), true );

  std::cout << "Using mask\n";
  ccp.set_fg_image( objs_mask );

  ccp.step();

  std::vector< image_object_sptr > objs = ccp.objects();

  TEST( "Number of objects found", objs.size(), 7 );

  std::cout << "Testing each object found...\n";

  float areas[7] = { 280, 195, 136, 457, 305, 270, 270 };

  float widths[7] = { 15, 15, 8, 27, 19, 15, 22};
  float heights[7] = {23, 13, 17, 23, 19, 22, 15};

  for( unsigned int i = 0; i < objs.size(); ++i )
  {
    std::cout << "Object " << i << ":\n";
    TEST( "Area calculated for object", objs[i]->get_image_area(), areas[i] );
    TEST( "Bounding box width", objs[i]->get_bbox().width(), widths[i] );
    TEST( "Bounding box height", objs[i]->get_bbox().height(), heights[i] );
  }
}

void test_connectivity( std::string g_data_dir )
{
  //Confirm objects caught with 4-conn and 8-conn timing
  connected_component_process ccp_4_conn( "proc_ccp_4_conn" );
  connected_component_process ccp_8_conn( "proc_ccp_8_conn" );

  config_block blk_4_conn = ccp_4_conn.params();
  config_block blk_8_conn = ccp_8_conn.params();

  blk_8_conn.set( "connectivity", 8 );
  ccp_8_conn.set_params( blk_8_conn );

  blk_4_conn.set( "connectivity", 4 );
  ccp_4_conn.set_params( blk_4_conn );

  vil_image_view<vxl_byte> img;
  vil_image_view<bool> binary_4_friendly, binary_8_friendly;

  std::vector< image_object_sptr > objs_4_conn, objs_8_conn;

  img = vil_load( ( g_data_dir+"/connectivity_test_8_friendly.png" ).c_str() );
  vil_threshold_above ( img, binary_8_friendly, static_cast<vxl_byte>(10) );

  img = vil_load( ( g_data_dir+"/connectivity_test_4_friendly.png" ).c_str() );
  vil_threshold_above ( img, binary_4_friendly, static_cast<vxl_byte>(10) );

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

  std::cout << "Test connected component connectivity speed\n";

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }

  std::string g_data_dir = argv[1];

  test_connectivity( g_data_dir );
  test_basic_connected_components( g_data_dir );

  return testlib_test_summary();
}
