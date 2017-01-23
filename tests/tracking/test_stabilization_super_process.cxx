/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>
#include <tracking/stabilization_super_process.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


// ----------------------------------------------------------------
void test_disabled_mode()
{
  // Configure stab sp
  vidtk::stabilization_super_process< vxl_byte > ssp( "test_ssp" );
  vidtk::config_block config;
  config = ssp.params();

  config.set_value( "mode", "disabled"); // also the default mode
  config.set_value( "run_async", "false" ); // run sync mode

  vidtk::timestamp input_ts(0.010 * 1e6, 1);
  vidtk::timestamp output_ts;
  vidtk::image_to_image_homography output_s2r;


  TEST( "Set params", ssp.set_params( config ), true );

  // supply timestamp, read back src_to_ref and check
  ssp.set_source_timestamp( input_ts );
  vidtk::timestamp first_ts( input_ts ); // save for later

  ssp.step2();

  output_s2r = ssp.src_to_ref_homography();
  output_ts = ssp.source_timestamp();
  // std::cout << "first homog: " << output_s2r;

  TEST( "Timestamp as expected", input_ts, output_ts );
  TEST( "Homography valid", output_s2r.is_valid(), true );
  TEST( "Homography new ref", output_s2r.is_new_reference(), true );
  TEST( "Homography dest ts", output_s2r.get_dest_reference(), first_ts );
  TEST( "Homography src ts", output_s2r.get_source_reference(), input_ts );

  // apply second frame
  input_ts.set_time(0.020 * 1e6);
  input_ts.set_frame_number( 2 );
  ssp.set_source_timestamp( input_ts );

  ssp.step2();

  output_s2r = ssp.src_to_ref_homography();
  output_ts = ssp.source_timestamp();

  // std::cout << "second homog: " << output_s2r;

  TEST( "Timestamp as expected", input_ts, output_ts );
  TEST( "Homography valid", output_s2r.is_valid(), true );
  TEST( "Homography new ref", output_s2r.is_new_reference(), false );
  TEST( "Homography dest ts", output_s2r.get_dest_reference(), first_ts );
  TEST( "Homography src ts", output_s2r.get_source_reference(), input_ts );

}


} // end namespace


// ================================================================
// Currently does not require data, but that may change.

int test_stabilization_super_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "stabilization_super_process" );

  test_disabled_mode();

  return testlib_test_summary();
}
