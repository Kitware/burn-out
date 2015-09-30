/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_sstream.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <vil/algo/vil_threshold.h>
#include <vil/vil_load.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>
#include <vil/vil_save.h>

#include <boost/bind.hpp>

#include <kwklt/klt_pyramid_process.h>
#include <kwklt/klt_tracking_process.h>

#include <tracking/homography_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


vcl_string g_data_dir;


inline double sqr( double x ) { return x*x; }


void
step( vidtk::klt_pyramid_process& pp,
      vidtk::klt_tracking_process& tp,
      vidtk::homography_process& hp,
      unsigned frame_number,
      vcl_string const& filename )
{
  vidtk::timestamp ts;
  ts.set_frame_number( frame_number );
  vil_image_view<vxl_byte> img = vil_load( (g_data_dir+filename).c_str() );
  vcl_cout << "Processing " << filename << "\n";
  if( !img )
  {
    TEST( "File load", false, true );
  }
  else
  {
    pp.set_image( img );
    TEST( "Step KLT pyramid process", pp.step(), true );

    tp.set_timestamp( ts );
    tp.set_image_pyramid( pp.image_pyramid() );
    tp.set_image_pyramid_gradx( pp.image_pyramid_gradx() );
    tp.set_image_pyramid_grady( pp.image_pyramid_grady() );
    TEST( "Step KLT tracking process", tp.step(), true );

    hp.set_new_tracks( tp.created_tracks() );
    hp.set_updated_tracks( tp.active_tracks() );
    TEST( "Step homography process", hp.step(), true );
  }
}


void
test_translation( vidtk::homography_process const& hp,
                  vcl_vector< vnl_double_2 > const& pts,
                  double dx, double dy,
                  bool expected_equal = true )
{
  bool good = true;
  vcl_cout << "Testing that homography is roughly a translation of ("
           << dx << "," << dy << ")\n";
  for( unsigned i = 0; i < pts.size(); ++i )
  {
    vnl_double_3 img( pts[i][0]+dx, pts[i][1]+dy, 0 );
    vnl_double_3 wld = hp.project_to_world( img );
    vnl_double_3 exp_wld( pts[i][0], pts[i][1], 0 );
    double err = (wld-exp_wld).magnitude();
    vcl_cout << img << " -> " << wld << " (expect " << exp_wld << "). Error = "
             << err << "\n";
    if( err > 0.5 )
    {
      good = false;
    }
  }
  TEST( "Projection error is small enough", good, expected_equal );
}





void
test_expected_translation()
{
  vcl_cout << "\n\n\nTest estimation under image translation\n\n";

  using namespace vidtk;

  klt_pyramid_process pp( "klt_pyramid" );
  klt_tracking_process tp( "klt" );
  homography_process hp( "homog" );

  config_block blk = tp.params();
  blk.set( "feature_count", "20" );
  // Comes out to zero, but klt_tracking has guards against it *being* zero
  blk.set( "min_feature_count_percent", "0.01" );

  vcl_cout << "KLT\n";
  TEST( "Set parameters", tp.set_params( blk ), true );
  TEST( "Initialize", tp.initialize(), true );

  vcl_cout << "Homography\n";
  //TEST( "Set parameters", hp.set_params( blk ), true );
  TEST( "Initialize", hp.initialize(), true );

  // For verification
  vcl_vector< vnl_double_2 > world_pts;
  world_pts.push_back( vnl_double_2( 40, 40 ) );
  world_pts.push_back( vnl_double_2( 100, 40 ) );
  world_pts.push_back( vnl_double_2( 100, 100 ) );
  world_pts.push_back( vnl_double_2( 40, 100 ) );

  {
    step( pp, tp, hp, 0, "foursquare.png" );
    TEST( "Features found", tp.active_tracks().size(), 16 );
    TEST( "Features created", tp.created_tracks().size(), 16 );
    test_translation( hp, world_pts, 0, 0 );
  }

  {
    step( pp, tp, hp, 1, "foursquare.png" );
    TEST( "Features tracked", tp.active_tracks().size(), 16 );
    TEST( "0 features created", tp.created_tracks().size(), 0 );
    TEST( "0 features terminated", tp.terminated_tracks().size(), 0 );
    test_translation( hp, world_pts, 0, 0 );
  }

  {
    step( pp, tp, hp, 2, "foursquare_trans+7+4.png" );
    TEST( "Features tracked", tp.active_tracks().size(), 16 );
    TEST( "0 features created", tp.created_tracks().size(), 0 );
    TEST( "0 features terminated", tp.terminated_tracks().size(), 0 );
    test_translation( hp, world_pts, 7, 4 );
  }

  {
    step( pp, tp, hp, 3, "foursquare.png" );
    TEST( "Features tracked", tp.active_tracks().size(), 16 );
    TEST( "0 features created", tp.created_tracks().size(), 0 );
    TEST( "0 features terminated", tp.terminated_tracks().size(), 0 );
    test_translation( hp, world_pts, 0, 0 );
  }

  {
    step( pp, tp, hp, 4, "foursquare_trans+13-1.png" );
    TEST( "Features tracked", tp.active_tracks().size(), 16 );
    test_translation( hp, world_pts, 13, -1 );
  }
}


void
test_expected_translation_with_mask( bool use_mask )
{
  vcl_cout << "\n\n\nTest estimation with a provided mask\n\n";

  using namespace vidtk;

  klt_pyramid_process pp( "klt_pyramid" );
  klt_tracking_process tp( "klt" );
  homography_process hp( "homog" );

  vil_image_view<vxl_byte> byte_mask = vil_load( (g_data_dir+"burn_in_mask.pgm").c_str() );
  TEST( "Loaded mask", ! byte_mask, false );

  vil_image_view<bool> mask;
  vil_threshold_above( byte_mask, mask, static_cast<vxl_byte>(127) );

  config_block blk = tp.params();
  blk.set( "feature_count", "100" );
  blk.set( "min_feature_count_percent", "0" );

  vcl_cout << "KLT\n";
  TEST( "Set parameters", tp.set_params( blk ), true );
  TEST( "Initialize", tp.initialize(), true );

  vcl_cout << "Homography\n";
  //TEST( "Set parameters", hp.set_params( blk ), true );
  TEST( "Initialize", hp.initialize(), true );

  // For verification
  vcl_vector< vnl_double_2 > world_pts;
  world_pts.push_back( vnl_double_2( 40, 40 ) );
  world_pts.push_back( vnl_double_2( 100, 40 ) );
  world_pts.push_back( vnl_double_2( 100, 100 ) );
  world_pts.push_back( vnl_double_2( 40, 100 ) );

  if( use_mask )
  {
    vcl_cout << "Using mask\n";
    hp.set_mask_image( mask );
  }
  else
  {
    vcl_cout << "Not using mask\n";
  }

  {
    step( pp, tp, hp, 0, "homog_test_mask_0.pgm" );
    vcl_cout << tp.created_tracks().size() << " features created\n";
    test_translation( hp, world_pts, 0, 0 );
  }

  {
    step( pp, tp, hp, 1, "homog_test_mask_1.pgm" );
    vcl_cout << tp.active_tracks().size() << " features tracked\n";
    vcl_cout << tp.created_tracks().size() << " features created\n";
    vcl_cout << tp.terminated_tracks().size() << " features terminated\n";
    test_translation( hp, world_pts, 0, 0, /*expected equal=*/ use_mask );
  }
}



} // end anonymous namespace

int test_homography_process( int argc, char* argv[] )
{
  testlib_test_start( "homography process" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    g_data_dir = argv[1];
    g_data_dir += "/";

    test_expected_translation();
    test_expected_translation_with_mask( false );
    test_expected_translation_with_mask( true );
  }

  return testlib_test_summary();
}
