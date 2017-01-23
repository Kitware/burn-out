/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

#include <track_oracle/scoring_framework/score_core.h>
#include <track_oracle/scoring_framework/score_phase1.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_min_gt_ct_pcent__
VIDTK_LOGGER("test_min_gt_ct_pcent");

#include <testlib/testlib_test.h>


using std::make_pair;
using std::map;
using std::ostringstream;
using std::pair;
using std::runtime_error;
using std::string;
using std::vector;

namespace {  /* anonymous */

typedef map< pair< double, double >, string > test_map_type;
typedef map< pair< double, double >, string >::const_iterator test_map_cit;

using vidtk::track_handle_type;

track_handle_type
make_singleframe_track( double xmin, double ymin, double width, double height )
{
  vidtk::scorable_track_type trk;

  // Single-frame tracks are enough for this.
  track_handle_type h = trk.create();
  vidtk::frame_handle_type f = trk.create_frame();

  trk.external_id() = 0;
  trk[f].timestamp_frame() = 0;
  trk[f].timestamp_usecs() = 0;
  trk[f].bounding_box() = vgl_box_2d<double>( xmin, xmin+width, ymin, ymin+height );
  return h;
}

void
run_gt_ct_pcent_tests()
{
  vidtk::scorable_track_type trk;

  // The unchanging truth track, a 10x10 box:
  track_handle_type truth_track = make_singleframe_track( 0, 0, 10, 10 );

  // and some computed tracks:
  vidtk::track_handle_list_type computed_tracks;

  // ...one identical to the truth track:
  computed_tracks.push_back( make_singleframe_track( 0, 0, 10, 10 ) );

  // ...one with 50-50 overlap
  computed_tracks.push_back( make_singleframe_track( 0, 5, 10, 10 ) );

  // ...one with 100-1 overlap
  computed_tracks.push_back( make_singleframe_track( 0, 0, 100, 100 ) );

  // ...one with no overlap
  computed_tracks.push_back( make_singleframe_track( 100, 0, 10, 10) );


  // For each setting of the min_pcent_gt_ct parameter, enumerate whether
  // or not we expect an overlap, e.g. -1:-1 -> "YYYn" means the default
  // (single-pixel) overlap settings should result in overlaps on all
  // but the last track.

  map< pair<double, double >, string > tests;
  // symmetric cases
  tests[ make_pair( -1.0, -1.0 ) ] = "YYYn";
  tests[ make_pair(    0,    0 ) ] = "YYYn"; // last n: we never return 0 area overlaps
  tests[ make_pair(  100,  100 ) ] = "Ynnn";
  tests[ make_pair(  101,  101 ) ] = "nnnn";
  tests[ make_pair(   51,   51 ) ] = "Ynnn";
  tests[ make_pair(   49,   49 ) ] = "YYnn";

  // asymmetric cases
  tests[ make_pair(   30,   80 ) ] = "Ynnn";
  tests[ make_pair(   80,   30 ) ] = "Ynnn";

  // don't care cases
  tests[ make_pair(   80,    0 ) ] = "YnYn";
  tests[ make_pair(    0,   30 ) ] = "YYnn";
  tests[ make_pair(    0,  0.5 ) ] = "YYYn";

  for (test_map_cit i = tests.begin(); i != tests.end(); ++i)
  {
    // unpack the string
    vector< bool > passes;
    for (size_t j=0; j<i->second.size(); ++j)
    {
      passes.push_back( i->second[j] == 'Y' );
    }
    if (passes.size() != computed_tracks.size())
    {
      ostringstream oss;
      oss << "String '" << i->second << " has " << passes.size()
          << " results, but only " << computed_tracks.size() << " test tracks are defined";
      throw runtime_error( oss.str().c_str() );
    }

    for (size_t j=0; j<computed_tracks.size(); ++j)
    {
      string tag;
      {
        ostringstream oss;
        oss << "[ " << i->first.first << ":" << i->first.second << " ]::" << i->second
            << "@" << j << "(" << i->second[j] << "): ";
        tag = oss.str();
      }

      // set up the experiment
      vidtk::track_handle_list_type raw_gt, raw_ct;
      raw_gt.push_back( truth_track );
      raw_ct.push_back( computed_tracks[j] );

      vidtk::phase1_parameters p1_params;
      p1_params.min_pcent_overlap_gt_ct = i->first;
      p1_params.debug_min_pcent_overlap_gt_ct = true;

      vidtk::track_handle_list_type gt, ct;
      p1_params.filter_track_list_on_aoi( raw_gt, gt );
      p1_params.filter_track_list_on_aoi( raw_ct, ct );

      // run the experiment
      vidtk::track2track_phase1 p1( p1_params );
      p1.compute_all( gt, ct );

      size_t n_overlaps = p1.t2t.size();
      if (n_overlaps == 1)
      {
        const vidtk::track2track_frame_overlap_record& s = p1.t2t.begin()->second.frame_overlaps[0];
        LOG_DEBUG( "Overlap: areas (truth, computed, overlap): "
                    << s.truth_area << ", " << s.computed_area << ", " << s.overlap_area );
      }

      // whether or not we expect an overlap depends on the pass status
      bool overlap_expected = passes[j];
      size_t n_overlaps_expected = (overlap_expected) ? 1 : 0;
      {
        ostringstream oss;
        oss << tag << "expected " << n_overlaps_expected << " frame overlap(s); found " << n_overlaps;
        TEST( oss.str().c_str(), n_overlaps, n_overlaps_expected );
      }
      bool expected_and_got_one_overlap =
        (n_overlaps == n_overlaps_expected) && (n_overlaps_expected == 1);
      if ( ! expected_and_got_one_overlap ) continue;

      const vidtk::track2track_score& s = p1.t2t.begin()->second;
      // did it return any overlap?
      double area = s.frame_overlaps[0].overlap_area;
      bool overlap_detected = (area >= 0);
      {
        ostringstream oss;
        oss << tag << "expected overlap? " << overlap_expected << " Detected? " << overlap_detected
            << " (area == " << area << ")";
        TEST( oss.str().c_str(), overlap_detected, overlap_expected );
      }
    } // for all the tracks in this min_gt_ct key
  } // for all min_gt_ct keys

}

} // anon

int test_min_gt_ct_pcent( int, char *[] )
{
  testlib_test_start( "test_min_gt_ct_pcent" );

  run_gt_ct_pcent_tests();

  return testlib_test_summary();
}
