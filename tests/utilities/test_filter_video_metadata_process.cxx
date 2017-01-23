/*ckwg +5
 * Copyright 2011-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/filter_video_metadata_process.h>
#include <utilities/config_block.h>

#include <algorithm>
#include <string>
#include <iostream>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

bool setup_process( std::map< std::string, std::string > const& params,
                    filter_video_metadata_process & proc )
{
  config_block blk = proc.params();

  checked_bool is_set = true;
  for( std::map< std::string, std::string >::const_iterator iter = params.begin();
       iter != params.end() && is_set; ++iter )
  {
    is_set = blk.set( iter->first, iter->second );
  }

  if (is_set)
  {
    TEST( "set_params()", true, proc.set_params( blk ) );
    return true;
  }
  else
  {
    TEST( "Transfering parameters from map", true, false );
    return false;
  }
}

//
//------------------------------------------------------------------------
//

void test_out_of_order_packet()
{
  std::cout << "Testing out of temporal order packets...\n";

  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    video_metadata md;

    // Step1
    md.timeUTC( 12345 );
    proc.set_source_metadata( md );
    TEST( "step() 1", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );

    // Step2
    md.timeUTC( 12346 );
    proc.set_source_metadata( md );
    TEST( "step() 2", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );

    // Step3
    md.timeUTC( 12345 );
    proc.set_source_metadata( md );
    TEST( "step() 3", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", false, proc.output_metadata().is_valid() );
  } // if( setup_successful )
}
//
//------------------------------------------------------------------------
//
void test_bad_config_parameter()
{
  filter_video_metadata_process proc( "fvmp" );
  config_block blk = proc.params();
  blk.set( "disabled", "BOB" );
  TEST( "Test bad set params", false, proc.set_params( blk ) );
}

//
//------------------------------------------------------------------------
//
void test_bad_input_parameter()
{
  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    TEST( "NO Input", true, proc.step() == process::FAILURE );
    video_metadata md;
    proc.set_source_metadata( md );
    std::vector< video_metadata > meta;
    proc.set_source_metadata_vector( meta );
    TEST( "Both Input", true, proc.step() == process::FAILURE );
  }
}

//
//------------------------------------------------------------------------
//

void test_disable_single_value()
{
  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  params["disabled"] = "true";
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    video_metadata md;

    // Step1
    md.timeUTC( 12345 );
    proc.set_source_metadata( md );
    TEST( "step() 1", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );
    TEST( "Output is correct.", md, proc.output_metadata() );

    // Step2
    md.timeUTC( 12346 );
    proc.set_source_metadata( md );
    TEST( "step() 2", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );
    TEST( "Output is correct.", md, proc.output_metadata() );

    // Step3
    md.timeUTC( 12345 );
    proc.set_source_metadata( md );
    TEST( "step() 3", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );
    TEST( "Output is correct.", md, proc.output_metadata() );
  } // if( setup_successful )

}
//
//------------------------------------------------------------------------
//

void test_disable_single_vector()
{
  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  params["disabled"] = "true";
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    std::vector< video_metadata > meta(3);
    video_metadata md[3];

    // Step1
    md[0].timeUTC( 12345 );
    meta[0] = md[0];
    md[1].timeUTC( 12346 );
    meta[1] = md[1];
    md[2].timeUTC( 12345 );
    meta[2] = md[2];
    proc.set_source_metadata_vector( meta );
    TEST( "step()", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct size.", 3, proc.output_metadata_vector().size());
    TEST( "Output is valid 1.", true, proc.output_metadata_vector()[0].is_valid() );
    TEST( "Output is valid 2.", true, proc.output_metadata_vector()[1].is_valid() );
    TEST( "Output is valid 3.", true, proc.output_metadata_vector()[2].is_valid() );
    TEST( "Output is correct 1.", md[0], proc.output_metadata_vector()[0] );
    TEST( "Output is correct 2.", md[1], proc.output_metadata_vector()[1] );
    TEST( "Output is correct 3.", md[2], proc.output_metadata_vector()[2] );
  }
}

//
//------------------------------------------------------------------------
//
void test_time_single_vector()
{
  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    std::vector< video_metadata > meta(3);
    video_metadata md[3];

    // Step1
    md[0].timeUTC( 12345 );
    meta[0] = md[0];
    md[1].timeUTC( 12346 );
    meta[1] = md[1];
    md[2].timeUTC( 12345 );
    meta[2] = md[2];
    proc.set_source_metadata_vector( meta );
    TEST( "step()", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct size.", 2, proc.output_metadata_vector().size());
    TEST( "Output is valid 1.", true, proc.output_metadata_vector()[0].is_valid() );
    TEST( "Output is valid 2.", true, proc.output_metadata_vector()[1].is_valid() );
    TEST( "Output is correct 1.", md[0], proc.output_metadata_vector()[0] );
    TEST( "Output is correct 2.", md[1], proc.output_metadata_vector()[1] );
  }
}

//
//------------------------------------------------------------------------
//
void test_consider_equal_time()
{
  std::cout << "Testing out of temporal order packets...\n";

  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  params["allow_equal_time"] = "true";
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    video_metadata md, md2;

    // Step1
    md.timeUTC( 12345 ).frame_center(vidtk::geo_coord::geo_lat_lon(30,30));
    proc.set_source_metadata( md );
    TEST( "step() 1", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", md, proc.output_metadata() );

    // Step2
    proc.set_source_metadata( md );
    TEST( "step() 2", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", false, proc.output_metadata().is_valid() );

    // Step3
    md2.timeUTC( 12345 ).frame_center(vidtk::geo_coord::geo_lat_lon(30,30.05));
    proc.set_source_metadata( md2 );
    TEST( "step() 3", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", md2, proc.output_metadata() );

    // Step4
    proc.set_source_metadata( md2 );
    TEST( "step() 4", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", false, proc.output_metadata().is_valid() );
  } // if( setup_successful )
}



//
//------------------------------------------------------------------------
//


void test_invalid_hfov()
{
  std::cout << "Testing invalid horizontal field-of-view field...\n";

  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  params[ "max_hfov" ] = "20";
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    video_metadata md;

    md.timeUTC( 12345 )
      .sensor_horiz_fov( 0.5 );
    proc.set_source_metadata( md );
    TEST( "step() 1", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );

    md.timeUTC( 12346 )
      .sensor_horiz_fov( 30 );
    proc.set_source_metadata( md );
    TEST( "step() 2", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", false, proc.output_metadata().is_valid() );
  }
}
//
//------------------------------------------------------------------------
//

void test_invalid_platform_altitude()
{
  std::cout << "Testing invalid platform altitude field...\n";

  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  params[ "max_platform_altitude_change" ] = "0.2";
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    video_metadata md;

    md.timeUTC( 12345 )
      .platform_altitude( 1555.44 );
    proc.set_source_metadata( md );
    TEST( "step() 1", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );

    md.timeUTC( 12346 )
      .platform_altitude( 515.44 );
    proc.set_source_metadata( md );
    TEST( "step() 2", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", false, proc.output_metadata().is_valid() );

    md.timeUTC( 12347 )
      .platform_altitude( 1455.44 );
    proc.set_source_metadata( md );
    TEST( "step() 3", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );
  }
}
//
//------------------------------------------------------------------------
//

void test_invalid_location( std::vector< video_metadata > mds )
{
  filter_video_metadata_process proc( "fvmp" );
  std::map< std::string, std::string > params;
  params[ "max_lat_lon_change" ] = "0.5 0.75";
  bool setup_successful = setup_process( params, proc );
  if( setup_successful )
  {
    proc.set_source_metadata( mds[0] );
    TEST( "step() 1", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );

    proc.set_source_metadata( mds[1] );
    TEST( "step() 2", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", false, proc.output_metadata().is_valid() );

    proc.set_source_metadata( mds[2] );
    TEST( "step() 3", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", false, proc.output_metadata().is_valid() );

    proc.set_source_metadata( mds[3] );
    TEST( "step() 4", true, proc.step() == process::SUCCESS );
    TEST( "Output is correct.", true, proc.output_metadata().is_valid() );
  }
}
//
//------------------------------------------------------------------------
//

void test_invalid_locations()
{
#define RESET_MDS(F) mds.clear(); \
  mds.push_back( video_metadata().timeUTC( 12345 )  \
                 .F( geo_coord::geo_lat_lon( 45, 100 ) ) );     \
  mds.push_back( video_metadata().timeUTC( 12346 ) \
                   .F( geo_coord::geo_lat_lon( 45.6001000000002, 100 ) ) ); \
  mds.push_back( video_metadata().timeUTC( 12347 ) \
                   .F( geo_coord::geo_lat_lon( 45.05, 100.8000010000002 ) ) ); \
  mds.push_back( video_metadata().timeUTC( 12348 ) \
                   .F( geo_coord::geo_lat_lon( 45.1, 100 ) ) )

  std::vector< video_metadata > mds;

  std::cout << "Testing invalid platform location field...\n";
  RESET_MDS( platform_location );
  test_invalid_location( mds );

  std::cout << "Testing invalid frame center field...\n";
  RESET_MDS( frame_center );
  test_invalid_location( mds );

  std::cout << "Testing invalid corner_ul field...\n";
  RESET_MDS( corner_ul );
  test_invalid_location( mds );

  std::cout << "Testing invalid corner_ur field...\n";
  RESET_MDS( corner_ur );
  test_invalid_location( mds );

  std::cout << "Testing invalid corner_lr field...\n";
  RESET_MDS( corner_lr );
  test_invalid_location( mds );

  std::cout << "Testing invalid corner_ll field...\n";
  RESET_MDS( corner_ll );
  test_invalid_location( mds );

#undef RESET_MDS
}

//
//------------------------------------------------------------------------
//

} // end anonymous namespace

int test_filter_video_metadata_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "filter video metadata process" );

  test_out_of_order_packet();

  test_invalid_hfov();

  test_invalid_platform_altitude();

  test_invalid_locations();

  test_bad_config_parameter();

  test_disable_single_value();

  test_disable_single_vector();

  test_time_single_vector();

  test_bad_input_parameter();

  test_consider_equal_time();

  return testlib_test_summary();
}
