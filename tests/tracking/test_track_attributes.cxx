/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_sstream.h>
#include <testlib/testlib_test.h>
#include <vil/vil_image_view.h>
#include <tracking/track.h>

using namespace vidtk;


// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void test_state_attributes()
{
  // Test track state attributes
  track_state a_state;
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_ASSOC_FG_SSD), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_ASSOC_FG_CSURF), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_ASSOC_FG_GROUP), false);

  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_ASSOC_DA_KINEMATIC), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_ASSOC_DA_MULTI_FEATURES), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_ASSOC_DA_GROUP), false);

  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_KALMAN_ESH), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_KALMAN_LVEL), false);

  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_INTERVAL_FORWARD), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_INTERVAL_BACK), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_INTERVAL_INIT), false);

  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_LINKING_START), false);
  TEST ("Initial state attributes", a_state.has_attr(track_state::ATTR_LINKING_END), false);


  // ================================================================
  a_state.set_attr (track_state::ATTR_ASSOC_DA_KINEMATIC);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_SSD), false);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_CSURF), false);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_GROUP), false);

  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_KINEMATIC), true);
  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_MULTI_FEATURES), false);
  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_GROUP), true);


  a_state.set_attr (track_state::ATTR_ASSOC_FG_CSURF);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_SSD), false);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_CSURF), true);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_GROUP), true);

  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_KINEMATIC), false);
  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_MULTI_FEATURES), false);
  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_GROUP), false);


  a_state.clear_attr (track_state::ATTR_ASSOC_FG_CSURF);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_SSD), false);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_CSURF), false);
  TEST ("FG tracker group", a_state.has_attr(track_state::ATTR_ASSOC_FG_GROUP), false);

  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_KINEMATIC), false);
  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_MULTI_FEATURES), false);
  TEST ("DA tracker group", a_state.has_attr(track_state::ATTR_ASSOC_DA_GROUP), false);


  // ================================================================
  a_state.set_attr (track_state::ATTR_KALMAN_ESH);
  TEST ("Kalman attributes", a_state.has_attr(track_state::ATTR_KALMAN_ESH), true);
  TEST ("Kalman attributes", a_state.has_attr(track_state::ATTR_KALMAN_LVEL), false);


  a_state.set_attr (track_state::ATTR_KALMAN_LVEL);
  TEST ("Kalman attributes", a_state.has_attr(track_state::ATTR_KALMAN_ESH), false);
  TEST ("Kalman attributes", a_state.has_attr(track_state::ATTR_KALMAN_LVEL), true);


  a_state.clear_attr (track_state::ATTR_KALMAN_LVEL);
  TEST ("Kalman attributes", a_state.has_attr(track_state::ATTR_KALMAN_ESH), false);
  TEST ("Kalman attributes", a_state.has_attr(track_state::ATTR_KALMAN_LVEL), false);


  // ================================================================
  a_state.set_attr (track_state::ATTR_INTERVAL_INIT);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_FORWARD), false);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_BACK), false);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_INIT), true);


  a_state.set_attr (track_state::ATTR_INTERVAL_BACK);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_FORWARD), false);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_BACK), true);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_INIT), false);


  a_state.clear_attr (track_state::ATTR_INTERVAL_BACK);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_FORWARD), false);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_BACK), false);
  TEST ("Interval attributes", a_state.has_attr(track_state::ATTR_INTERVAL_INIT), false);


  // ================================================================
  a_state.set_attr (track_state::ATTR_LINKING_START);
  TEST ("tracker attributes", a_state.has_attr(track_state::ATTR_LINKING_START), true);
  TEST ("tracker attributes", a_state.has_attr(track_state::ATTR_LINKING_END), false);

  a_state.set_attr (track_state::ATTR_LINKING_END);
  TEST ("tracker attributes", a_state.has_attr(track_state::ATTR_LINKING_START), false);
  TEST ("tracker attributes", a_state.has_attr(track_state::ATTR_LINKING_END), true);

  a_state.clear_attr (track_state::ATTR_LINKING_END);
  TEST ("tracker attributes", a_state.has_attr(track_state::ATTR_LINKING_START), false);
  TEST ("tracker attributes", a_state.has_attr(track_state::ATTR_LINKING_END), false);
}


void test_track_attributes()
{
  track trk;

  TEST ("Initial state attributes", trk.has_attr(track::ATTR_AMHI), false);
  TEST ("Initial state attributes", trk.has_attr(track::ATTR_TRACKER_PERSON), false);
  TEST ("Initial state attributes", trk.has_attr(track::ATTR_TRACKER_VEHICLE), false);

  trk.set_attr(track::ATTR_TRACKER_VEHICLE);
  TEST ("Initial state attributes", trk.has_attr(track::ATTR_TRACKER_PERSON), false);
  TEST ("Initial state attributes", trk.has_attr(track::ATTR_TRACKER_VEHICLE), true);

  trk.set_attr(track::ATTR_TRACKER_PERSON);
  TEST ("Initial state attributes", trk.has_attr(track::ATTR_TRACKER_PERSON), true);
  TEST ("Initial state attributes", trk.has_attr(track::ATTR_TRACKER_VEHICLE), false);


  trk.set_attr(track::ATTR_AMHI);
  TEST ("set attributes", trk.has_attr(track::ATTR_AMHI), true);

  trk.clear_attr(track::ATTR_AMHI);
  TEST ("clear attributes", trk.has_attr(track::ATTR_AMHI), false);
}

} // end namespace



int test_track_attributes ( int , char* [] )
{
  testlib_test_start( "track attributes" );

  test_state_attributes();
  test_track_attributes();

  return testlib_test_summary();
}
