/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>
#include <tracking/unique_track_id_process.h>

#include <pipeline/sync_pipeline.h>
#include <process_framework/process.h>
#include <tracking/track.h>
#include <tracking/tracking_keys.h>

#include <vcl_string.h>
#include <vcl_vector.h>


// Use ananymous namespace so that different tests don't conflict
namespace {

using namespace vidtk;

void
test_two_tracks()
{
  sync_pipeline p;

  unique_track_id_process test_process("utid");
  p.add( &test_process );

  config_block config = p.params();
  vcl_string map_filename = "test_map.txt";
  config.set_value("utid:map_filename", map_filename);

  TEST( "Set params", p.set_params( config ), true );
  TEST( "Initialize pipeline", p.initialize(), true );

  // Create two input tracks with same id, different tracker_subtype
  vcl_vector<track_sptr> active_tracks;
  vcl_vector<track_sptr> terminated_tracks;
  unsigned tracker_subtype;

  // Explicitly create tracks and states
  {
    // First active track, 1 state
    track_sptr trk( new track );
    trk->set_id(1);
    tracker_subtype = 1;
    trk->data().set(vidtk::tracking_keys::tracker_subtype, tracker_subtype);

    track_state_sptr st = new track_state;
    timestamp ts(10.0, 10);
    st->time_ = ts;
    trk->add_state(st);

    active_tracks.push_back( trk );
  }
  {
    // Second active track, 2 states
    track_sptr trk( new track );
    trk->set_id(1);
    tracker_subtype = 2;
    trk->data().set(vidtk::tracking_keys::tracker_subtype, tracker_subtype);

    track_state_sptr st1 = new track_state;
    timestamp ts1(11.0, 11);
    st1->time_ = ts1;
    trk->add_state(st1);

    track_state_sptr st2 = new track_state;
    timestamp ts2(12.0, 12);
    st2->time_ = ts2;
    trk->add_state(st2);

    active_tracks.push_back( trk );

    // First terminated track, clone of second active track
    track_sptr trk_clone = trk->clone();
    terminated_tracks.push_back( trk_clone );
  }

  // Set active tracks and execute process
  test_process.input_active_tracks( active_tracks );
  test_process.input_terminated_tracks( terminated_tracks );
  p.execute();

  // Make sure we still have 2 active tracks
  vcl_vector<track_sptr> output_active_tracks = test_process.output_active_tracks();
  TEST( "Output active tracks count ", output_active_tracks.size() == 2, true );
  TEST( "Output active track key count ", test_process.output_active_track_keys().size() == 2, true );
  TEST( "Output active track key map count ", test_process.output_track_map().size() == 2, true );

  // Verify that track ids are different
  unsigned id0 = output_active_tracks[0]->id();
  unsigned id1 = output_active_tracks[1]->id();
  TEST( "Active tracks have different ids", id0 != id1, true );

  // Verify that terminated track id matches active track with 2 states
  // We will presume that the track order hasn't changed
  vcl_vector<track_sptr> output_term_tracks = test_process.output_terminated_tracks();
  TEST( "Output terminated tracks count ", output_term_tracks.size() == 1, true );

  unsigned id_term = output_term_tracks[0]->id();
  TEST( "Terminated track id", id_term == id1, true );
}  // end test_two_tracks()

}  // end anonymous namespace


int test_unique_track_id_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "unique_track_id" );

  test_two_tracks();

  return testlib_test_summary();
}
