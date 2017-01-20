/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <sstream>

#include <vul/vul_temp_filename.h>
#include <vpl/vpl.h>
#include <vsl/vsl_binary_io.h>
#include <testlib/testlib_test.h>

#include <utilities/compute_transformations.h>
#include <utilities/video_metadata_util.h>
#include <utilities/homography.h>
#include <utilities/geo_lat_lon.h>


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void normalize( vgl_h_matrix_2d< double > & mat )
{
  vnl_matrix_fixed<double,3,3> matrix = mat.get_matrix();
  double const& factor = matrix[2][2];
  if (factor)
    mat = matrix / factor;
}

void simple_compute_homography()
{
  std::vector< vgl_homg_point_2d< double > > src_pts, dst_pts;
  src_pts.push_back( vgl_homg_point_2d< double >( 0.0, 0.0) );
  src_pts.push_back( vgl_homg_point_2d< double >( 10.0, 0.0) );
  src_pts.push_back( vgl_homg_point_2d< double >( 10.0, 10.0) );
  src_pts.push_back( vgl_homg_point_2d< double >( 0.0, 10.0) );

  dst_pts.push_back( vgl_homg_point_2d< double >( 0.0, 0.0) );
  dst_pts.push_back( vgl_homg_point_2d< double >( 2.0, 0.0) );
  dst_pts.push_back( vgl_homg_point_2d< double >( 2.0, 2.0) );
  dst_pts.push_back( vgl_homg_point_2d< double >( 0.0, 2.0) );

  vgl_h_matrix_2d<double> H;

  TEST( "simple test 1 compute_homography()", true,
    vidtk::compute_homography( src_pts, dst_pts, H) );

  normalize( H );
  TEST_NEAR( "Valid scale x", H.get(0,0), 0.2, 1e-10 );
  TEST_NEAR( "Valid scale y", H.get(1,1), 0.2, 1e-10 );
}

void simple_image_to_geographic_homography()
{
  vidtk::video_metadata md;
  md.corner_ul( vidtk::geo_coord::geo_lat_lon( -84.1217543666, 39.772375476 ) );
  md.corner_ur( vidtk::geo_coord::geo_lat_lon( -84.1134753907, 39.7723753787) );
  md.corner_lr( vidtk::geo_coord::geo_lat_lon( -84.1134749133, 39.7649697068) );

  std::pair<unsigned, unsigned> img_wh( 3196, 2858 );

  vgl_h_matrix_2d<double> H_img2wld;
  vidtk::plane_to_utm_homography H_wld2utm;

  TEST( "simple missing corner", false,
    vidtk::compute_image_to_geographic_homography( md, img_wh,H_img2wld, H_wld2utm ) );
  md.corner_ll( vidtk::geo_coord::geo_lat_lon( -84.1217539813, 39.7649698041) );

  md.is_valid( false );
  TEST( "simple missing valid metadata packet", false,
    vidtk::compute_image_to_geographic_homography( md, img_wh,H_img2wld, H_wld2utm ) );
  md.is_valid( true );

  TEST( "simple compute_image_to_geographic_homography()", true,
    vidtk::compute_image_to_geographic_homography( md, img_wh,H_img2wld, H_wld2utm ) );
}

void test_extract_lat_long_corner()
{
  vidtk::video_metadata meta;
  std::stringstream ss("1310663344000000 42.849669 -73.759223 879.1119384765625 "
                       "-1.3428144454956054688 11.648304939270019531 233.1114654541015625 "
                       "0 0 0 444 444 444 444 444 444 444 444 772.85107421875 "
                       "0.91187912225723266602 0.68390935659408569336 42.8499992 -77.759254");
  ascii_deserialize( ss, meta );
  std::vector< vidtk::geo_coord::geo_lat_lon > latlon_pts;
  TEST( "Extract Corner points for metadata: ", extract_latlon_corner_points( meta, 720, 480, latlon_pts ), true);

#define TEST_POINT( LATLONG, TRUE_LAT, TRUE_LONG )\
  TEST_NEAR( "TEST Point Lat: ", LATLONG.get_latitude(), TRUE_LAT, 0.000005 );\
  TEST_NEAR( "TEST Point Long: ", LATLONG.get_longitude(), TRUE_LONG, 0.000005 )\

  TEST_POINT( latlon_pts[0], 42.76442387225091, -70.48399675064043);
  TEST_POINT( latlon_pts[1], 42.72618583937669, -70.4872521867961);
  TEST_POINT( latlon_pts[2], 42.87203970145575, -75.00534697140654);
  TEST_POINT( latlon_pts[3], 42.85746565700939, -75.00552654811555);
}

void test_computed_corner_point_homography()
{
  vidtk::video_metadata meta;
  std::stringstream ss("1310663344000000 42.849669 -73.759223 879.1119384765625 "
                       "-1.3428144454956054688 11.648304939270019531 233.1114654541015625 "
                       "0 0 0 444 444 444 444 444 444 444 444 772.85107421875 "
                       "0.91187912225723266602 0.68390935659408569336 42.8499992 -77.759254");
  ascii_deserialize(ss, meta);

  vgl_h_matrix_2d<double> H_img2wld;
  vidtk::plane_to_utm_homography H_wld2utm;

  TEST( "Test computing the homography",
        vidtk::compute_image_to_geographic_homography( meta, std::pair<unsigned, unsigned>(720,480), H_img2wld, H_wld2utm ), true);

  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(0,0), -0.0140623, 1e-5 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(0,1), 163.918, 1e-3 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(0,2), 48082.6, 0.1 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(1,0), -1.54047, 1e-5 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(1,1), -1.49452, 1e-3 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(1,2), 115.451, 0.01 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(2,0), -6.76987e-19, 1e-18 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(2,1), -0.0019714, 1e-6 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(2,2), 0.259914, 1e-5 );

  vidtk::utm_zone_t utm = H_wld2utm.get_dest_reference();
  TEST( "UTM is correct zone: ", utm.zone(), 18 );
  TEST( "UTM is north: ", utm.is_north(), true );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(0,0), 1, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(0,1), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(0,2), 684530.87709090, 1e-6 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(1,0), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(1,1), 1, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(1,2), 4744109.4911270, 1e-6 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(2,0), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(2,1), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(2,2), 1, 1e-10 );
}

void test_given_corner_point_homograph()
{
  vidtk::video_metadata meta;
  std::stringstream ss("1256157126875 444 444 0 0 0 0 0 0 0 39.785872688237120087 "
                      "-84.100546233855993705 39.785872328639420914 -84.09684976878645557 "
                      "39.781277492360743508 -84.096849777410469073 39.781277851948104285 "
                      "-84.100546242468993796 0 0 0 39.783573343260144384 "
                      "-84.098697844090594344 1");
  ascii_deserialize(ss, meta);

  vgl_h_matrix_2d<double> H_img2wld;
  vidtk::plane_to_utm_homography H_wld2utm;

  TEST( "Test computing the homography",
        vidtk::compute_image_to_geographic_homography( meta, std::pair<unsigned, unsigned>(720,480), H_img2wld, H_wld2utm ), true);
  std::cout << H_img2wld << std::endl << H_wld2utm << std::endl;

  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(0,0), -0.227388, 1e-5 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(0,1), -0.0178329, 1e-6 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(0,2), 86.0171, 0.001 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(1,0), -0.00734552, 1e-7 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(1,1), 0.54991, 1e-4 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(1,2), -129.067, 0.01 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(2,0), 1.38548e-09, 1e-8 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(2,1), 7.1812e-08, 1e-7 );
  TEST_NEAR( "TEST H_img2wld: ", H_img2wld.get(2,2), -0.516418, 1e-5 );

  vidtk::utm_zone_t utm = H_wld2utm.get_dest_reference();
  TEST( "UTM is correct zone: ", utm.zone(), 16 );
  TEST( "UTM is north: ", utm.is_north(), true );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(0,0), 1, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(0,1), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(0,2), 748453.7923879669, 1e-6 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(1,0), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(1,1), 1, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(1,2), 4407763.744729394, 1e-6 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(2,0), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(2,1), 0, 1e-10 );
  TEST_NEAR( "TEST H_img2wld: ", H_wld2utm.get_transform().get(2,2), 1, 1e-10 );
}

} // end anonymous namespace

int test_compute_transformation( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "compute_transformation" );

  simple_compute_homography();
  simple_image_to_geographic_homography();

  test_extract_lat_long_corner();
  test_computed_corner_point_homography();
  test_given_corner_point_homograph();

  return testlib_test_summary();
}
