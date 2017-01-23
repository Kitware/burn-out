/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <algorithm>
#include <iomanip>

#include <vil/vil_image_view.h>
#include <vil/vil_save.h>
#include <vil/vil_load.h>
#include <vil/vil_crop.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <testlib/testlib_test.h>

#include <vil_plugins/vil_plugins_config.h>
#include <vil_plugins/vil_plugin_loader.h>

#include <video_io/filename_frame_metadata_process.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

struct corner_points
{
public:
  double ul[2], ur[2], lr[2], ll[2];
};

bool equ(vil_image_view<vxl_byte> const& i1, vil_image_view<vxl_byte> const& i2)
{
  if(!i1 || !i2 || i1.ni() != i2.ni() || i1.nj() != i2.nj() || i1.nplanes() != i2.nplanes())
  {
    return false;
  }

  vil_image_view<vxl_byte>::const_iterator iter1 = i1.begin();
  vil_image_view<vxl_byte>::const_iterator iter2 = i2.begin();
  for(; iter1 != i1.end() && iter2 != i2.end(); ++iter1, ++iter2)
  {
    if(*iter1 != *iter2)
    {
      return false;
    }
  }
  return iter1 == i1.end() && iter2 == i2.end();
}


void test_passed_roi( std::string const&          dir,
                      corner_points               cp,
                      vil_image_view< vxl_byte >  ref_img,
                      std::string                 cache )
{
  std::cout << "\n\nTesting passed roi cache: " << cache << "\n\n";

  // process should find the metadata files with a path relative to the image
  // directory.

  filename_frame_metadata_process< vxl_byte > src( "src" );

  config_block blk = src.params();
  blk.set( "metadata_mode", "NITF" );
  blk.set( "framenum_regex", "_([0-9]+).ntf" );
  blk.set( "crop_mode", "pixel_input" );
  blk.set( "cache_mode", cache );
  blk.set( "use_latlon_for_xfm", "true" );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );
  src.set_filename( dir + "/test_img_1.ntf" );
  src.set_pixel_roi( "3199x2859+11174+14725" );
  TEST( "Step", src.step(), true );
  timestamp ts = src.timestamp();
  vil_image_view< vxl_byte > img = src.image();
  video_metadata vm = src.metadata();
  //Test timestamp
  TEST( "Timestamp is valid:", ts.is_valid(), true );
  TEST( "Frame:", ts.frame_number(), 1 );
  TEST_NEAR( "Time:", ts.time(), 1256157126875000, 1e-7 );
  std::cout << ts << std::endl;
  //Test Image
  TEST( "Image values are same", equ( img, ref_img ), true );
  //Test metadata
  TEST( "Video metadata is valid:", vm.is_valid(), true );
  std::cout << std::setprecision( 20 )
            << vm.corner_ul().get_latitude() << " " << vm.corner_ul().get_longitude() << " "
            << vm.corner_ur().get_latitude() << " " << vm.corner_ur().get_longitude() << " "
            << vm.corner_lr().get_latitude() << " " << vm.corner_lr().get_longitude() << " "
            << vm.corner_ll().get_latitude() << " " << vm.corner_ll().get_longitude() << std::endl;
  TEST_NEAR( "VM UL (lat):", vm.corner_ul().get_latitude(), cp.ul[0], 2e-5 );
  TEST_NEAR( "VM UL (lon):", vm.corner_ul().get_longitude(), cp.ul[1], 2e-5 );

  TEST_NEAR( "VM UR (lat):", vm.corner_ur().get_latitude(), cp.ur[0], 2e-5 );
  TEST_NEAR( "VM UR (lon):", vm.corner_ur().get_longitude(), cp.ur[1], 2e-5 );

  TEST_NEAR( "VM LR (lat):", vm.corner_lr().get_latitude(), cp.lr[0], 2e-5 );
  TEST_NEAR( "VM LR (lon):", vm.corner_lr().get_longitude(), cp.lr[1], 2e-5 );

  TEST_NEAR( "VM LL (lat):", vm.corner_ll().get_latitude(), cp.ll[0], 2e-5 );
  TEST_NEAR( "VM LL (lon):", vm.corner_ll().get_longitude(), cp.ll[1], 2e-5 );
} // test_passed_roi


void test_geo_roi( std::string const&         dir,
                   corner_points              cp,
                   vil_image_view< vxl_byte > ref_img,
                   std::string                roi )
{
  std::cout << "\n\nTesting geo roi ROI: " << roi << "\n\n";

  // process should find the metadata files with a path relative to the image
  // directory.

  filename_frame_metadata_process< vxl_byte > src( "src" );

  config_block blk = src.params();
  blk.set( "metadata_mode", "NITF" );
  blk.set( "framenum_regex", "_([0-9]+).ntf" );
  blk.set( "crop_mode", "geographic_config" );
  blk.set( "cache_mode", "NONE" );
  blk.set( "use_latlon_for_xfm", "true" ); //pixels are "squared" in lat lon
  blk.set( "geo_roi", roi );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );
  src.set_filename( dir + "/test_img_1.ntf" );
  TEST( "Step", src.step(), true );
  timestamp ts = src.timestamp();
  vil_image_view< vxl_byte > img = src.image();
  video_metadata vm = src.metadata();
  //Test timestamp
  TEST( "Timestamp is valid:", ts.is_valid(), true );
  TEST( "Frame:", ts.frame_number(), 1 );
  TEST_NEAR( "Time:", ts.time(), 1256157126875000, 1e-7 );
  std::cout << ts << std::endl;
  //Test Image
  TEST( "Image values are same", equ( img, ref_img ), true );
  //Test metadata
  TEST( "Video metadata is valid:", vm.is_valid(), true );
  TEST_NEAR( "VM UL (lat):", vm.corner_ul().get_latitude(), cp.ul[0], 2e-5 );
  TEST_NEAR( "VM UL (lon):", vm.corner_ul().get_longitude(), cp.ul[1], 2e-5 );

  TEST_NEAR( "VM UR (lat):", vm.corner_ur().get_latitude(), cp.ur[0], 2e-5 );
  TEST_NEAR( "VM UR (lon):", vm.corner_ur().get_longitude(), cp.ur[1], 2e-5 );

  TEST_NEAR( "VM LR (lat):", vm.corner_lr().get_latitude(), cp.lr[0], 2e-5 );
  TEST_NEAR( "VM LR (lon):", vm.corner_lr().get_longitude(), cp.lr[1], 2e-5 );

  TEST_NEAR( "VM LL (lat):", vm.corner_ll().get_latitude(), cp.ll[0], 2e-5 );
  TEST_NEAR( "VM LL (lon):", vm.corner_ll().get_longitude(), cp.ll[1], 2e-5 );
} // test_geo_roi


void test_geo_roi_utm_xform( std::string const&         dir,
                             corner_points              cp,
                             vil_image_view< vxl_byte > ref_img,
                             std::string                roi )
{
  std::cout << "\n\nTesting geo roi utm xform: " << roi << "\n\n";

  // process should find the metadata files with a path relative to the image
  // directory.

  filename_frame_metadata_process< vxl_byte > src( "src" );

  config_block blk = src.params();
  blk.set( "metadata_mode", "NITF" );
  blk.set( "framenum_regex", "_([0-9]+).ntf" );
  blk.set( "crop_mode", "geographic_config" );
  blk.set( "cache_mode", "NONE" );
  blk.set( "use_latlon_for_xfm", "false" );
  blk.set( "geo_roi", roi );

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );
  src.set_filename( dir + "/test_img_1.ntf" );
  TEST( "Step", src.step(), true );
  timestamp ts = src.timestamp();
  vil_image_view< vxl_byte > img = src.image();
  video_metadata vm = src.metadata();
  //Test timestamp
  TEST( "Timestamp is valid:", ts.is_valid(), true );
  TEST( "Frame:", ts.frame_number(), 1 );
  TEST_NEAR( "Time:", ts.time(), 1256157126875000, 1e-7 );
  std::cout << ts << std::endl;
  //Test Image
  TEST( "Image values are same", equ( img, ref_img ), true ); //The source image is squared in lat lon, so UTM based xfrom will likely introduce a off by 1 pixel
  //Test metadata
  TEST( "Video metadata is valid:", vm.is_valid(), true );
  TEST_NEAR( "VM UL (lat):", vm.corner_ul().get_latitude(), cp.ul[0], 2e-5 );
  TEST_NEAR( "VM UL (lon):", vm.corner_ul().get_longitude(), cp.ul[1], 2e-5 );

  TEST_NEAR( "VM UR (lat):", vm.corner_ur().get_latitude(), cp.ur[0], 2e-5 );
  TEST_NEAR( "VM UR (lon):", vm.corner_ur().get_longitude(), cp.ur[1], 2e-5 );

  TEST_NEAR( "VM LR (lat):", vm.corner_lr().get_latitude(), cp.lr[0], 2e-5 );
  TEST_NEAR( "VM LR (lon):", vm.corner_lr().get_longitude(), cp.lr[1], 2e-5 );

  TEST_NEAR( "VM LL (lat):", vm.corner_ll().get_latitude(), cp.ll[0], 2e-5 );
  TEST_NEAR( "VM LL (lon):", vm.corner_ll().get_longitude(), cp.ll[1], 2e-5 );
  vil_save( img, "result.png" );
} // test_geo_roi_utm_xform


void test_config()
{
  std::cout << "\n\nTesting when input is not set\n\n";

  filename_frame_metadata_process< vxl_byte > src( "src" );

  config_block blk = src.params();
  blk.set( "metadata_mode", "BAD" );
  TEST( "Set params", src.set_params( blk ), false );
#if defined ( HAS_LTI_NITF2 ) || defined ( HAS_VIL_GDAL )
  blk.set( "metadata_mode", "NITF" );
  TEST( "Set params", src.set_params( blk ), true );
#else
  blk.set( "metadata_mode", "NITF" );
  TEST( "Set params", src.set_params( blk ), false );
#endif
  blk.set( "metadata_mode", "NONE" );
  TEST( "Set params", src.set_params( blk ), true );
  blk.set( "crop_mode", "BAD" );
  TEST( "Set params", src.set_params( blk ), false );
  blk.set( "crop_mode", "pixel_input" );
  TEST( "Set params", src.set_params( blk ), true );
  blk.set( "crop_mode", "geographic_config" );
  TEST( "Set params", src.set_params( blk ), false );
#if defined ( HAS_LTI_NITF2 ) || defined ( HAS_VIL_GDAL )
  blk.set( "metadata_mode", "NITF" );
  TEST( "Set params", src.set_params( blk ), true );
  blk.set( "geo_roi", "39.773130421598160922 -84.120149319035462554" );
  TEST( "Set params", src.set_params( blk ), false );
  blk.set( "geo_roi", "39.773130421598160922 -84.120149319035462554 +100" );
#else
  blk.set( "crop_mode", "pixel_input" );
#endif
  TEST( "Set params", src.set_params( blk ), true );
  blk.set( "cache_mode", "BAD" );
  TEST( "Set params", src.set_params( blk ), false );
  blk.set( "cache_mode", "NONE" );
  TEST( "Set params", src.set_params( blk ), true );
  blk.set( "cache_mode", "RAM" );
  TEST( "Set params", src.set_params( blk ), true );
  blk.set( "cache_mode", "DISK" );
  TEST( "Set params", src.set_params( blk ), true );
} // test_config


void
test_non_set_port()
{
  std::cout << "\n\nTesting when input is not set\n\n";

  filename_frame_metadata_process<vxl_byte> src( "src" );

  config_block blk = src.params();

  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );

  // with no input image set on the port, this should fail
  TEST( "Step 0", src.step(), false );
}

} // end anonymous namespace



int filename_frame_metadata_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "filename_frame_metadata_process" );

#if defined(HAS_LTI_NITF2) && defined(HAS_VIL_GDAL) //&& is on purpose.  It was found
                                                    //if HAS_LTI_NITF2 but not HAS_VIL_GDAL,
                                                    //LTI cannot parse the metadata because
                                                    //a computers GDAL_DATA environment
                                                    //varable is not set.  Not all
                                                    //Dashboard has this set.
  vil_image_view<vxl_byte> ref_img_corner = vil_load((std::string(argv[1])+"/test_img_1.roi.png").c_str());
  vil_image_view<vxl_byte> ref_img_geo_sq = vil_load((std::string(argv[1])+"/test_img_1.roi2.png").c_str());
  vil_image_view<vxl_byte> ref_img_geo_xfm = vil_load((std::string(argv[1])+"/test_img_1.geo_xfm.png").c_str());
  corner_points lat_lon_truth, cp_truth;

  lat_lon_truth.ul[0] =  39.773129126468170114;
  lat_lon_truth.ul[1] = -84.120148023855549013;
  lat_lon_truth.ur[0] =  39.773129126468319328;
  lat_lon_truth.ur[1] = -84.111864053054077317;
  lat_lon_truth.lr[0] =  39.765726163405652471;
  lat_lon_truth.lr[1] = -84.111864053054276269;
  lat_lon_truth.ll[0] =  39.765726163405474836;
  lat_lon_truth.ll[1] = -84.120148023855776387;

  cp_truth.ul[0] =  39.775501804622557245;
  cp_truth.ul[1] = -84.12322278099787809;
  cp_truth.ur[0] =  39.775501804622656721;
  cp_truth.ur[1] = -84.117075857072933331;
  cp_truth.lr[0] =  39.770759038573771704;
  cp_truth.lr[1] = -84.117075857073089651;
  cp_truth.ll[0] =  39.770759038573665123;
  cp_truth.ll[1] = -84.123222780998034409;
  test_passed_roi( argv[1], lat_lon_truth, ref_img_corner, "NONE" );
  test_passed_roi( argv[1], lat_lon_truth, ref_img_corner, "RAM" );
  test_passed_roi( argv[1], lat_lon_truth, ref_img_corner, "DISK" );
  test_geo_roi( argv[1], lat_lon_truth, ref_img_corner,
                "39.773129126468170114 -84.120148023855549013 "
                "39.765726163405652471 -84.111864053054276269");
  test_geo_roi( argv[1], lat_lon_truth, ref_img_corner,
                "39.773129126468170114 -84.120148023855549013 "
                "39.773129126468319328 -84.111864053054077317 "
                "39.765726163405652471 -84.111864053054276269 "
                "39.765726163405474836 -84.120148023855776387");
  test_geo_roi( argv[1], lat_lon_truth, ref_img_corner,
                "16N 746653.955 4406544.761 "
                "16N 747363.571 4406567.631 "
                "16N 747390.094 4405745.816 "
                "16N 746680.401 4405722.947" );
  test_geo_roi( argv[1], cp_truth, ref_img_geo_sq,
                "39.773130415952032024 -84.120149216385385671 +255.35" );
  test_geo_roi( argv[1], cp_truth, ref_img_geo_sq,
                "16N 746653.848 4406544.90055 +255.35" );
  test_geo_roi( argv[1], cp_truth, ref_img_geo_sq,
                "16N 746398.49800000002142 4406289.5505500007421  "
                "16N 746909.19799999997485 4406800.250549999997" );
  test_geo_roi_utm_xform( argv[1], lat_lon_truth, ref_img_geo_xfm,
                          "16N 746653.883 4406545.045 16N 747363.717 4406567.971 "
                          "16N 747390.256 4405745.868 16N 746680.345 4405722.944" );
#else
  (void)argv;
#endif

  test_config();
  test_non_set_port();

  return testlib_test_summary();
}
