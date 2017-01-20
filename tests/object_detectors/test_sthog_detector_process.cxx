/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_load.h>
#include <vil/vil_image_view.h>

#include <vgl/vgl_intersection.h>
#include <vgl/vgl_intersection.txx>

#include <iostream>
#include <string>
#include <cstdio>

#include <testlib/testlib_test.h>

#include <object_detectors/sthog_detector_process.h>

#include <tracking_data/image_object.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

int test_sthog_object_detector(const std::string& indir, const std::string& outdir)
{
  sthog_detector_process<vxl_byte> detector_process("detector");

  config_block params = detector_process.params();

  params.set( "disabled", "true");
  params.set( "write_image_mode", "junk" );
  detector_process.set_params( params );
  TEST( "Prove disabled returns when disabled", true, detector_process.set_params( params ) );

  params.set( "disabled", "false");
  params.set( "write_image_mode", "junk" );
  TEST( "Bad write_image config fails", false, detector_process.set_params( params ) );

  params.set( "write_image_mode", "none" );
  params.set( "model_path_root", indir );
  params.set( "model_filenames", "/lair_vehicle_libsvm_linear1.dat /lair_vehicle_libsvm_linear2.dat" );
  params.set( "det_threshold", "1.0");
  params.set( "group_threshold", "2");
  params.set( "det_levels", "1");
  params.set( "det_tile_size", "0 0");
  params.set( "det_tile_margin", "0");
  params.set( "output_file", outdir + "/lair_x1d8_f2500_rotate_crop_460x322.log");
  params.set( "output_image_pattern", outdir + "/lair_x1d8_f2500_rotate_crop_460x322_");
  params.set( "sthog_frames", "1");
  params.set( "hog_window_size", "64 64");
  params.set( "hog_stride_step",  "4 4");

  params.print( std::cout );

  TEST( "Set parameters", true, detector_process.set_params( params ) );
  TEST( "Initialize", true, detector_process.initialize() );

  std::string input_file = indir + "/lair_x1d8_f2500_rotate_crop_460x322.png";
  std::cout << input_file << std::endl;

  vil_image_view<vxl_byte> src;
  src = vil_load( input_file.c_str() );

  detector_process.set_source_image( src );

  TEST("Step", detector_process.step(), true );

  std::vector< image_object_sptr > st = detector_process.detect_object();

  bool good = ( st.size() == 5 );
  TEST("Detect five vehicles", good, true);

  std::vector< vgl_box_2d<unsigned> > gt;
  gt.push_back( vgl_box_2d<unsigned>( 46, 110, 237, 301) );
  gt.push_back( vgl_box_2d<unsigned>( 162, 226, 184, 248) );
  gt.push_back( vgl_box_2d<unsigned>( 193, 257, 246, 310) );
  gt.push_back( vgl_box_2d<unsigned>( 324, 388, 85, 149) );
  gt.push_back( vgl_box_2d<unsigned>( 294, 358, 258, 322) );

  std::vector< image_object_sptr >::iterator st_it;
  std::vector< vgl_box_2d<unsigned> >::iterator gt_it;

  char str[1024];
  for( st_it=st.begin(); st_it!=st.end(); ++st_it )
  {
    image_object_sptr img_obj = *st_it;
    int i=1;
    for( gt_it=gt.begin(); gt_it != gt.end(); ++gt_it, ++i )
    {
      vgl_box_2d<unsigned> overlap = vgl_intersection<unsigned>( *gt_it, img_obj->get_bbox() );
      if( overlap.area() / gt_it->area() > 0.95 )
      {
        sprintf(str, "Detect vehicle %d", i );
        TEST( str, true, true );
        break;
      }
    }
  }

  st.clear();
  detector_process.set_source_image( src );
  detector_process.step();
  st = detector_process.detect_object();
  TEST("Detect clears output after step", st.size() == 5, true);

  return 1;
}

} // end anonymous namespace

int test_sthog_detector_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "sthog_detector_process" );

  std::cout << std::endl;
  std::cout << "Testing vehicle detection on lair_x1d8_f2500_rotate_crop_460x322.png";
  std::cout << std::endl;

  std::string indir( argv[1] );
  std::string outdir( argv[2] );

  test_sthog_object_detector( indir, outdir );

  return testlib_test_summary();
}
