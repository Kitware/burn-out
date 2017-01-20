/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <object_detectors/diff_super_process.h>
#include <process_framework/process.h>
#include <tracking_data/shot_break_flags.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>
#include <utilities/config_block.h>
#include <vil/vil_image_view.h>
#include <testlib/testlib_test.h>

using namespace vidtk;


void setup_and_test_disabled_sp( bool disabled, bool run_async )
{
  diff_super_process< vxl_byte > dsp( "dsp" );

  config_block cfg = dsp.params();

  if (!cfg.set( "disabled", disabled ) ||
      !cfg.set( "run_async", run_async ) ||
      !cfg.set( "image_diff_spacing", 3 ) ||
      !cfg.set( "image_diff:threshold", 125 ))
  {
    TEST( "Config block has parameters I am looking for.", true, false );
  }

  TEST( "set_params()", true, dsp.set_params( cfg ) );

  TEST( "initialize()", true, dsp.initialize() );

  // Frame # 1
  vil_image_view< vxl_byte > img1( 20, 20 );
  img1.fill( 0 );
  timestamp ts( 0.0, 1 );

  image_to_image_homography i2i;  i2i.set_valid( true );
  image_to_plane_homography i2p;  i2p.set_valid( true );
  plane_to_image_homography p2i;  p2i.set_valid( true );
  image_to_utm_homography i2u;    i2u.set_valid( true );

  // Supply data to all input ports
  dsp.set_source_image( img1 );
  dsp.set_source_timestamp( ts );
  dsp.set_src_to_ref_homography( i2i );
  dsp.set_src_to_wld_homography( i2p );
  dsp.set_wld_to_src_homography( p2i );
  dsp.set_src_to_utm_homography( i2u );
  dsp.set_world_units_per_pixel( 1.0 );
  dsp.set_input_ref_to_wld_homography( i2p );
  dsp.set_input_video_modality( VIDTK_COMMON );
  dsp.set_input_shot_break_flags( shot_break_flags() );

  TEST( "first step2()", true, dsp.step2() == process::SUCCESS );

  bool output_checks = dsp.source_timestamp() == ts
    && (disabled || dsp.fg_out()(10,10) == false)
    && dsp.fg_out()(10,15) == false
    && (disabled || dsp.diff_out()(10,10) == 0.0)
    && dsp.diff_out()(1,5) == 0.0;
  TEST( "Verify output of first step", true, output_checks );

  // Frame # 2
  ts = timestamp( 0.5, 2 );

  // Supply data to all input ports
  dsp.set_source_image( img1 );
  dsp.set_source_timestamp( ts );
  dsp.set_src_to_ref_homography( i2i );
  dsp.set_src_to_wld_homography( i2p );
  dsp.set_wld_to_src_homography( p2i );
  dsp.set_src_to_utm_homography( i2u );
  dsp.set_world_units_per_pixel( 1.0 );
  dsp.set_input_ref_to_wld_homography( i2p );
  dsp.set_input_video_modality( VIDTK_COMMON );
  dsp.set_input_shot_break_flags( shot_break_flags() );

  TEST( "second step2()", true, dsp.step2() == process::SUCCESS );

  output_checks = dsp.source_timestamp() == ts
    && (disabled || dsp.fg_out()(10,10) == false)
    && dsp.fg_out()(10,15) == false
    && (disabled || dsp.diff_out()(10,10) == 0.0)
    && dsp.diff_out()(1,5) == 0.0;
  TEST( "Verify output of second step", true, output_checks );

  // Frame # 3
  img1( 10, 10 ) = 255;
  ts = timestamp( 1.0, 3 );

  // Supply data to all input ports
  dsp.set_source_image( img1 );
  dsp.set_source_timestamp( ts );
  dsp.set_src_to_ref_homography( i2i );
  dsp.set_src_to_wld_homography( i2p );
  dsp.set_wld_to_src_homography( p2i );
  dsp.set_src_to_utm_homography( i2u );
  dsp.set_world_units_per_pixel( 1.0 );
  dsp.set_input_ref_to_wld_homography( i2p );
  dsp.set_input_video_modality( VIDTK_COMMON );
  dsp.set_input_shot_break_flags( shot_break_flags() );

  TEST( "third step2()", true, dsp.step2() == process::SUCCESS );

  output_checks = dsp.source_timestamp() == ts
    && (disabled || dsp.fg_out()(10,10) == true)
    && dsp.fg_out()(10,15) == false
    && (disabled || dsp.diff_out()(10,10) > 100)
    && dsp.diff_out()(1,5) == 0.0;
  TEST( "Verify output of third step", true, output_checks );
}

//------------------------------------------------------------------------

void test_disabled()
{
  // Configuration 1: disabled, sync
  setup_and_test_disabled_sp( true, false );

  // Configuration 2: enabled, sync
  setup_and_test_disabled_sp( false, false );

  // Configuration 3: disabled, async
  setup_and_test_disabled_sp( true, true );

  // Configuration 4: enabled, async
  // This function call is not suitable for this test. Punting for now!
  // setup_and_test_disabled_sp( false, true );
}

//------------------------------------------------------------------------

int test_diff_super_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test diff super process" );

  test_disabled();

  return testlib_test_summary();
}
