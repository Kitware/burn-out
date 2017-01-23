/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <cmath>
#include <iomanip>
#include <tracking_data/crop_track_to_aoi.h>

#include <geographic/geo_coords.h>

#include <utilities/geo_UTM.h>

#include <testlib/testlib_test.h>

using namespace vidtk;

namespace
{

void test_crop_track_to_aoi()
{
  aoi_sptr a = new vidtk::aoi();
  a->set_aoi_crop_string("500x500+1000+1000");
  a->set_geo_point_lat_lon( aoi::UPPER_LEFT, 42.850196, -73.759802);
  a->set_geo_point_lat_lon( aoi::UPPER_RIGHT, 42.849277, -73.759802);
  a->set_geo_point_lat_lon( aoi::LOWER_LEFT, 42.850196, -73.758132);
  a->set_geo_point_lat_lon( aoi::LOWER_RIGHT, 42.849277, -73.758132);
  timestamp ts1(0,0), ts2(10,10);
  a->set_start_time(ts1);
  a->set_end_time(ts2);

  geographic::geo_coords geo_inside(42.849817, -73.758964);
  geographic::geo_coords geo_outside(80.0003, 120.00001);

  track_state_sptr ts_in = new track_state();
  ts_in->set_timestamp(vidtk::timestamp(5,5));
  track_state_sptr ts_empty = new track_state();
  ts_empty->set_timestamp(vidtk::timestamp(5,5));
  track_state_sptr ts_outside = new track_state();
  ts_outside->set_timestamp(vidtk::timestamp(5,5));
  {
    ts_in->set_latitude_longitude( geo_inside.latitude(), geo_inside.longitude() );
    geo_coord::geo_UTM tmp_utm( geo_inside.zone(), geo_inside.is_north(), geo_inside.easting(), geo_inside.northing() );
    ts_in->set_smoothed_loc_utm( tmp_utm);
    image_object_sptr io = new image_object();
    io->set_image_loc( 1250, 1250 );
    ts_in->set_image_object(io);
  }

  {
    ts_outside->set_latitude_longitude( geo_outside.latitude(), geo_outside.longitude() );
    geo_coord::geo_UTM tmp_utm( geo_outside.zone(), geo_outside.is_north(), geo_outside.easting(), geo_outside.northing() );
    ts_outside->set_smoothed_loc_utm( tmp_utm);
    image_object_sptr io = new image_object();
    io->set_image_loc( 2250, 2250 );
    ts_outside->set_image_object(io);
  }

  //Test inside
  {
    track_sptr trk = new track();
    trk->add_state(ts_in);
    {
      crop_track_to_aoi cropper(aoi::UTM);
      TEST("UTM inside", cropper(trk, a)->history().size(), 1);
    }
    {
      crop_track_to_aoi cropper(aoi::LAT_LONG);
      TEST("Lat Long inside", cropper(trk, a)->history().size(), 1);
    }
    {
      crop_track_to_aoi cropper(aoi::PIXEL);
      TEST("Pixel inside", cropper(trk, a)->history().size(), 1);
    }
  }

  //Test outside
  {
    track_sptr trk = new track();
    trk->add_state(ts_outside);
    {
      crop_track_to_aoi cropper(aoi::UTM);
      TEST("UTM outside", cropper(trk, a), NULL);
    }
    {
      crop_track_to_aoi cropper(aoi::LAT_LONG);
      TEST("Lat Long outside", cropper(trk, a), NULL);
    }
    {
      crop_track_to_aoi cropper(aoi::PIXEL);
      TEST("Pixel outside", cropper(trk, a), NULL);
    }
  }

  //Test empty
  {
    track_sptr trk = new track();
    trk->add_state(ts_empty);
    {
      crop_track_to_aoi cropper(aoi::UTM);
      TEST("UTM empty", cropper(trk, a), NULL);
    }
    {
      crop_track_to_aoi cropper(aoi::LAT_LONG);
      TEST("Lat Long empty", cropper(trk, a), NULL);
    }
    {
      crop_track_to_aoi cropper(aoi::PIXEL);
      TEST("Pixel empty", cropper(trk, a), NULL);
    }
  }
}

}

int test_track_transformation_functions(int /*argc*/, char** const /*argv[]*/)
{
  testlib_test_start("test_track_transformation_functions");

  test_crop_track_to_aoi();

  return testlib_test_summary();
}
