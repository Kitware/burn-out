/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <vil/vil_load.h>
#include <vil/algo/vil_threshold.h>
#include <testlib/testlib_test.h>

#include <tracking_data/image_object.h>
#include <object_detectors/filter_image_objects_process.h>
#include <utilities/compute_transformations.h>

#include <vxl_config.h>


int test_filter_image_objects_process( int argc, char* argv[] )
{
  using namespace vidtk;

  if (argc < 2)
  {
    return 1;
  }

  testlib_test_start( "filter image objects process" );

  std::string g_data_dir = argv[1];

  std::vector< image_object_sptr > no_objs;

  std::vector< image_object_sptr > src_objs;
  {
    image_object_sptr o = new image_object;
    o->set_area( 10 );
    src_objs.push_back( o );
  }
  {
    image_object_sptr o = new image_object;
    o->set_area( 1 );
    src_objs.push_back( o );
  }
  {
    image_object_sptr o = new image_object;
    o->set_area( 15 );
    src_objs.push_back( o );
  }
  {
    image_object_sptr o = new image_object;
    o->set_area( 3 );
    src_objs.push_back( o );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "min_area", "5" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( src_objs );
    TEST( "Step 1", f1.step(), true );
    bool good = true;
    for( unsigned i = 0; i < f1.objects().size(); ++i )
    {
      std::cout << "Remaining object " << i
                << ": area = " << f1.objects()[i]->get_area() << "\n";
      if( f1.objects()[i]->get_area() < 5 )
      {
        good = false;
      }
    }
    TEST( "Remaining areas are valid", good, true );
    TEST( "Two objects remained", f1.objects().size(), 2 );

    f1.set_source_objects( no_objs );
    TEST( "Step 2", f1.step(), true );
    TEST( "No object after second time", f1.objects().empty(), true );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "min_area", "25" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( no_objs );
    TEST( "Step 1", f1.step(), true );
    TEST( "No objects left", f1.objects().empty(), true );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "max_area", "5" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( src_objs );
    TEST( "Step 1", f1.step(), true );
    bool good = true;
    for( unsigned i = 0; i < f1.objects().size(); ++i )
    {
      std::cout << "Remaining object " << i
                << ": area = " << f1.objects()[i]->get_area() << "\n";
      if( f1.objects()[i]->get_area() > 5 )
      {
        good = false;
      }
    }
    TEST( "Remaining areas are valid", good, true );
    TEST( "Two objects remained", f1.objects().size(), 2 );

    f1.set_source_objects( no_objs );
    TEST( "Step 2", f1.step(), true );
    TEST( "No object after second time", f1.objects().empty(), true );
  }

  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    blk.set( "max_area", "0" );
    TEST( "Set params", f1.set_params( blk ), true );

    f1.set_source_objects( no_objs );
    TEST( "Step 1", f1.step(), true );
    TEST( "No objects left", f1.objects().empty(), true );
  }

  {
    // Test image_object filtering based set 'image' bounds
    // with a triangular filtered region as a string and
    // a second test with 2 opposing triangular regions
    // which are provided by a file.
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();

    image_object_sptr o = new image_object;
    std::vector< image_object_sptr > objs;
    objs.push_back( o );

    // Construct the filter region
    // A triangle in the upper left quadrant
    std::string poly_str = "-73.76038695126473 42.850873 "
      "-73.7577700485445 42.850873 -73.76038695126473 42.849013";

    std::string file_poly_str = g_data_dir + "/image_obj_filter_region";

    // Set up and compute the homography for my 'image' bounds, which is roughly kitware+some
    std::vector< vgl_homg_point_2d< double > > img_pts(4);
    img_pts[0] = vgl_homg_point_2d< double > ( 0, 0 );
    img_pts[1] = vgl_homg_point_2d< double > ( 50, 0 );
    img_pts[2] = vgl_homg_point_2d< double > ( 50, 50 );
    img_pts[3] = vgl_homg_point_2d< double > ( 0, 50 );

    std::vector< geographic::geo_coords > latlon_pts(4);
    latlon_pts[0] = geographic::geo_coords( 42.850873, -73.76038695126473 );
    latlon_pts[1] = geographic::geo_coords( 42.850873, -73.75515314582427 );
    latlon_pts[2] = geographic::geo_coords( 42.847153, -73.75515314582427 );
    latlon_pts[3] = geographic::geo_coords( 42.847153, -73.76038695126473 );

    // Compute the homographies
    image_to_utm_homography H_img2utm;
    image_to_plane_homography H_img2wld;
    plane_to_utm_homography H_wld2utm;
    compute_image_to_geographic_homography( img_pts, latlon_pts, H_img2wld, H_wld2utm);
    vgl_h_matrix_2d<double> img2utm = H_wld2utm.get_transform() * H_img2wld.get_transform();

    // Populate Homography
    H_img2utm.set_transform( img2utm );
    utm_zone_t zone( 18, true );
    H_img2utm.set_dest_reference( zone );

#define TEST_STEP(MSG, SIZE)                   \
    f1.set_src_to_utm_homography( H_img2utm ); \
    f1.set_source_objects( objs );             \
    TEST( "Test step", f1.step(), true );      \
    TEST( MSG, f1.objects().size(), SIZE )

#define SET_PARAMS(PARAM, VALUE, MSG, RESULT) \
    blk.set( PARAM, VALUE );                  \
    TEST( MSG, f1.set_params( blk ), RESULT )

#define SET_POINT(X, Y)       \
    objs[0]->set_image_loc( X, Y )

    // Disable filtering, step keeps all tracks
    SET_PARAMS("geofilter_enabled", false, "Set geofilter in params", true);
    TEST_STEP("Geofiltering disabled, still have 1 object", 1);

    // Now enable filtering and don't pass region, should be false
    SET_PARAMS( "geofilter_enabled", "true",
                "set_params returns false when geofilter_enabled "
                "and no geofilter_polygon set", false );

    // Now set a region that is point is outside, keeping track
    SET_PARAMS("geofilter_polygon", poly_str, "Set geofilter_polygon in params", true);
    SET_POINT(30, 30);
    TEST_STEP( "Object is outside region, still have 1 object", 1 );

    // Now set a region that overlaps, contains properly keeps it.
    SET_POINT(25, 0);
    TEST_STEP( "Object is on region border, still have 1 object", 1 );

    // Now a point inside the region. more objects.
    SET_POINT(10, 10);
    TEST_STEP( "Object is inside region, no objects", 0 );

    // Now run the tests with polygon from file.
    SET_PARAMS("geofilter_polygon", file_poly_str, "Set file geofilter_polygon", true);
    SET_POINT(30, 30);
    TEST_STEP( "Object is outside region, still have 1 object", 1 );

    // Now set a region that overlaps, contains properly keeps it.
    SET_POINT(25, 0);
    TEST_STEP( "Object is on region border, still have 1 object", 1 );

    // Now a point inside the upper region. more objects.
    SET_POINT(10, 10);
    TEST_STEP( "Object is inside region, no objects", 0 );

    // Now a point inside the lower region. more objects.
    SET_POINT(45, 45);
    TEST_STEP( "Object is inside region, no objects", 0 );

  }

  //Test image threshold filtering
  std::cout << "\nTesting image_threshold filtering...\n";
  {
    filter_image_objects_process< vxl_byte > f1( "f1" );
    config_block blk = f1.params();
    //blk.set( "diff_threshold", "126" ); //color of squares in foursquare.png
    blk.set( "filter_binary_image", "true" );
    blk.set( "min_area", "-1" );
    blk.set( "max_area", "-1" );


    std::vector< image_object_sptr > th_objs;

    //Load image and set new objects' bounding boxes to sections of image
    vil_image_view< unsigned char > img = vil_load( ( g_data_dir + "/foursquare.png" ).c_str() );

    TEST( "Loaded test image", ! img, false );

    TEST( "Set params", f1.set_params( blk ), true );

    {
      image_object_sptr o = new image_object;

      //inside top left square, should be accepted
      vgl_box_2d< unsigned > *bbox = new vgl_box_2d< unsigned >(45, 50, 45, 50 );
      o->set_bbox( *bbox );

      vil_image_view< bool > mask;
      image_object::image_point_type mask_origin( 0, 0 );
      vil_threshold_above( img, mask, static_cast<unsigned char >( 0 ) ); //create mask of all (or at least mostly) 1's

      o->set_object_mask( mask, mask_origin );

      th_objs.push_back( o );
    }

    {
      image_object_sptr o = new image_object;

      //inside top left square, should be accepted
      vgl_box_2d< unsigned > *bbox = new vgl_box_2d< unsigned >(30, 35, 30, 35 );
      o->set_bbox( *bbox );

      vil_image_view< bool > mask;
      image_object::image_point_type mask_origin( 0, 0 );
      vil_threshold_below( img, mask, static_cast< unsigned char >( 255 ) ); //create mask of all (or at least mostly) 1's

      o->set_object_mask( mask, mask_origin );

      th_objs.push_back( o );
    }

    vil_image_view<bool> binary_img;
    vil_threshold_above( img, binary_img, static_cast<unsigned char >( 126 ) ); //color of squares in foursquare.png

    //Add image and objects to objects filter
    f1.set_source_objects( th_objs );
    f1.set_binary_image( binary_img );

    //Run threshold filtering...
    TEST( "Step 1", f1.step(), true );
    TEST( "Only 1 object left", f1.objects().size(), 1 );

  }

  return testlib_test_summary();
}
