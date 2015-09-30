/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>
#include <tracking/stabilization_super_process.h>
#include <tracking/diff_super_process.h>
#include <pipeline/sync_pipeline.h>
#include <utilities/config_block.h>
#include <utilities/homography.h>
#include <video/generic_frame_process.h>
#include <video/threshold_image_process.h>
#include <vnl/vnl_double_2.h>
#include <vxl_config.h>

using namespace vidtk;

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

vcl_string g_data_dir;

void
test_translation( vgl_h_matrix_2d<double> const& H,
                  vcl_vector< vnl_double_2 > const& pts,
                  double dx, double dy,
                  bool expected_equal = true )
{
  bool good = true;
  vcl_cout << "Testing that homography is roughly a translation of ("
           << dx << "," << dy << ")\n";
  for( unsigned i = 0; i < pts.size(); ++i )
  {
    vgl_homg_point_2d<double> img( pts[i][0]+dx, pts[i][1]+dy );
    vgl_point_2d<double> wld = H * img;
    vnl_double_2 error_chord( wld.x() - pts[i][0],
                              wld.y() - pts[i][1] );

    double err = error_chord.magnitude();
    vcl_cout << img << " -> " << wld << " (expect " << pts[i] << "). Error = "
             << err << "\n";
    if( err > 0.5 )
    {
      good = false;
    }
  }
  TEST( "Projection error is small enough", good, expected_equal );
}

void test_stab_sp( bool with_mask )
{
  sync_pipeline p;

  generic_frame_process< vxl_byte > src( "src" );
  p.add( &src );

  stabilization_super_process< vxl_byte > stab_sp( "stab_sp" );
  p.add( &stab_sp );

  p.connect( src.image_port(),
             stab_sp.set_source_image_port() );
  p.connect( src.timestamp_port(),
             stab_sp.set_source_timestamp_port() );

  config_block c = p.params();
  c.set( "src:type", "image_list" );
  c.set( "src:image_list:list", g_data_dir + "homog_test_mask_0.pgm\n" +
                                g_data_dir + "homog_test_mask_1.pgm\n" );
  c.set( "stab_sp:mode", "compute" );
  c.set( "stab_sp:run_async", "false" );
  c.set( "stab_sp:klt_homog_sp:klt_tracking:feature_count", "100" );
  c.set( "stab_sp:klt_homog_sp:klt_tracking:min_feature_count_percent", "0" );
  //c.set( "stab_sp:klt_homog_sp:klt_tracking:verbosity_level", "0" );
  if( with_mask )
  {
    c.set( "stab_sp:masking_enabled", "true" );
    c.set( "stab_sp:md_mask_sp:metadata_mask_reader:image_list:list",
            g_data_dir + "burn_in_mask.pgm" );
  }

  TEST( "stab_sp pipeline: set_params()", p.set_params( c ), true );
  TEST( "stab_sp pipeline: intialize()", p.initialize(), true );
  TEST( "stab_sp pipeline: execute()", p.execute(), process::SUCCESS );
  TEST( "stab_sp pipeline: execute()", p.execute(), process::SUCCESS );

  image_to_image_homography H_s2r = stab_sp.src_to_ref_homography();

  // For verification
  vcl_vector< vnl_double_2 > world_pts;
  world_pts.push_back( vnl_double_2( 40, 40 ) );
  world_pts.push_back( vnl_double_2( 100, 40 ) );
  world_pts.push_back( vnl_double_2( 100, 100 ) );
  world_pts.push_back( vnl_double_2( 40, 100 ) );

  //// With mask, the computed homography should be identity, without mask
  //// it should be non-identity.
  test_translation( H_s2r.get_transform(), world_pts, 0, 0, with_mask );
}

void test_diff_sp( bool with_mask )
{
  sync_pipeline p;

  generic_frame_process< vxl_byte > src( "src" );
  p.add( &src );

  diff_super_process< vxl_byte > diff_sp( "diff_sp" );
  p.add( &diff_sp );

  generic_frame_process< vxl_byte > mask( "mask" );
  p.add( &mask );

  threshold_image_process< vxl_byte > thresh ( "thresh" );
  p.add( &thresh );

  config_block c = p.params();

  p.connect( src.image_port(),
             diff_sp.set_source_image_port() );
  p.connect( src.timestamp_port(),
             diff_sp.set_source_timestamp_port() );

  if( with_mask )
  {
    p.connect( mask.image_port(),
               thresh.set_source_image_port() );
    p.connect( thresh.thresholded_image_port(),
               diff_sp.set_mask_port() );
    c.set( "diff_sp:masking_enabled", "true" );
    c.set( "diff_sp:run_async", "false" );
    c.set( "mask:image_list:list", g_data_dir + "burn_in_mask.pgm" );
    c.set( "mask:read_only_first_frame", "true" );
  }

  c.set( "src:type", "image_list" );
  c.set( "src:image_list:list", g_data_dir + "diff_test_black.pgm\n" +
                                g_data_dir + "homog_test_mask_0.pgm\n" +
                                g_data_dir + "homog_test_mask_1.pgm\n" );
  c.set( "mask:type", "image_list" );
  c.set( "thresh:threshold", "127" );

  // Just to test that cropping of src image and mask is consistent. An error
  // will be thrown if the two images are inconsistent in size.
  /*c.set( "diff_sp:source_crop:upper_left", "5 10" );
  c.set( "diff_sp:source_crop:lower_right", "-5 -10" );
  c.set( "diff_sp:trans_for_cropping:postmult_matrix", "1 0 5   0 1 10  0 0 1" ); */

  // Test pipeline initialization
  TEST( "diff_sp pipeline: set_params()", p.set_params( c ), true );
  TEST( "diff_sp pipeline: intialize()", p.initialize(), true );

  // Create Valid Homography
  image_to_image_homography H;
  H.set_identity(true);

  // Execute Step 1
  diff_sp.set_src_to_ref_homography( H );
  TEST( "diff_sp pipeline: execute1()", p.execute(), process::SUCCESS );

  // Execute Step 2
  diff_sp.set_src_to_ref_homography( H );
  TEST( "diff_sp pipeline: execute2()", p.execute(), process::SUCCESS );

  // Execute Step 3
  diff_sp.set_src_to_ref_homography( H );
  TEST( "diff_sp pipeline: execute3()", p.execute(), process::SUCCESS );

  // Retrieve output image from diff_sp
  vil_image_view<bool> fg = diff_sp.fg_out();

  // Check to see if there was at least one mover (foreground) pixel.
  bool has_movers = false;
  for( unsigned j = 0; j < fg.nj() && !has_movers ; j++ )
    for( unsigned i = 0; i < fg.ni() && !has_movers ; i++ )
      if( fg(i,j) )
        has_movers = true;

  TEST( "Expected movers detected.", has_movers, !with_mask );
}

} // end anonymous namespace

int test_metadata_mask_pipeline( int argc, char* argv[] )
{
  testlib_test_start( "metadata mask pipeline" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    g_data_dir = argv[1];
    g_data_dir += "/";

    vcl_cout<< "\n Testing stabilization with out metdata mask.\n\n";
    test_stab_sp( false );
    vcl_cout<< "\n Testing stabilization with metdata mask.\n\n";
    test_stab_sp( true );
    vcl_cout<< "\n Testing image differencing with out metdata mask.\n\n";
    test_diff_sp( false );
    vcl_cout<< "\n Testing image differencing with metdata mask.\n\n";
    test_diff_sp( true );
  }

  return testlib_test_summary();
}
