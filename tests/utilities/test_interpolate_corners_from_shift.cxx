/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <vil/vil_image_view.h>

#include <utilities/interpolate_corners_from_shift_process.h>

namespace
{

void test_process()
{
  vil_image_view<vxl_byte> image( 20, 20, 3 );

  vidtk::timestamp ts1( 1, 1 );
  vidtk::timestamp ts2( 2, 2 );
  vidtk::timestamp ts3( 3, 3 );

  vidtk::geo_coord::geo_lat_lon gp1( 80.050, 100.050 );
  vidtk::geo_coord::geo_lat_lon gp3( 80.100, 100.150 );

  vidtk::video_metadata md1, md2, md3;
  md1.frame_center( gp1 );
  md3.frame_center( gp3 );

  std::vector< vidtk::video_metadata > mdv1, mdv2, mdv3;
  mdv1.push_back( md1 );
  mdv2.push_back( md2 );
  mdv3.push_back( md3 );

  vgl_h_matrix_2d< double > t1, t2, t3;
  t1.set_identity();
  t2.set_identity();
  t2.set_translation( 5, 5 );
  t3.set_identity();
  t3.set_translation( 10, 10 );

  vidtk::image_to_image_homography h1, h2, h3;
  h1.set_transform( t1 );
  h1.set_new_reference( true );
  h1.set_source_reference( ts1 );
  h1.set_dest_reference( ts1 );
  h2.set_transform( t2 );
  h2.set_new_reference( false );
  h2.set_source_reference( ts2 );
  h2.set_dest_reference( ts1 );
  h3.set_transform( t3 );
  h3.set_new_reference( false );
  h3.set_source_reference( ts3 );
  h3.set_dest_reference( ts1 );

  vidtk::interpolate_corners_from_shift_process<vxl_byte> proc_interp( "test" );

  vidtk::config_block blk = proc_interp.params();
  blk.set( "algorithm", "last_2_points" );

  TEST( "Process set_params", proc_interp.set_params( blk ), true );

  proc_interp.set_input_image( image );
  proc_interp.set_input_timestamp( ts1 );
  proc_interp.set_input_metadata_vector( mdv1 );
  proc_interp.set_input_homography( h1 );

  TEST( "Process step 1", proc_interp.step(), true );

  vidtk::video_metadata::vector_t out1 = proc_interp.output_metadata_vector();

  proc_interp.set_input_image( image );
  proc_interp.set_input_timestamp( ts2 );
  proc_interp.set_input_metadata_vector( mdv2 );
  proc_interp.set_input_homography( h2 );

  TEST( "Process step 2", proc_interp.step(), true );

  vidtk::video_metadata::vector_t out2 = proc_interp.output_metadata_vector();

  proc_interp.set_input_image( image );
  proc_interp.set_input_timestamp( ts3 );
  proc_interp.set_input_metadata_vector( mdv3 );
  proc_interp.set_input_homography( h3 );

  TEST( "Process step 3", proc_interp.step(), true );

  vidtk::video_metadata::vector_t out3 = proc_interp.output_metadata_vector();

  TEST( "Output 1 size", out1.size(), 1 );
  TEST( "Output 2 size", out2.size(), 1 );
  TEST( "Output 3 size", out3.size(), 1 );

  TEST( "Output 1 has corners", out1[0].has_valid_corners(), false );
  TEST( "Output 2 has corners", out2[0].has_valid_corners(), false );
  TEST( "Output 3 has corners", out3[0].has_valid_corners(), true );

  double ul_lat = out3[0].corner_ul().get_latitude();
  double ul_lon = out3[0].corner_ul().get_longitude();

  double lr_lat = out3[0].corner_lr().get_latitude();
  double lr_lon = out3[0].corner_lr().get_longitude();

  TEST( "Output 3 value 1", 80.14 <= ul_lat && ul_lat <= 80.16 , true );
  TEST( "Output 3 value 2", 100.24 <= ul_lon && ul_lon <= 100.26 , true );

  TEST( "Output 3 value 3", 80.04 <= lr_lat && lr_lat <= 80.06 , true );
  TEST( "Output 3 value 4", 100.04 <= lr_lon && lr_lon <= 100.06 , true );
}

}

int test_interpolate_corners_from_shift( int /*argc*/, char* /*argv*/[] )
{
  test_process();
  return testlib_test_summary();
}
