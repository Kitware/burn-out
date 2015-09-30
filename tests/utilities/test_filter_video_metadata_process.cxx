/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/filter_video_metadata_process.h>
#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>

#include <vcl_algorithm.h>
#include <vcl_string.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

bool setup_process( vcl_map< vcl_string, vcl_string > const& params,
                    filter_video_metadata_process & proc )
{
  config_block blk = proc.params();
  try
  {
    for( vcl_map< vcl_string, vcl_string >::const_iterator iter = params.begin();
         iter != params.end(); iter++ )
    {
      blk.set( iter->first, iter->second );
    }

    TEST( "set_params()", true, proc.set_params( blk ) );
  }
  catch( unchecked_return_value const& e )
  {
    vcl_cerr << e.what() << "\n";
    TEST( "Transfering parameters from map", true, false );
    return false;
  }

  TEST( "initialize()", true, proc.initialize() );

  return true;
}

//
//------------------------------------------------------------------------
//

void test_out_of_order_packet()
{
  vcl_cout << "Testing out of temporal order packets...\n";

  filter_video_metadata_process proc( "fvmp" );
  vcl_map< vcl_string, vcl_string > params;
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

void test_invalid_hfov()
{
  vcl_cout << "Testing invalid horizontal field-of-view field...\n";

  filter_video_metadata_process proc( "fvmp" );
  vcl_map< vcl_string, vcl_string > params;
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
  vcl_cout << "Testing invalid platform altitude field...\n";

  filter_video_metadata_process proc( "fvmp" );
  vcl_map< vcl_string, vcl_string > params;
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

void test_invalid_location( vcl_vector< video_metadata > mds )
{
  filter_video_metadata_process proc( "fvmp" );
  vcl_map< vcl_string, vcl_string > params;
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
                   .F( video_metadata::lat_lon_t( 45, 100 ) ) ); \
  mds.push_back( video_metadata().timeUTC( 12346 ) \
                   .F( video_metadata::lat_lon_t( 45.6001000000002, 100 ) ) ); \
  mds.push_back( video_metadata().timeUTC( 12347 ) \
                   .F( video_metadata::lat_lon_t( 45.05, 100.8000010000002 ) ) ); \
  mds.push_back( video_metadata().timeUTC( 12348 ) \
                   .F( video_metadata::lat_lon_t( 45.1, 100 ) ) )

  vcl_vector< video_metadata > mds;

  vcl_cout << "Testing invalid platform location field...\n";
  RESET_MDS( platform_location );
  test_invalid_location( mds );

  vcl_cout << "Testing invalid frame center field...\n";
  RESET_MDS( frame_center );
  test_invalid_location( mds );

  vcl_cout << "Testing invalid corner_ul field...\n";
  RESET_MDS( corner_ul );
  test_invalid_location( mds );

  vcl_cout << "Testing invalid corner_ur field...\n";
  RESET_MDS( corner_ur );
  test_invalid_location( mds );

  vcl_cout << "Testing invalid corner_lr field...\n";
  RESET_MDS( corner_lr );
  test_invalid_location( mds );

  vcl_cout << "Testing invalid corner_ll field...\n";
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

  return testlib_test_summary();
}
