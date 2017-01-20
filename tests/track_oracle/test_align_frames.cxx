/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>
#include <iomanip>
#include <cstdlib>

#include <track_oracle/track_oracle.h>
#include <track_oracle/track_base.h>
#include <track_oracle/track_field.h>
#include <track_oracle/track_kw18/track_kw18.h>
#include <track_oracle/track_kw18/file_format_kw18.h>

#include <track_oracle/scoring_framework/score_phase1.h>

#include <testlib/testlib_test.h>


using std::cerr;
using std::exit;
using std::ostringstream;
using std::pair;
using std::string;
using std::vector;

namespace {  /* anonymous */

void test_basic_alignment( const string& data_dir )
{
  vidtk::track_handle_list_type computed_tracks, truth_tracks;
  string gt_fn = data_dir+"/score_tracks_data/GTs/AlignTestGT.kw18";
  vidtk::file_format_kw18 kw18_reader;
  if ( ! kw18_reader.read( gt_fn, truth_tracks ))
  {
    cerr << "Couldn't load truth tracks from '" << gt_fn << "'\n";
    exit(EXIT_FAILURE);
  }
  string test_fn = data_dir+"/score_tracks_data/CTs/AlignTestCT.kw18";
  if ( ! kw18_reader.read( test_fn, computed_tracks ))
  {
    cerr << "Couldn't load computed tracks from '" << test_fn << "'\n";
    exit(EXIT_FAILURE);
  }

  TEST("Ground truth has 1 track", truth_tracks.size() == 1, true);
  TEST("Computed track has 1 track", computed_tracks.size() == 1, true);

  vidtk::frame_handle_list_type t_sorted_frames = sort_frames_by_field( truth_tracks[0], "timestamp_usecs" );
  vidtk::frame_handle_list_type c_sorted_frames = sort_frames_by_field( computed_tracks[0], "timestamp_usecs" );

  TEST("Ground truth has 11 Frames", t_sorted_frames.size() == 11, true);
  TEST("Computed track has 13 Frames", c_sorted_frames.size() == 13, true);

  // check that all frames are all marked IN_AOI_UNMATCHED
  vidtk::scorable_track_type trk;
  unsigned sorted_t_count = 0, sorted_c_count = 0;
  for (unsigned i=0; i<t_sorted_frames.size(); ++i)
  {
    if (trk[ t_sorted_frames[i] ].frame_has_been_matched() == vidtk::IN_AOI_UNMATCHED)
    {
      ++sorted_t_count;
    }
  }
  for (unsigned i=0; i<c_sorted_frames.size(); ++i)
  {
    if (trk[ c_sorted_frames[i] ].frame_has_been_matched() == vidtk::IN_AOI_UNMATCHED)
    {
      ++sorted_c_count;
    }
  }

  {
    ostringstream oss;
    oss << "Ground truth frames all IN_AOI_UNMATCHED; expecting " << t_sorted_frames.size() << "; found " << sorted_t_count;
    TEST( oss.str().c_str(), t_sorted_frames.size() == sorted_t_count, true );
  }

  {
    ostringstream oss;
    oss << "Computed frames all IN_AOI_UNMATCHED; expecting " << c_sorted_frames.size() << "; found " << sorted_c_count;
    TEST( oss.str().c_str(), c_sorted_frames.size() == sorted_c_count, true );
  }

  //t2t score will not be valid as p1 is not computing it
  //we're only using it to test the align frames function
  vidtk::track2track_score t2t_score;
  const double fps = (1.0/30) * 1000 * 1000;
  vector< pair< vidtk::frame_handle_type, vidtk::frame_handle_type > > aligned_frames =
    t2t_score.align_frames(t_sorted_frames, c_sorted_frames, fps);
  {
    ostringstream oss;
    oss << "Expecting 7 aligned frames; found " << aligned_frames.size();
    TEST( oss.str().c_str(), aligned_frames.size() == 7, true);
  }
}

void test_10Hz_vs_30Hz_alignment()
{
  vidtk::scorable_track_type trk;

  // set up a track with frames every 10Hz
  const unsigned long long fps_10Hz = static_cast<unsigned long long>( 1.0 / 10.0 * 1.0e6 );
  vidtk::track_handle_type track_10Hz = trk.create();
  for (unsigned i=0; i<10; ++i)
  {
    vidtk::frame_handle_type f = trk.create_frame();
    trk[f].timestamp_usecs() = i * fps_10Hz;
    cerr << "10Hz " << i << ": " << trk[f].timestamp_usecs() << "\n";
  }

  // set up a track with frames every 30Hz
  const unsigned long long fps_30Hz = static_cast<unsigned long long>( 1.0 / 30.0 * 1.0e6 );
  vidtk::track_handle_type track_30Hz = trk.create();
  for (unsigned i=0; i<30; ++i)
  {
    vidtk::frame_handle_type f = trk.create_frame();
    trk[f].timestamp_usecs() = i * fps_30Hz;
    cerr << "30Hz " << i << ": " << trk[f].timestamp_usecs() << "\n";
  }

  // the 10Hz frames should line up
  vidtk::frame_handle_list_type sorted_frames_10Hz = sort_frames_by_field( track_10Hz, "timestamp_usecs" );
  vidtk::frame_handle_list_type sorted_frames_30Hz = sort_frames_by_field( track_30Hz, "timestamp_usecs" );
  vidtk::track2track_score t2t_score;
  const double fps = (1.0/30) * 1000 * 1000;
  vector< pair< vidtk::frame_handle_type, vidtk::frame_handle_type > > aligned_frames =
    t2t_score.align_frames(sorted_frames_10Hz, sorted_frames_30Hz, fps);
  {
    ostringstream oss;
    oss << "Expecting 10 aligned frames at 10Hz; found " << aligned_frames.size();
    TEST( oss.str().c_str(), aligned_frames.size(), 10 );
  }
  // lineup should be symmetric
  vector< pair< vidtk::frame_handle_type, vidtk::frame_handle_type > > aligned_frames_2 =
    t2t_score.align_frames(sorted_frames_30Hz, sorted_frames_10Hz, fps);
  bool aligned_sets_same_size = (aligned_frames.size() == aligned_frames_2.size());
  TEST( "10Hz->30Hz same size as 30Hz->10Hz", aligned_sets_same_size, true );
  if (aligned_sets_same_size)
  {
    for (unsigned i=0; i<aligned_frames.size(); ++i)
    {
      bool same = ((aligned_frames[i].first == aligned_frames_2[i].second) &&
                   (aligned_frames[i].second == aligned_frames_2[i].first));
      ostringstream oss;
      oss << "10Hz->30Hz same as 30Hz->10Hz: frame " << i;
      TEST( oss.str().c_str(), same, true );
    }
  }
}

} // anon namespace

int test_align_frames( int argc, char *argv[] )
{
  if (argc < 2)
  {
    cerr << "Need the data directory as argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "test_align_frames" );

  test_basic_alignment( argv[1] );
  test_10Hz_vs_30Hz_alignment();

  return testlib_test_summary();
}

