/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
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

#include <video_io/frame_metadata_super_process.h>

#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>

#include <boost/thread/thread.hpp>


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

void tcp_tread_function( frame_metadata_super_process<vxl_byte> * src, corner_points * cp, vil_image_view<vxl_byte> * ref_img)
{
  TEST( "Step", src->step(), true );
  timestamp ts = src->timestamp();
  vil_image_view<vxl_byte> img = src->image();
  video_metadata vm = src->metadata();
  //Test timestamp
  TEST("Timestamp is valid:", ts.is_valid(), true);
  TEST("Frame:", ts.frame_number(), 1);
  TEST_NEAR("Time:", ts.time(), 1256157126875000, 1e-7);
  std::cout << ts << std::endl;
  //Test Image
  TEST("Image values are same", equ(img,*ref_img), true);
  //Test metadata
  TEST("Video metadata is valid:", vm.is_valid(), true);
  std::cout << std::setprecision(20)
  << vm.corner_ul().get_latitude() << " " << vm.corner_ul().get_longitude() << " "
  << vm.corner_ur().get_latitude() << " " << vm.corner_ur().get_longitude() << " "
  << vm.corner_lr().get_latitude() << " " << vm.corner_lr().get_longitude() << " "
  << vm.corner_ll().get_latitude() << " " << vm.corner_ll().get_longitude()<< std::endl;
  TEST_NEAR("VM UL (lat):", vm.corner_ul().get_latitude(), cp->ul[0], 2e-5);
  TEST_NEAR("VM UL (lon):", vm.corner_ul().get_longitude(), cp->ul[1], 2e-5);

  TEST_NEAR("VM UR (lat):", vm.corner_ur().get_latitude(), cp->ur[0], 2e-5);
  TEST_NEAR("VM UR (lon):", vm.corner_ur().get_longitude(), cp->ur[1], 2e-5);

  TEST_NEAR("VM LR (lat):", vm.corner_lr().get_latitude(), cp->lr[0], 2e-5);
  TEST_NEAR("VM LR (lon):", vm.corner_lr().get_longitude(), cp->lr[1], 2e-5);

  TEST_NEAR("VM LL (lat):", vm.corner_ll().get_latitude(), cp->ll[0], 2e-5);
  TEST_NEAR("VM LL (lon):", vm.corner_ll().get_longitude(), cp->ll[1], 2e-5);

}

void write_handler(const boost::system::error_code &/*ec*/, std::size_t /*bytes_transferred*/)
{
}


void test_passed_roi( corner_points               cp,
                      std::string                  pass,
                      std::string                  geo,
                      std::string                  mode,
                      vil_image_view< vxl_byte >  ref_img  )
{
  // process should find the metadata files with a path relative to the image
  // directory.

  frame_metadata_super_process< vxl_byte > src( "src" );

  config_block blk = src.params();
  blk.set( "list_reader:type", "tcp" );
  blk.set( "list_reader:tcp:port", "10010" );
  blk.set( "filename_frame_metadata:metadata_mode", "NITF" );
  blk.set( "filename_frame_metadata:framenum_regex", "_([0-9]+).ntf" );
  blk.set( "filename_frame_metadata:cache_mode", "NONE" );
  blk.set( "filename_frame_metadata:use_latlon_for_xfm", "true" );
  blk.set( "filename_frame_metadata:geo_roi", geo );
  blk.set( "decoder_type", "filename_frame_metadata" );
  blk.set( "filename_frame_metadata:crop_mode", mode );


  TEST( "Set params", src.set_params( blk ), true );
  TEST( "Initialize", src.initialize(), true );
  //src.set_filename(dir+"/test_img_1.ntf");
  //src.set_pixel_roi("3199x2859+11174+14725");
  boost::thread worker( &tcp_tread_function, &src, &cp, &ref_img );
  boost::this_thread::sleep( boost::posix_time::seconds( 2 ) );
  boost::asio::io_service io_service;
  boost::asio::ip::tcp::endpoint endpoint( boost::asio::ip::address_v4::from_string( "127.0.0.1" ), 10010 );
  boost::asio::ip::tcp::socket sock( io_service );
  sock.connect( endpoint );
  boost::asio::async_write( sock, boost::asio::buffer( pass ), write_handler );
  sock.close();
  worker.join();
}



void
test_config()
{
  std::cout << "\n\nTesting when input is not set\n\n";

  frame_metadata_super_process<vxl_byte> src( "src" );

  config_block blk = src.params();
  blk.set( "list_reader:type", "tcp" );
  blk.set( "decoder_type", "BAD" );
  TEST( "Set params (decoder_type BAD)", src.set_params( blk ), false );
  blk.set( "decoder_type", "frame_metadata_decoder" );
  TEST( "Set params (decoder_type good)", src.set_params( blk ), true );
#if defined(HAS_LTI_NITF2) || defined(HAS_VIL_GDAL)
  blk.set( "filename_frame_metadata:metadata_mode", "NITF" );
  blk.set( "filename_frame_metadata:geo_roi", "39.773130421598160922 -84.120149319035462554 +100" );
  blk.set( "decoder_type", "filename_frame_metadata" );
  TEST( "Set params (params good)", src.set_params( blk ), true );
#endif
  blk.set( "decoder_type", "filename_frame_metadata" );
  blk.set( "filename_frame_metadata:metadata_mode", "BAD" );
  TEST( "Set params (metadata_mode BAD)", src.set_params( blk ), false );
}

} // end anonymous namespace

int test_frame_metadata_super_process( int argc, char* argv[] )
{
  if( argc < 2 )
  {
    std::cerr << "Need the data directory as an argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "frame_metadata_super_process" );

#if defined(HAS_LTI_NITF2) && defined(HAS_VIL_GDAL) //&& is on purpose.  It was found
                                                    //if HAS_LTI_NITF2 but not HAS_VIL_GDAL,
                                                    //LTI cannot parse the metadata because
                                                    //a computers GDAL_DATA environment
                                                    //varable is not set.  Not all
                                                    //Dashboard has this set.
  vil_image_view<vxl_byte> ref_img_corner = vil_load((std::string(argv[1])+"/test_img_1.roi.png").c_str());
  corner_points lat_lon_truth;

  lat_lon_truth.ul[0] =  39.773129126468170114;
  lat_lon_truth.ul[1] = -84.120148023855549013;
  lat_lon_truth.ur[0] =  39.773129126468319328;
  lat_lon_truth.ur[1] = -84.111864053054077317;
  lat_lon_truth.lr[0] =  39.765726163405652471;
  lat_lon_truth.lr[1] = -84.111864053054276269;
  lat_lon_truth.ll[0] =  39.765726163405474836;
  lat_lon_truth.ll[1] = -84.120148023855776387;

  //test_passed_roi( corner_points cp, std::string pass, std::string geo, std::string mode, vil_image_view<vxl_byte> ref_img  )
  test_passed_roi( lat_lon_truth, std::string(argv[1])+"/test_img_1.ntf:3199x2859+11174+14725\n", "",
                   "pixel_input", ref_img_corner );
  test_passed_roi( lat_lon_truth, std::string(argv[1])+"/test_img_1.ntf\n",
                   "39.773129126468170114 -84.120148023855549013 "
                   "39.773129126468319328 -84.111864053054077317 "
                   "39.765726163405652471 -84.111864053054276269 "
                   "39.765726163405474836 -84.120148023855776387",
                   "geographic_config" , ref_img_corner);
#else
  (void)argv;
#endif

  test_config();

  return testlib_test_summary();
}
