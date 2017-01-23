/*ckwg +5
 * Copyright 2010-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <iostream>
#include <string>
#include <sstream>

#include <track_oracle/track_oracle.h>
#include <track_oracle/track_base.h>
#include <track_oracle/track_field.h>
#include <track_oracle/track_kw18/track_kw18.h>
#include <track_oracle/track_kw18/file_format_kw18.h>

#include <track_oracle/scoring_framework/score_tracks_hadwav.h>
#include <track_oracle/scoring_framework/phase1_parameters.h>

#include <testlib/testlib_test.h>


using std::cerr;
using std::cout;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::string;

namespace {  /* anonymous */

  using namespace vidtk;

  void
    test_aoi_track_to_self( const string & dir )
  {
    vidtk::file_format_kw18 kw18_reader;
    vidtk::track_handle_list_type computed_tracks, truth_tracks;
    string single_track_file( dir + "/track_oracle_kw18_track_aoi_tracks.txt" );
    kw18_reader.read( single_track_file.c_str(), truth_tracks );
    kw18_reader.read( single_track_file.c_str(), computed_tracks );
    TEST( "Loaded one gt track from track_a", (truth_tracks.size() == 559), true );
    TEST( "Loaded one computed track from track_a", (computed_tracks.size() == 559), true );
    vidtk::phase1_parameters p1_params;
    p1_params.setAOI( "500x500+100+100" );
    track_handle_list_type aoi_filtered_truth_tracks, aoi_filtered_computed_tracks;
    p1_params.filter_track_list_on_aoi( truth_tracks, aoi_filtered_truth_tracks );
    p1_params.filter_track_list_on_aoi( computed_tracks, aoi_filtered_computed_tracks );
    cout << aoi_filtered_truth_tracks.size() << " " << aoi_filtered_computed_tracks.size() << endl;
    TEST("AOI Filtered correct number of tracks", aoi_filtered_truth_tracks.size(), 56 );
    TEST("AOI Filtered correct number of tracks", aoi_filtered_computed_tracks.size(), 56 );

    vidtk::track2track_phase1 p1(p1_params);
    p1.compute_all( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks );

    vidtk::track2track_phase2_hadwav p2;
    p2.compute( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks, p1 );

    vidtk::overall_phase3_hadwav p3;
    p3.compute( p2 );

    {
      ostringstream oss;
      oss << "Detection Pd of " << p2.detectionPD << " is 1: ";
      TEST( oss.str().c_str(), (p2.detectionPD == 1), true );
    }
    {
      ostringstream oss;
      oss << "Detection FA of " << p2.detectionFalseAlarms << " is 0: ";
      TEST( oss.str().c_str(), (p2.detectionFalseAlarms == 0), true );
    }

    TEST( "Track purity == 1", (p3.avg_track_continuity == 1), true );
    TEST( "Target purity == 1", (p3.avg_target_continuity == 1), true );
  }

  void
    test_score_track_to_self( const string& dir )
  {
    cout << "Testing self-to-self scoring...\n";

    vidtk::track_handle_list_type computed_tracks, truth_tracks;
    string single_track_file( dir + "/track_oracle_kw18_track_a.txt" );
    vidtk::file_format_kw18 kw18_reader;
    kw18_reader.read( single_track_file.c_str(), truth_tracks );
    kw18_reader.read( single_track_file.c_str(), computed_tracks );
    TEST( "Loaded one gt track from track_a", (truth_tracks.size() == 1), true );
    TEST( "Loaded one computed track from track_a", (computed_tracks.size() == 1), true );

    vidtk::phase1_parameters p1_params;
    track_handle_list_type aoi_filtered_truth_tracks, aoi_filtered_computed_tracks;
    p1_params.filter_track_list_on_aoi( truth_tracks, aoi_filtered_truth_tracks );
    p1_params.filter_track_list_on_aoi( computed_tracks, aoi_filtered_computed_tracks );
    TEST("AOI Filtered correct number of tracks", aoi_filtered_truth_tracks.size(), 1 );
    TEST("AOI Filtered correct number of tracks", aoi_filtered_computed_tracks.size(), 1 );

    vidtk::track2track_phase1 p1(p1_params);
    p1.compute_all( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks );

    vidtk::track2track_phase2_hadwav p2;
    p2.compute( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks, p1 );

    vidtk::overall_phase3_hadwav p3;
    p3.compute( p2 );

    {
      ostringstream oss;
      oss << "phase1 t2t size " << p1.t2t.size() << " is 1: ";
      TEST( oss.str().c_str(), (p1.t2t.size() == 1), true );
    }

    {
      ostringstream oss;
      oss << "Detection Pd of " << p2.detectionPD << " is 1: ";
      TEST( oss.str().c_str(), (p2.detectionPD == 1), true );
    }
    {
      ostringstream oss;
      oss << "Detection FA of " << p2.detectionFalseAlarms << " is 0: ";
      TEST( oss.str().c_str(), (p2.detectionFalseAlarms == 0), true );
    }

    {
      ostringstream oss;
      oss << "Pd of " << p3.trackPd << " is 1: ";
      TEST( oss.str().c_str(), (p3.trackPd == 1), true );
    }
    {
      ostringstream oss;
      oss << "FA of " << p3.trackFA << " is 0: ";
      TEST( oss.str().c_str(), (p3.trackFA == 0), true );
    }

    TEST( "Track continuity == 1", (p3.avg_track_continuity == 1), true );
    TEST( "Track purity == 1", (p3.avg_track_continuity == 1), true );
    TEST( "Target continuity == 1", (p3.avg_target_continuity == 1), true );
    TEST( "Target purity == 1", (p3.avg_target_continuity == 1), true );
  }

  void
    test_score_track_to_superset( const string& dir )
  {
    cout << "Testing gt(A) to c(A,B) scoring...\n";

    vidtk::track_handle_list_type computed_tracks, truth_tracks;
    string single_track_file( dir + "/track_oracle_kw18_track_a.txt" );
    string two_track_file( dir + "/track_oracle_kw18_track_ab.txt" );
    vidtk::file_format_kw18 kw18_reader;
    kw18_reader.read( single_track_file.c_str(), truth_tracks );
    kw18_reader.read( two_track_file.c_str(), computed_tracks );
    TEST( "Loaded one gt track from track_a", (truth_tracks.size() == 1), true );
    TEST( "Loaded two computed tracks from track_ab", (computed_tracks.size() == 2), true );

    vidtk::phase1_parameters p1_params;
    track_handle_list_type aoi_filtered_truth_tracks, aoi_filtered_computed_tracks;
    p1_params.filter_track_list_on_aoi( truth_tracks, aoi_filtered_truth_tracks );
    p1_params.filter_track_list_on_aoi( computed_tracks, aoi_filtered_computed_tracks );
    TEST("AOI Filtered correct number of tracks", aoi_filtered_truth_tracks.size(), 1 );
    TEST("AOI Filtered correct number of tracks", aoi_filtered_computed_tracks.size(), 2 );

    vidtk::track2track_phase1 p1(p1_params);
    p1.compute_all( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks );

    vidtk::track2track_phase2_hadwav p2;
    p2.compute( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks, p1 );

    vidtk::overall_phase3_hadwav p3;
    p3.compute( p2 );

    TEST( "Detection Pd == 1: ", (p2.detectionPD == 1), true );
    {
      ostringstream oss;
      oss << "Detection FA == 5 (is: " << p2.detectionFalseAlarms << ")";
      TEST( oss.str().c_str(), (p2.detectionFalseAlarms == 5), true );
    }

    TEST( "Pd == 1: ", (p3.trackPd == 1), true );
    {
      ostringstream oss;
      oss << "FA == 1 (is: " << p3.trackFA << ")";
      TEST( oss.str().c_str(), (p3.trackFA == 1), true );
    }
    {
      ostringstream oss;
      oss << "Track continuity is " << p3.avg_track_continuity << " ; should be 1: ";
      TEST( oss.str().c_str(), (p3.avg_track_continuity == 1), true );
    }
    {
      ostringstream oss;
      oss << "Track purity is " << p3.avg_track_purity << " ; should be 1: ";
      TEST( oss.str().c_str(), (p3.avg_track_purity == 1), true );
    }
    TEST( "Target continuity == 1", (p3.avg_target_continuity == 1), true );
    TEST( "Target purity == 1", (p3.avg_target_purity == 1), true );
  }

  void
    test_score_track_to_disjoint( const string& dir )
  {
    cout << "Testing gt(A) to c(B) scoring...\n";

    vidtk::track_handle_list_type computed_tracks, truth_tracks;
    string single_track_file( dir + "/track_oracle_kw18_track_a.txt" );
    string other_track_file( dir + "/track_oracle_kw18_track_b.txt" );
    vidtk::file_format_kw18 kw18_reader;
    kw18_reader.read( single_track_file.c_str(), truth_tracks );
    kw18_reader.read( other_track_file.c_str(), computed_tracks );
    TEST( "Loaded one gt track from track_a", (truth_tracks.size() == 1), true );
    TEST( "Loaded one computed track from track_b", (computed_tracks.size() == 1), true );

    vidtk::phase1_parameters p1_params;
    track_handle_list_type aoi_filtered_truth_tracks, aoi_filtered_computed_tracks;
    p1_params.filter_track_list_on_aoi( truth_tracks, aoi_filtered_truth_tracks );
    p1_params.filter_track_list_on_aoi( computed_tracks, aoi_filtered_computed_tracks );
    TEST("AOI Filtered correct number of tracks", aoi_filtered_truth_tracks.size(), 1 );
    TEST("AOI Filtered correct number of tracks", aoi_filtered_computed_tracks.size(), 1 );

    vidtk::track2track_phase1 p1(p1_params);
    p1.compute_all( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks );

    vidtk::track2track_phase2_hadwav p2;
    p2.compute( aoi_filtered_truth_tracks, aoi_filtered_computed_tracks, p1 );

    vidtk::overall_phase3_hadwav p3;
    p3.compute( p2 );

    TEST( "Detection Pd == 0: ", (p2.detectionPD == 0), true );
    TEST( "Detection FA == 5: ", (p2.detectionFalseAlarms == 5), true );
    TEST( "Pd == 0: ", (p3.trackPd == 0), true );
    TEST( "FA == 1: ", (p3.trackFA == 1), true );
    {
      ostringstream oss;
      oss << "Track continuity is " << p3.avg_track_continuity << " ; should be 0: ";
      TEST( oss.str().c_str(), (p3.avg_track_continuity == 0), true );
    }
    {
      ostringstream oss;
      oss << "Track purity is " << p3.avg_track_purity << " ; should be 0: ";
      TEST( oss.str().c_str(), (p3.avg_track_purity == 0), true );
    }
    TEST( "Target continuity == 0", (p3.avg_target_continuity == 0), true );
    TEST( "Target purity == 0", (p3.avg_target_purity == 0), true );
  }

  void
    test_bbox_expansion()
  {
    cout << "Testing bbox expansion...\n";
    // kw18 track with one bounding box
    const char* track_a_str = "0 1 1  15 15 0 0  15 15   10 10 20 20  100  15 15 0  314\n";

    // same as track_a, but bounding box shifted
    const char* track_b_str = "0 1 1  35 15 0 0  35 15   30 10 40 20  100  35 15 0  314\n";

    track_handle_list_type track_a, track_b;

    {
      istringstream iss_a( track_a_str );
      istringstream iss_b( track_b_str );

      vidtk::file_format_kw18 kw18_reader;
      TEST( "Read track A", (kw18_reader.read(iss_a, track_a ) && (track_a.size() == 1)), true);
      TEST( "Read track B", (kw18_reader.read(iss_b, track_b ) && (track_b.size() == 1)), true );
    }

    // verify that the tracks do not overlap
    {
      track2track_phase1 p1;
      p1.compute_all( track_a, track_b );
      track2track_phase2_hadwav p2;
      p2.compute( track_a, track_b, p1 );
      overall_phase3_hadwav p3;
      p3.compute( p2 );
      TEST( "Expansion == 0: Pd == 0: ", (p3.trackPd == 0), true );
      TEST( "Expansion == 0: FA == 1: ", (p3.trackFA == 1), true );
    }

    // expansion of 10-eps should still not overlap
    {
      phase1_parameters p1p( 9.99 );
      track2track_phase1 p1( p1p );
      p1.compute_all( track_a, track_b );
      track2track_phase2_hadwav p2;
      p2.compute( track_a, track_b, p1 );
      overall_phase3_hadwav p3;
      p3.compute( p2 );
      TEST( "Expansion == 9.99: Pd == 0: ", (p3.trackPd == 0), true );
      TEST( "Expansion == 9.99: FA == 1: ", (p3.trackFA == 1), true );
    }

    // expansion of 10+eps should have the boxes just touching
    {
      phase1_parameters p1p( 10.01 );
      track2track_phase1 p1( p1p );
      p1.compute_all( track_a, track_b );
      track2track_phase2_hadwav p2;
      p2.compute( track_a, track_b, p1 );
      overall_phase3_hadwav p3;
      p3.compute( p2 );
      TEST( "Expansion == 10.01: Pd == 1: ", (p3.trackPd == 1), true );
      TEST( "Expansion == 10.01: FA == 0: ", (p3.trackFA == 0), true );
    }

  }

void
test_latlon_bounding_box()
{
  {
    phase1_parameters p;
    string not_an_aoi("This is not an aoi");
    string pixel_aoi("5x2+100+200");
    string geo_aoi("-107,42.8:-105,41.9");

    TEST( "Setting AOI from non-AOI: ", p.setAOI( not_an_aoi ), false );
    TEST( "Setting AOI from pixel: ", p.setAOI( pixel_aoi ), true );
    TEST( "Setting AOI from geo: ", p.setAOI( geo_aoi ), true );

  }
}

}// anon namespace

int test_score_hadwav( int argc, char *argv[] )
{
  if (argc < 2)
  {
    cerr << "Need the data directory as argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "test_score_hadwav" );
  test_score_track_to_self( argv[1] );
  test_score_track_to_superset( argv[1] );
  test_score_track_to_disjoint( argv[1] );
  test_aoi_track_to_self( argv[1] );
  test_bbox_expansion();
  test_latlon_bounding_box();

  return testlib_test_summary();
}
