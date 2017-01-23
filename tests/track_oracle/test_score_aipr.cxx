/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>
#include <iomanip>

#include <track_oracle/track_oracle.h>
#include <track_oracle/track_base.h>
#include <track_oracle/track_field.h>
#include <track_oracle/track_kw18/track_kw18.h>
#include <track_oracle/track_kw18/file_format_kw18.h>

#include <track_oracle/scoring_framework/score_tracks_aipr.h>

#include <testlib/testlib_test.h>


using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;
using std::setprecision;
using std::string;

namespace {  /* anonymous */

  using namespace vidtk;

  overall_phase3_aipr dare_to_compare(const char* gt,const char* ct)
  {
    vidtk::track_handle_list_type computed_tracks, truth_tracks;
    vidtk::file_format_kw18 kw18_reader;
    if ( ! kw18_reader.read(gt, truth_tracks ))
    {
      cerr << "Couldn't load truth tracks from '" << gt << "'\n";
      exit(EXIT_FAILURE);
    }
    cerr << "Loaded " << truth_tracks.size() << " tracks from '" << gt << "'\n";
    if ( ! kw18_reader.read(ct, computed_tracks ))
    {
      cerr << "Couldn't load computed tracks from '" << ct<< "'\n";
      exit(EXIT_FAILURE);
    }
    cerr << "Loaded " << computed_tracks.size() << " from '" << ct << "'\n";

    track2track_phase1 p1;
    p1.params.perform_sanity_checks = false;
    p1.compute_all( truth_tracks, computed_tracks );

    track2track_phase2_aipr p2;
    p2.compute( truth_tracks, computed_tracks, p1 );

    overall_phase3_aipr p3;
    p3.compute( p2 );

    return p3;
  }

  void test_tcf_factor( const string& dir )
  {
    vidtk::file_format_kw18 kw18_reader;
    vidtk::track_handle_list_type track_4_states, track_1_state, track_2_states;
    string tcf_4st_file( dir + "/track_oracle_kw18_track_tcf_4states.txt" );
    string tcf_1st_file( dir + "/track_oracle_kw18_track_tcf_1state.txt" );
    string tcf_2st_file( dir + "/track_oracle_kw18_track_tcf_2states.txt" );
    kw18_reader.read( tcf_4st_file, track_4_states );
    kw18_reader.read( tcf_1st_file, track_1_state );
    kw18_reader.read( tcf_2st_file, track_2_states );
    TEST( "Loaded one 4 state track", (track_4_states.size() == 1), true );
    TEST( "Loaded one 1 state track", (track_1_state.size() == 1), true );
    TEST( "Loaded one 2 state track", (track_2_states.size() == 1), true );

    // TCF is, basically, how much of the ground truth track is overlapped by any
    // computed track.

    // GT = 4 states, computed = 1 state; TCF = 0.25.
    // GT = 1 state, CT = 4 states; TCF = 1.0;

    // Note that when GT has 4 states, and computed has 2 tracks: 1st and 2st,
    // and the one state in 1st is ALSO in 2st, TCF should be 0.5 (number of UNIQUE
    // overlapping states) rather than 0.75 (total number of overlapping states)

    {
      track2track_phase1 p1;
      p1.params.perform_sanity_checks = false;
      p1.compute_all( track_4_states, track_1_state );

      track2track_phase2_aipr p2;
      p2.compute( track_4_states, track_1_state, p1 );

      overall_phase3_aipr p3;
      p3.compute( p2 );

      ostringstream oss;
      oss << "TCF GT(4) -> C(1): computed " << p3.track_completeness_factor << " ; expected 0.25";
      TEST( oss.str().c_str(), p3.track_completeness_factor, 0.25 );
    }

    {
      track2track_phase1 p1;
      p1.params.perform_sanity_checks = false;
      p1.compute_all( track_4_states, track_2_states );

      track2track_phase2_aipr p2;
      p2.compute( track_4_states, track_2_states, p1 );

      overall_phase3_aipr p3;
      p3.compute( p2 );

      ostringstream oss;
      oss << "TCF GT(4) -> C(2): computed " << p3.track_completeness_factor << " ; expected 0.5";
      TEST( oss.str().c_str(), p3.track_completeness_factor, 0.5 );
    }

    {
      track2track_phase1 p1;
      p1.params.perform_sanity_checks = false;
      p1.compute_all( track_1_state, track_4_states );

      track2track_phase2_aipr p2;
      p2.compute( track_1_state, track_4_states, p1 );

      overall_phase3_aipr p3;
      p3.compute( p2 );

      ostringstream oss;
      oss << "TCF GT(1) -> C(4): computed " << p3.track_completeness_factor << " ; expected 1.0";
      TEST( oss.str().c_str(), p3.track_completeness_factor, 1.0 );
    }

    {
      vidtk::track_handle_list_type two_tracks;
      two_tracks.push_back( track_2_states[0] );
      two_tracks.push_back( track_1_state[0] );
      track2track_phase1 p1;
      p1.params.perform_sanity_checks = false;
      p1.compute_all( track_4_states, two_tracks );

      track2track_phase2_aipr p2;
      p2.compute( track_4_states, two_tracks, p1 );

      overall_phase3_aipr p3;
      p3.compute( p2 );

      ostringstream oss;
      oss << "TCF GT(4) -> two tracks: computed " << p3.track_completeness_factor << " ; expected 0.5";
      TEST( oss.str().c_str(), p3.track_completeness_factor, 0.5 );
    }

  }
}

int test_score_aipr( int argc, char *argv[] )
{
  if (argc < 2)
  {
    cerr << "Need the data directory as argument\n";
    return EXIT_FAILURE;
  }

  const char *ground_truth_files[10] =
  {
  "GroundTruth3BowieHeads1Star",
  "GroundTruth1Star",
  "GroundTruth3BowieHeads",
  "GroundTruthBlueGreenBowieHead",
  "GroundTruthRedGreenBowieHeads",
  "GroundTruthRedBlueBowieHead",
  "GroundTruthGreenBowieHead",
  "GroundTruthBlueBowieHead",
  "GroundTruthRedBowieHead",
  "GroundTruth0BowieHeads"
  };

  const char *computed_trk_files[10] =
  {
  "CompPerfect3BowieHeads1Star",
  "CompSplitTrackFalsePosAndFragmented",
  "CompRobustAIPRAssociationTest",
  "CompFragmentedAndSwitch",
  "Comp1TrackInStar",
  "CompPerfectGreenBowieHead",
  "Comp1TrackSwitchFromRedToBlue",
  "Comp1TrackFragmentedOverGreen",
  "Comp1TrackOnGreenFrame4to10",
  "Comp0Tracks",
  };

  double expected_values[100][5]={
    //CompPerfect3BowieHeads1Star
    {0, 0, 1, 1, 1} , //GroundTruth3BowieHeads1Star
    {0, 0, 1, 1, 1} , //GroundTruth1Star
    {0, 0, 1, 1, 1} , //GroundTruth3BowieHeads
    {0, 0, 1, 1, 1} , //GroundTruthBlueGreenBowieHead
    {0, 0, 1, 1, 1} , //GroundTruthRedGreenBowieHeads
    {0, 0, 1, 1, 1} , //GroundTruthRedBlueBowieHead
    {0, 0, 1, 1, 1} , //GroundTruthGreenBowieHead
    {0, 0, 1, 1, 1} , //GroundTruthBlueBowieHead
    {0, 0, 1, 1, 1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //CompSplitTrackFalsePosAndFragmented
    {0.40000000000000002, 2, 0.5730337078651685, 1.25, 1.2808988764044944} , //GroundTruth3BowieHeads1Star
    {0, 0, 0.38461538461538464, 1, 1} , //GroundTruth1Star
    {0.20000000000000001, 1, 0.80952380952380953, 1.3333333333333333, 1.3968253968253967} , //GroundTruth3BowieHeads
    {0.20000000000000001, 1, 0.77777777777777779, 1.5, 1.5555555555555556} , //GroundTruthBlueGreenBowieHead
    {0, 0, 1.1860465116279071, 1.5, 1.5813953488372092} , //GroundTruthRedGreenBowieHeads
    {0, 0, 1.0789473684210527, 1, 1} , //GroundTruthRedBlueBowieHead
    {0, 0, 1.3999999999999999, 2, 2} , //GroundTruthGreenBowieHead
    {0, 0, 1.25, 1, 1} , //GroundTruthBlueBowieHead
    {0, 0, 0.88888888888888884, 1, 1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //CompRobustAIPRAssociationTest
    {0.16666666666666666, 1, 0.7078651685393258, 1.25, 1.2808988764044944} , //GroundTruth3BowieHeads1Star
    {0, 0, 0.80769230769230771, 1, 1} , //GroundTruth1Star
    {0, 0, 1, 1.3333333333333333, 1.3968253968253967} , //GroundTruth3BowieHeads
    {0, 0, 1, 1.5, 1.5555555555555556} , //GroundTruthBlueGreenBowieHead
    {0, 0, 1.069767441860465, 1.5, 1.5813953488372092} , //GroundTruthRedGreenBowieHeads
    {0, 0, 0.92105263157894735, 1, 1} , //GroundTruthRedBlueBowieHead
    {0, 0, 1.1200000000000001, 2, 2} , //GroundTruthGreenBowieHead
    {0, 0, 0.84999999999999998, 1, 1} , //GroundTruthBlueBowieHead
    {0, 0, 1, 1, 1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //CompFragmentedAndSwitch
    {0.33333333333333331, 2, 0.3146067415730337, 2.6666666666666665, 2.6338028169014085} , //GroundTruth3BowieHeads1Star
    {0, 0, 0.19230769230769232, 2, 2} , //GroundTruth1Star
    {0.16666666666666666, 1, 0.42857142857142855, 3, 3} , //GroundTruth3BowieHeads
    {0.16666666666666666, 1, 0.59999999999999998, 3, 3} , //GroundTruthBlueGreenBowieHead
    {0, 0, 0.44186046511627908, 3, 3} , //GroundTruthRedGreenBowieHeads
    {0, 0, 0.52631578947368418, 3, 3} , //GroundTruthRedBlueBowieHead
    {0, 0, 0.76000000000000001, 3, 3} , //GroundTruthGreenBowieHead
    {0, 0, 1, 3, 3} , //GroundTruthBlueBowieHead
    {0, 0, 0, -1, -1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //Comp1TrackInStar
    {1, 1, 0.29213483146067415, 1, 1} , //GroundTruth3BowieHeads1Star
    {0, 0, 1, 1, 1} , //GroundTruth1Star
    {1, 1, 0.41269841269841268, 1, 1} , //GroundTruth3BowieHeads
    {1, 1, 0.57777777777777772, 1, 1} , //GroundTruthBlueGreenBowieHead
    {0, 0, 0.60465116279069764, 1, 1} , //GroundTruthRedGreenBowieHeads
    {0, 0, 0.68421052631578949, 1, 1} , //GroundTruthRedBlueBowieHead
    {0, 0, 1.04, 1, 1} , //GroundTruthGreenBowieHead
    {0, 0, 1.3, 1, 1} , //GroundTruthBlueBowieHead
    {0, 0, 0, -1, -1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //CompPerfectGreenBowieHead
    {1, 1, 0.2808988764044944, 1, 1} , //GroundTruth3BowieHeads1Star
    {0, 0, 0.96153846153846156, 1, 1} , //GroundTruth1Star
    {1, 1, 0.3968253968253968, 1, 1} , //GroundTruth3BowieHeads
    {1, 1, 0.55555555555555558, 1, 1} , //GroundTruthBlueGreenBowieHead
    {0, 0, 0.58139534883720934, 1, 1} , //GroundTruthRedGreenBowieHeads
    {0, 0, 0.65789473684210531, 1, 1} , //GroundTruthRedBlueBowieHead
    {0, 0, 1, 1, 1} , //GroundTruthGreenBowieHead
    {0, 0, 1.25, 1, 1} , //GroundTruthBlueBowieHead
    {0, 0, 0, -1, -1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //Comp1TrackSwitchFromRedToBlue
    {1, 1, 0.21348314606741572, 1, 1} , //GroundTruth3BowieHeads1Star
    {0, 0, 0.73076923076923073, 1, 1} , //GroundTruth1Star
    {1, 1, 0.30158730158730157, 1, 1} , //GroundTruth3BowieHeads
    {1, 1, 0.42222222222222222, 1, 1} , //GroundTruthBlueGreenBowieHead
    {1, 1, 0.44186046511627908, 1, 1} , //GroundTruthRedGreenBowieHeads
    {1, 1, 0.5, 1, 1} , //GroundTruthRedBlueBowieHead
    {0, 0, 0.76000000000000001, 1, 1} , //GroundTruthGreenBowieHead
    {0, 0, 0.94999999999999996, 1, 1} , //GroundTruthBlueBowieHead
    {0, 0, 1.0555555555555556, 1, 1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //Comp1TrackFragmentedOverGreen
    {0.5, 2, 0.19101123595505617, 2.3333333333333335, 2.4225352112676055} , //GroundTruth3BowieHeads1Star
    {0, 0, 0.46153846153846156, 2, 2} , //GroundTruth1Star
    {0.25, 1, 0.26984126984126983, 2.5, 2.6666666666666665} , //GroundTruth3BowieHeads
    {0.25, 1, 0.37777777777777777, 2.5, 2.6666666666666665} , //GroundTruthBlueGreenBowieHead
    {0, 0, 0.39534883720930231, 4, 4} , //GroundTruthRedGreenBowieHeads
    {0, 0, 0.18421052631578946, 1, 1} , //GroundTruthRedBlueBowieHead
    {0, 0, 0.68000000000000005, 4, 4} , //GroundTruthGreenBowieHead
    {0, 0, 0.34999999999999998, 1, 1} , //GroundTruthBlueBowieHead
    {0, 0, 0, -1, -1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //Comp1TrackOnGreenFrame4to10
    {1, 1, 0.078651685393258425, 1, 1} , //GroundTruth3BowieHeads1Star
    {0, 0, 0.26923076923076922, 1, 1} , //GroundTruth1Star
    {0, 0, 0.1111111111111111, 1, 1} , //GroundTruth3BowieHeads
    {0, 0, 0.15555555555555556, 1, 1} , //GroundTruthBlueGreenBowieHead
    {0, 0, 0.16279069767441862, 1, 1} , //GroundTruthRedGreenBowieHeads
    {0, 0, 0, -1, -1} , //GroundTruthRedBlueBowieHead
    {0, 0, 0.28000000000000003, 1, 1} , //GroundTruthGreenBowieHead
    {0, 0, 0, -1, -1} , //GroundTruthBlueBowieHead
    {0, 0, 0, -1, -1} , //GroundTruthRedBowieHead
    {0, 0, -1, -1, -1} , //GroundTruth0BowieHeads

    //Comp0Tracks
    {-1, 0, 0, -1, -1} , //GroundTruth3BowieHeads1Star
    {-1, 0, 0, -1, -1} , //GroundTruth1Star
    {-1, 0, 0, -1, -1} , //GroundTruth3BowieHeads
    {-1, 0, 0, -1, -1} , //GroundTruthBlueGreenBowieHead
    {-1, 0, 0, -1, -1} , //GroundTruthRedGreenBowieHeads
    {-1, 0, 0, -1, -1} , //GroundTruthRedBlueBowieHead
    {-1, 0, 0, -1, -1} , //GroundTruthGreenBowieHead
    {-1, 0, 0, -1, -1} , //GroundTruthBlueBowieHead
    {-1, 0, 0, -1, -1} , //GroundTruthRedBowieHead
    {-1, 0, -1, -1, -1}  //GroundTruth0BowieHeads
  };

  testlib_test_start( "test_score_aipr" );
  //string str("");
  //std::ostringstream oss;
  for(int i = 0; i < 10 ; i++)
  {
    //oss <<
    //    "//" << string(computed_trk_files[i]) << "\n" ;
    for(int j = 0; j < 10; j++)
    {
      overall_phase3_aipr p3 = dare_to_compare(
        (string(argv[1])+"/score_tracks_data/GTs/"+string(ground_truth_files[j])+".kw18").c_str(),
        (string(argv[1])+"/score_tracks_data/CTs/"+string(computed_trk_files[i])+".kw18").c_str());

      TEST( "Identity Switch:", static_cast<int>(p3.identity_switch * 10) == static_cast<int>(expected_values[(i*10)+j][0] * 10 ), true );
      TEST( "Identity Switch: ", static_cast<int>(p3.num_identity_switch * 10) == static_cast<int>(expected_values[(i*10)+j][1] * 10), true );
      // removed next test since (a) the values would need to be recomputed due to the TCF fix and
      // (b) the test was never semantically validated anyway, since there was a huge bug in TCF all
      // the time the tests were passing
      //       TEST( "Track Completeness Factor: ", static_cast<int>(p3.track_completeness_factor * 10) == static_cast<int>(expected_values[(i*10)+j][2] * 10), true );
      TEST( "Track Fragmentation: ", static_cast<int>(p3.track_fragmentation * 10) == static_cast<int>(expected_values[(i*10)+j][3] * 10), true );
      TEST( "Normalized Track Fragmentation: ", static_cast<int>(p3.normalized_track_fragmentation * 10) == static_cast<int>(expected_values[(i*10)+j][4] * 10), true );

/*
      oss << "{" <<
        setprecision(18) <<
        p3.identity_switch << ", " <<
        p3.num_identity_switch << ", " <<
        p3.track_completeness_factor << ", " <<
        p3.track_fragmentation << ", " <<
        p3.normalized_track_fragmentation << "} , //" << string(ground_truth_files[j]) << endl;

      cout << " " << expected_values[(i*10)+j][0]<< ", "
        << expected_values[(i*10)+j][1]<< ", "
        << expected_values[(i*10)+j][2]<< ", "
        << expected_values[(i*10)+j][3]<< ", "
        << expected_values[(i*10)+j][4]<< endl;
      cout << "+===============================================\n";*/
    }
  }
  //cout << oss.str();

  test_tcf_factor( argv[1] );

  return testlib_test_summary();
}

