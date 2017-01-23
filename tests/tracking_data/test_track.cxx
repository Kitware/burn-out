/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <cmath>
#include <tracking_data/pvo_probability.h>
#include <tracking_data/io/track_reader.h>
#include <tracking_data/tracking_keys.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_resource.h>
#include <iomanip>

using namespace vidtk;

void test_track_state_get_location()
{
  track_state ts;
  //test empty ts;
  vnl_vector_fixed<double,3> loc;
  //enum location_type {stabilized_plane, unstabilized_world, stabilized_world};
  TEST("Test loc (it always there even if not set)", ts.get_location(track_state::stabilized_plane, loc), true);
  TEST("Test unstabilized_world", ts.get_location(track_state::unstabilized_world, loc), false);
  TEST("Test stabilized_world", ts.get_location(track_state::stabilized_world, loc), false);

  ts.loc_[0] = 100;
  ts.loc_[1] = 150;
  TEST("Test loc (it always there even if not set)", ts.get_location(track_state::stabilized_plane, loc), true);
  TEST("Test loc (it always there even if not set)", loc[0], 100);
  TEST("Test loc (it always there even if not set)", loc[1], 150);

  ts.set_latitude_longitude(35,160);
  TEST("Test unstabilized_world (1)", ts.get_location(track_state::unstabilized_world, loc), true);
  TEST_NEAR("Test unstabilized_world", loc[0], 591253.25283392297569662, 1e6);
  TEST_NEAR("Test unstabilized_world", loc[1], 3873499.8508478724397719, 1e6);
  TEST("Test unstabilized_world (2)", ts.get_location(track_state::unstabilized_world, loc), true);
  TEST_NEAR("Test unstabilized_world", loc[0], 591253.25283392297569662, 1e6);
  TEST_NEAR("Test unstabilized_world", loc[1], 3873499.8508478724397719, 1e6);
  vidtk::geo_coord::geo_UTM geo(10, true, 166043, 1094443);
  std::cout << " " << geo << std::endl;
  ts.set_smoothed_loc_utm(geo);
  TEST("Test stabilized_world", ts.get_location(track_state::stabilized_world, loc), true);
  TEST("Test stabilized_world", loc[0], 166043);
  TEST("Test stabilized_world", loc[1], 1094443);
}

void test_track_state_cov()
{
  track_state ts;
  vnl_double_2x2 cov = ts.location_covariance();
  TEST("Test that default covariance is identity.", cov.is_identity(), true);

  cov.fill(2);
  ts.set_location_covariance(cov);

  vnl_double_2x2 const& new_cov = ts.location_covariance();
  TEST("Test input and output covariance matrix is the same.", new_cov == cov, true);
}

int test_track(int argc, char* argv[])
{
  testlib_test_start("test vidtk track data structure");
  test_track_state_get_location();
  test_track_state_cov();

  if ( argc < 2 )
  {

    TEST("DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  //Load in kw18 track with pixel information
  std::string dataPath(argv[1]);
  dataPath += "/vidtk_track_test.kw18";
  std::string imagePath(argv[1]);
  imagePath += "/score_tracks_data/test_frames";
  std::vector< vidtk::track_sptr > tracks;
  vidtk::track_reader* reader = new vidtk::track_reader( dataPath );

  vidtk::ns_track_reader::track_reader_options opt;
  opt.set_path_to_images(imagePath);

  reader->update_options (opt);

  TEST ("Opening track file", reader->open(), true );

  //Test the deep copy of the track
  TEST("Read kw18 file", reader->read_all(tracks), 1);
  TEST("There is 1 Track", tracks.size(), 1);
  vidtk::track_sptr track_orig = tracks[0];
  track_orig->set_tracker_type(track::TRACKER_TYPE_PERSON);
  track_orig->set_tracker_id(14);
  track_orig->set_pvo(pvo_probability(0.5, 0.4, 0.1));

  std::string note;
  track_orig->add_note("this is a note1");
  track_orig->get_note(note);
  TEST("Track note", note, "this is a note1");
  track_orig->add_note("this is a note2");
  track_orig->get_note(note);
  TEST("Track notes", note, "this is a note1\nthis is a note2");

  vidtk::track_sptr track_copy = track_orig->clone();
  TEST("Tracker type access", track_orig->get_tracker_type(), track::TRACKER_TYPE_PERSON);
  TEST("Tracker id access", track_orig->get_tracker_id(), 14);

  TEST("Tracker type access clone", track_copy->get_tracker_type(), track::TRACKER_TYPE_PERSON);
  TEST("Tracker id access clone", track_copy->get_tracker_id(), 14);

  vidtk::pvo_probability pvo_orig;
  bool has_pvo = track_orig->get_pvo(pvo_orig);
  TEST("Track has pvo", has_pvo, true);
  double pvo_diff = std::fabs(pvo_orig.get_probability_person() - 0.5) +
    std::fabs(pvo_orig.get_probability_vehicle() - 0.4) +
    std::fabs(pvo_orig.get_probability_other() - 0.1);
  TEST("Track pvo", pvo_diff < 0.001, true);

  std::cout << "Testing Deep Copy: " << std::endl;
  std::cout << "Pointers are differnt for:" << std::endl;
  TEST("track pointers", track_orig == track_copy, false);

  std::vector< track_state_sptr > orig_history = track_orig->history();
  std::vector< track_state_sptr > copy_history = track_copy->history();
  TEST("history pointers", orig_history ==  copy_history, false);

  track_state_sptr orig_state = orig_history[0];
  track_state_sptr copy_state = copy_history[0];
  TEST("state pointers", orig_state == copy_state, false);

  std::vector< vidtk::image_object_sptr > orig_objs, copy_objs;

  orig_state->data_.key_map().begin();
  orig_state->data_.get(vidtk::tracking_keys::img_objs, orig_objs);
  copy_state->data_.get(vidtk::tracking_keys::img_objs, copy_objs);

  TEST("object pointers", orig_objs == copy_objs, false);

  vil_image_resource_sptr orig_pxl_gen;
  vil_image_resource_sptr copy_pxl_gen;
  unsigned int orig_pxl_data_buff = 0;
  unsigned int copy_pxl_data_buff = 1;

  orig_objs[0]->get_image_chip( orig_pxl_gen, orig_pxl_data_buff );
  copy_objs[0]->get_image_chip( copy_pxl_gen, copy_pxl_data_buff );

  vil_image_view<vxl_byte> orig_pxl_data = static_cast< vil_image_view<vxl_byte> >(*(orig_pxl_gen->get_view()));
  vil_image_view<vxl_byte> copy_pxl_data = static_cast< vil_image_view<vxl_byte> >(*(copy_pxl_gen->get_view()));

  TEST("pxl data pointers", orig_pxl_gen == copy_pxl_gen, false);
  TEST("pxl buffer", orig_pxl_data_buff == copy_pxl_data_buff, true);

  //Make sure the data is the same but the pointers are different
  bool data_equal = true;
  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < 10; j++)
    {
      for (int p = 0; p < 3; p++)
      {
        if ( orig_pxl_data(i, j, p) != copy_pxl_data(i, j, p) )
        {
          data_equal = false;
        }
      }
    }
  }
  TEST("pxl data values", data_equal, true);

  //Test the UTM functions
  {
    vidtk::track_state ts;
    ts.loc_[0] = 165;
    ts.loc_[1] = 255;
    ts.vel_[0] = 0.5;
    ts.vel_[1] = 1.0;
    vidtk::image_object_sptr io = new image_object();
    io->set_world_loc( 160, 250, 0);
    ts.set_image_object(io);
    vidtk::plane_to_utm_homography homog;
    homog.set_dest_reference(vidtk::utm_zone_t(10,true));
    vidtk::plane_to_utm_homography::transform_t h;
    h.set_identity();
    h.set(0,2, 100000);
    h.set(1,2, 1000000);
    homog.set_transform(h);
    ts.fill_utm_based_fields(homog);

    vidtk::geo_coord::geo_UTM geo;
    ts.get_smoothed_loc_utm( geo );
    TEST( "is_north", geo.is_north(), true );
    TEST( "zone", geo.get_zone(), 10 );
    TEST_NEAR( "easting", geo.get_easting(), 100165, 1e-27);
    TEST_NEAR( "northing", geo.get_northing(), 1000255, 1e-27);

    ts.get_raw_loc_utm( geo );
    TEST( "is_north", geo.is_north(), true );
    TEST( "zone", geo.get_zone(), 10 );
    TEST_NEAR( "easting", geo.get_easting(), 100160, 1e-27 );
    TEST_NEAR( "northing", geo.get_northing(), 1000250, 1e-27 );
    vnl_vector_fixed<double, 2> vel;
    ts.get_utm_vel( vel );
    TEST_NEAR( "vel 0", vel[0], 0.5, 1e-27 );
    TEST_NEAR( "vel 1", vel[1], 1.0, 1e-27 );
  }

  TEST("tracker type unknown", track::convert_tracker_type(track::TRACKER_TYPE_UNKNOWN), std::string("unknown"));
  TEST("tracker type person", track::convert_tracker_type(track::TRACKER_TYPE_PERSON), std::string("person"));
  TEST("tracker type vehicle", track::convert_tracker_type(track::TRACKER_TYPE_VEHICLE), std::string("vehicle"));

  TEST("tracker type unknown", track::convert_tracker_type("unknown"), track::TRACKER_TYPE_UNKNOWN);
  TEST("tracker type person", track::convert_tracker_type("person"), track::TRACKER_TYPE_PERSON);
  TEST("tracker type vehicle", track::convert_tracker_type("vehicle"), track::TRACKER_TYPE_VEHICLE);

  // Test pvo
  vidtk::pvo_probability pvo_copy;
  has_pvo = track_copy->get_pvo(pvo_copy);
  TEST("Copy has pvo", has_pvo, true);
  pvo_diff = std::fabs(pvo_orig.get_probability_person() - pvo_copy.get_probability_person()) +
    std::fabs(pvo_orig.get_probability_vehicle() - pvo_copy.get_probability_vehicle()) +
    std::fabs(pvo_orig.get_probability_other() - pvo_copy.get_probability_other());
  TEST("Copy pvo same", pvo_diff < 0.001, true);

  // reverse PV
  track_copy->set_pvo(vidtk::pvo_probability(0.4, 0.5, 0.1));
  vidtk::pvo_probability check_pvo;
  has_pvo = track_copy->get_pvo(check_pvo);
  pvo_diff = std::fabs(pvo_orig.get_probability_person() - check_pvo.get_probability_person()) +
    std::fabs(pvo_orig.get_probability_vehicle() - check_pvo.get_probability_vehicle()) +
    std::fabs(pvo_orig.get_probability_other() - check_pvo.get_probability_other());
  TEST("Copy pvo diff", std::fabs(pvo_diff-0.2) < 0.001, true);

  // Test pvo using default copy constructor
  vidtk::track_sptr track_ccopy  = new vidtk::track(*track_orig);
  track_ccopy->set_pvo(pvo_probability(0.2, 0.6, 0.2));
  has_pvo = track_orig->get_pvo(check_pvo);
  pvo_diff = std::abs(check_pvo.get_probability_person() - 0.5) +
    std::fabs(check_pvo.get_probability_vehicle() - 0.4) +
    std::fabs(check_pvo.get_probability_other() - 0.1);
  TEST("Copy constructor pvo diff", pvo_diff < 0.001, true);

  delete reader;
  return testlib_test_summary();
} /* test_track */
