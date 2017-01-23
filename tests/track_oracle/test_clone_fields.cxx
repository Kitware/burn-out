/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>
#include <cstdlib>

#include <track_oracle/track_oracle.h>
#include <track_oracle/scoring_framework/score_core.h>
#include <track_oracle/scoring_framework/track_synthesizer.h>

#include <testlib/testlib_test.h>


using std::ostringstream;

namespace { // anon

using namespace vidtk;

void
test_track_length( track_handle_type t, size_t len )
{
  ostringstream oss;
  size_t n_frames = track_oracle::get_n_frames( t );
  oss << "Track has " << n_frames << " frames; expecting " << len;
  TEST( oss.str().c_str(), n_frames, len );
}

void
test_clone_fields()
{
  // create some tracks
  track_synthesizer_params p( 10, 0, 1 ); // 10pixel boxes, no overlap, 1fps
  track_synthesizer ts( p );
  track_handle_list_type tracks;

  // make some tracks
  ts.make_tracks( "aaadd", tracks );
  {
    ostringstream oss;
    oss << "Synthesized " << tracks.size() << " tracks; expecting 2";
    TEST( oss.str().c_str(), tracks.size(), 2 );
  }
  if ( tracks.size() != 2 ) return;

  test_track_length( tracks[0], 5 );
  test_track_length( tracks[1], 2 );

  scorable_track_type trk;

  // create the dest track
  track_handle_type t = trk.create();
  TEST( "Created destination track", t.is_valid(), true );

  {
    // test that copying empty rows succeeds
    track_handle_type empty_dst = trk.create();
    TEST( "Copying empty row succeeds", track_oracle::clone_nonsystem_fields( t, empty_dst ), true );
  }

  // copy the frames from track 0
  frame_handle_list_type t0_frames = track_oracle::get_frames( tracks[0] );

  // verify that we can't clone from or to an invalid handle
  {
    frame_handle_type invalid_frame;
    TEST( "Cloning from an invalid handle fails",
          track_oracle::clone_nonsystem_fields( invalid_frame, t0_frames[0] ),
          false );
    TEST( "Cloning to an invalid handle fails",
          track_oracle::clone_nonsystem_fields( t0_frames[0], invalid_frame ),
          false );
  }

  for (size_t i=0; i<t0_frames.size(); ++i)
  {
    ostringstream oss;
    frame_handle_type f = trk( t ).create_frame();
    bool rc = track_oracle::clone_nonsystem_fields( t0_frames[i], f );
    oss << "Cloning frame " << i << " call succeeds";
    TEST( oss.str().c_str(), rc, true );
  }

  frame_handle_list_type clone_frames = track_oracle::get_frames( t );
  frame_handle_list_type post_clone_t0_frames = track_oracle::get_frames( tracks[0] );
  bool source_same_size = (t0_frames.size() == post_clone_t0_frames.size());
  TEST( "Post clone: source track has same number of frames", source_same_size, true );
  bool dest_same_size = (t0_frames.size() == clone_frames.size());
  TEST( "Post clone: dest track has same number of frames", dest_same_size, true );
  if ( ! dest_same_size ) return;
  for (size_t i=0; i<t0_frames.size(); ++i)
  {
    {
      ostringstream oss;
      oss << "Post clone: source track frame " << i << " has the same handle";
      TEST( oss.str().c_str(), t0_frames[i], post_clone_t0_frames[i] );
    }
    {
      ostringstream oss;
      oss << "Post clone: source track frame " << i << " distinct from dest";
      TEST( oss.str().c_str(), (t0_frames[i] != clone_frames[i]), true );
    }
    {
      vgl_box_2d<double> src_box = trk[ t0_frames[i] ].bounding_box();
      vgl_box_2d<double> dst_box = trk[ clone_frames[i] ].bounding_box();
      ostringstream oss;
      oss << "Post clone: frame " << i << " has same bounding box";
      TEST( oss.str().c_str(), src_box, dst_box );
    }
  }
}


} // anon

int test_clone_fields( int, char *[] )
{
  testlib_test_start( "test_clone_fields" );
  test_clone_fields();

  return testlib_test_summary();
}
