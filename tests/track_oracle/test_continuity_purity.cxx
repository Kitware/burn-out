/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <iostream>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>

#include <track_oracle/scoring_framework/track_synthesizer.h>

#include <track_oracle/scoring_framework/score_tracks_hadwav.h>

#include <testlib/testlib_test.h>

#include <logger/logger.h>


using std::istringstream;
using std::ostringstream;
using std::runtime_error;
using std::string;
using std::vector;


#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_continuity_purity__
VIDTK_LOGGER("test_continuity_purity");

namespace  // anon
{

struct scores_type
{
  double track_pd, track_fa;
  double detection_pd, detection_fa;
  double avg_track_continuity, avg_track_purity;
  double avg_target_continuity, avg_target_purity;
};

bool within_percentage( double a,
                        double b,
                        double percentage )
{
  double diff =
    (a > b)
    ? a-b
    : b-a;
  double upper_bound = percentage * a;
  return (diff <= upper_bound);
}

void test_value( const string& tag,
                 double a,
                 double b )
{
  const double percentage = 0.0005;  // 0.05%
  ostringstream oss;
  oss << tag << ": " << a << " vs " << b;
  TEST( oss.str().c_str(), within_percentage( a, b, percentage ), true );
}

void
verify_scores( const string& tag,
               const scores_type& s,
               const string& expected_scores )
{
  vector<double> e;
  istringstream iss( expected_scores );
  for (size_t i=0; i<8; ++i)
  {
    double tmp;
    if ( ! (iss >> tmp ))
    {
      ostringstream oss;
      oss << tag << " expected_score parse error index " << i
          << " string '" << expected_scores << "'";
      throw runtime_error( oss.str().c_str() );
    }
    e.push_back( tmp );
  }

  test_value( tag+" track_pd", e[0], s.track_pd );
  test_value( tag+" track_fa", e[1], s.track_fa );
  test_value( tag+" detection_pd", e[2], s.detection_pd );
  test_value( tag+" detection_fa", e[3], s.detection_fa );
  test_value( tag+" avg_track_cont", e[4], s.avg_track_continuity );
  test_value( tag+" avg_track_purity", e[5], s.avg_track_purity );
  test_value( tag+" avg_target_cont", e[6], s.avg_target_continuity );
  test_value( tag+" avg_target_purity", e[7], s.avg_target_purity );
}

scores_type
compute_stats( const vidtk::track_synthesizer_params& p,
               const string& gt_track_description,
               const string& c_track_description,
               bool verbose = false )
{

  vidtk::track_synthesizer ts( p );
  vidtk::track_handle_list_type raw_gt_tracks, raw_c_tracks;

  if ( ( ! ts.make_tracks( gt_track_description, raw_gt_tracks ))
       ||
       ( ! ts.make_tracks( c_track_description, raw_c_tracks )) )
  {
    ostringstream oss;
    oss << "Failed to make test tracks: '" << gt_track_description
        << "'; '" << c_track_description << "'";
    throw runtime_error( oss.str().c_str() );
  }
  if (verbose)
  {
    vidtk::scorable_track_type trk;
    LOG_INFO( raw_gt_tracks.size() << "raw gt_tracks" );
    for (size_t i=0; i<raw_gt_tracks.size(); ++i)
    {
      LOG_INFO( i << trk( raw_gt_tracks[i] ) );
    }
    LOG_INFO( raw_c_tracks.size() << " raw c_tracks" );
    for (size_t i=0; i<raw_c_tracks.size(); ++i)
    {
      LOG_INFO( i << trk( raw_c_tracks[i] ) );
    }
  }


  vidtk::phase1_parameters p1_params;

  // this step is non-optional; it sets the AOI status of the frames
  // (argh)
  vidtk::track_handle_list_type gt_tracks, c_tracks;
  p1_params.filter_track_list_on_aoi( raw_gt_tracks, gt_tracks );
  p1_params.filter_track_list_on_aoi( raw_c_tracks, c_tracks );

  vidtk::track2track_phase1 p1( p1_params );
  p1.compute_all( gt_tracks, c_tracks );

  vidtk::track2track_phase2_hadwav p2( verbose );
  p2.compute( gt_tracks, c_tracks, p1 );

  vidtk::overall_phase3_hadwav p3;
  p3.verbose = verbose;
  p3.compute( p2 );

  scores_type stats;
  stats.track_pd = p3.trackPd;
  stats.track_fa = p3.trackFA;
  stats.detection_pd = p2.detectionPD;
  stats.detection_fa = static_cast<double>( p2.detectionFalseAlarms );
  stats.avg_track_continuity = p3.avg_track_continuity;
  stats.avg_track_purity = p3.avg_track_purity;
  stats.avg_target_continuity = p3.avg_target_continuity;
  stats.avg_target_purity = p3.avg_target_purity;

  return stats;
}

void
test_sanity_stats()
{
  vidtk::track_synthesizer_params p( 10, 5, 10 );

  // no tracks at all should produce zeros
  {
    string gt_tracks = "";
    scores_type s = compute_stats( p, gt_tracks, gt_tracks );
    verify_scores( "no overlap", s, "0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0" );
  }
  // tracks but no overlap should throw
  {
    string gt_tracks = "a...";
    string c_tracks  = "...b";
    bool caught_exception = false;
    try
    {
      scores_type s = compute_stats( p, gt_tracks, c_tracks );
      verify_scores( "no overlap (throws)", s, "0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0" );
    }
    catch (runtime_error& e)
    {
      LOG_INFO("Caught exception: " << e.what() );
      caught_exception = true;
    }
    TEST( "No overlap throws exception", caught_exception, true );
  }
  // against itself, we should get straight unity
  {
    string gt_tracks = "a";
    scores_type s = compute_stats( p, gt_tracks, gt_tracks );
    verify_scores( "single frame identity", s, "1.0 0.0 1.0 0.0 1.0 1.0 1.0 1.0" );
  }
  {
    string gt_tracks = "aaaaabbbbbccccc";
    scores_type s = compute_stats( p, gt_tracks, gt_tracks );
    verify_scores( "multi track / multi frame identity", s, "1.0 0.0 1.0 0.0 1.0 1.0 1.0 1.0" );
  }

}

void
test_basic_stats()
{
  vidtk::track_synthesizer_params p( 10, 5, 10 );

  // interleaving tracks should produce zeros
  {
    string gt_tracks = "a.a.a.a";
    string  c_tracks = ".b.b.b.";
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    verify_scores( "interleave no-overlap", s, "0.0 1.0   0.0 3.0    0.0 0.0    0.0 0.0" );
  }
  // interleaving tracks with a single overlap
  {
    string gt_tracks = "a.a.a.a";
    string c_tracks  = ".b.b.bb";
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // pd = 1.0, fa = 0
    // detection pd = 0.25, fa = 3
    // track cont = 1, purity = 0.25
    // target cont = 1, purity = 0.25
    verify_scores( "interleave 1-overlap v1", s, "1.0 0.0   0.25 3.0   1.0 0.25  1.0 0.25" );
  }
  // as above, but strings reversed
  {
    string gt_tracks = "a.a.a.a";
    string c_tracks  = "bb.b.b.";
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    verify_scores( "interleave 1-overlap v2", s, "1.0 0.0   0.25 3.0   1.0 0.25  1.0 0.25" );
  }
  // interleaving tracks with multiple overlaps
  {
    string gt_tracks = "a.a.a.a.a.a.a.a.a.a";  // 10 frames
    string c_tracks  = ".b.b.bb..b.bb..bbb.";  // 10 frames w/ 3 overlaps
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // pd = 1.0, fa = 0
    // detection pd = 0.3, fa = 7
    // track cont = 1, purity = 0.3
    // target cont = 1, purity = 0.3
    verify_scores( "interleave multiple overlap v1", s, "1.0 0.0   0.3 7.0   1.0 0.3  1.0 0.3" );
  }
}

void
test_basic_multitrack_stats()
{
  vidtk::track_synthesizer_params p( 10, 5, 10 );

  // interleaving tracks with multiple overlaps
  {
    string gt_tracks = "a.a.a..c.c.c.c.c.c.c";  // TWO targets: 3 frames, 7 frames
    string c_tracks  = ".b.bb...b.b.bb..bbb.";  // 10 frames w/ 3 overlaps
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd should still be 1.0, fa = 0
    // detection pd should still be 0.3, fa = 7
    // track continuity: 2 (b is associated with a and b)
    // track purity: 0.2 (2/10 frames of b are associated with c, the dominant track)
    // target continuity: 1 (both a and c are associated with b)
    // target purity:
    // average of:
    // ...1/3 (for a, not 1/10, since we're clipping to the "life of the target")
    // ...2/7 (for c, not 2/10, since we're clipping to the "life of the target")
    double expected_target_purity = ( (1.0/3.0) + (2.0/7.0) ) / 2.0;
    ostringstream oss;
    oss << "1.0 0.0   0.3 7.0   2.0 0.2  1.0 " << expected_target_purity;
    verify_scores( "multitrack 1", s, oss.str() );
  }

  // interleaving tracks with multiple overlaps
  {
    string gt_tracks = "c.a.a.a....c.c.c.c.c.c";  // TWO targets: 3 frames, 7 frames
    string c_tracks  = ".b.b..b...b.b.bb..bbb.";  // 10 frames w/ 3 overlaps
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd should still be 1.0, fa = 0
    // detection pd should still be 0.3, fa = 7
    // track continuity: 2 (b is associated with a and b)
    // track purity: 0.2 (2/10 frames of b are associated with c, the dominant track)
    // target continuity: 1 (both a and c are associated with b)
    // target purity:
    // average of:
    // ...1/3  (for a, not 1/10, since we're clipping to the "life of the target")
    // ...2/7 still?  Roddy is confused.  Let's lay it out in painful detail, ignoring
    // ground truth track A:
    // GT: c..........c.c.c.c.c.c
    //                    |   |
    // C : .b.b..b...b.b.bb..bbb.
    //
    // Target purity: "Percentage of associations with the predominant track
    // utilizing the given target over the life of the target."
    //
    // We do NOT "clip to the life of the target."  Instead, the associations
    // are computed as what percent of GT (here, c) match "the predominant track
    // utilizing the given target over the life of the target."  Okay.  This is
    // still 2/7.
    double expected_target_purity = ( (1.0/3.0) + (2.0/7.0) ) / 2.0;
    ostringstream oss;
    oss << "1.0 0.0   0.3 7.0   2.0 0.2  1.0 " << expected_target_purity;
    verify_scores( "multitrack 2", s, oss.str() );
  }

  // interleaving tracks with multiple overlaps
  {
    string gt_tracks = "c.c.c.c.c.c.c.c.c.c";  // one target, 10 frames
    string c_tracks  = "a.a.a.a.b.b.b.b.b.b";  // two tracks; 4 and 6 frames
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd should still be 1.0, fa = 0
    // detection pd should be 1.0, fa = 0
    // track continuity: 1+1 / 2 == 1 (both a and b are associated with c)
    // track purity: 1 (both a and b completely match their mutually dominant target, c)
    // target continuity: 2 (a and b are initialized against c)
    // target purity: 0.6 (b is the predominant track.)

    ostringstream oss;
    oss << "1.0 0.0   1.0 0.0   1.0 1.0  2.0 0.6 ";
    verify_scores( "multitrack 3", s, oss.str() );
  }
}

void
test_naresh_scenario()
{
  vidtk::track_synthesizer_params p( 10, 5, 10 );

  // Naresh's scenario as drawn on Roddy's whiteboard:
  //
  //  t | GT-1  | GT-2  |  CT
  // ---+-------+-------+------
  //  1 |  x    |       |
  //  2 |  x    |  x    |   x
  //  3 |       |  x    |
  //  4 |  x    |  x    |   x
  //  5 |  x    |  x    |   x
  //  6 |  x    |       |   x
  //  7 |  x    |       |   x
  //  8 |       |       |   x
  //  9 |       |       |   x
  // 10 |       |       |
  //
  // ... with two possibilities: on frames 2, 4, and 5, are the boxes
  // non-overlapping (v1 below) or overlapping (v2 below)?

  // Naresh's scenario, v1: none of the ground truth boxes overlap.
  {
    string gt_tracks = "a.d.b.d.d.a.a......";  // two targets: a = 6 frames, b = 4 frames; a and b are disjoint
    string c_tracks  = "..c...c.c.c.c.c.c..";  // one track: 7 frames (c will match only a frames)
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd will be 0.5; fa = 0 (b is completely missed)
    // detection pd should be: 5 / 10 = 0.5; fa = 2 (the last two c's)
    // track continuity: 1 (associated with a)
    // track purity: 5/7
    // target continuity: target a = 1; target b = 0 => 0.5
    // target purity: ((5/6) + 0 ) /2
    double expected_track_purity = 5.0/7.0;
    double expected_target_purity = (5.0/6.0) / 2;
    ostringstream oss;
    oss << "0.5 0   0.5 2   1 " << expected_track_purity << "    0.5 " << expected_target_purity;
    verify_scores( "naresh scenario w/ no-intersection", s, oss.str() );
  }
  // Naresh's scenario, v2: in any frame with multiple tracks, the frames overlap.
  {
    string gt_tracks = "a.x.b.x.x.a.a......";  // two targets: a = 6 frames, b = 4 frames; a and b overlap
    string c_tracks  = "..c...c.c.c.c.c.c..";  // one track: 7 frames (c will match only a frames)
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd now is 1.0; fa = 0
    // detection pd should be: 8 / 10 = 0.8; fa = 2 (the last two c's)
    // track continuity: 2 (associated with a and b)
    // track purity: 5/7
    // target continuity: target a = 1; target b = 1 => 1
    // target purity: ((5/6) + (3/4 ) /2
    double expected_track_purity = 5.0/7.0;
    double expected_target_purity = ((5.0/6.0) + (3.0/4.0)) / 2.0;
    ostringstream oss;
    oss << "1 0   0.8 2   2 " << expected_track_purity << "    1 " << expected_target_purity;
    verify_scores( "naresh scenario w/intersection", s, oss.str() );
  }
}

void
test_dominance()
{
  vidtk::track_synthesizer_params p( 10, 5, 10 );
  {
    string gt_tracks = "a.a.a.a.a.....b.b.b.b.b";  // two targets, 5 frames each
    string c_tracks  = "c.c.c.c.c.c.c.c.c.c.c.c";  // one track, 12 frames
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd = 1, fa = 0
    // det pd = 1, fa = 2
    // track continuity: 2 (c -> a,b)
    // track purity: 5 / 12
    // target continuity: a => 1, b => 1  ==> 1
    // target purity: a => 1, b => 1 ==> 1
    double expected_track_purity = 5.0/12.0;
    ostringstream oss;
    oss << "1 0   1 2   2 " << expected_track_purity << "   1 1";
    verify_scores( "dominance test 1", s, oss.str() );
  }

  {
    // as above, but with gt / c reversed
    string gt_tracks = "c.c.c.c.c.c.c.c.c.c.c.c";  // one target, 12 frames
    string c_tracks  = "a.a.a.a.a.....b.b.b.b.b";  // two tracks, 5 frames each
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd = 1, fa = 0
    // det pd = 10/12, fa = 0
    // track continuity: a => 1, b => 1  ==> 1
    // track purity: a => 1, b => 1 ==> 1
    // target continuity: 2 (c -> a,b)
    // target purity: 5 / 12
    double expected_detection_pd = 10.0/12.0;
    double expected_target_purity = 5.0/12.0;
    ostringstream oss;
    oss << "1 0   " << expected_detection_pd << " 0   1 1   2 " << expected_target_purity;
    verify_scores( "dominance test 2", s, oss.str() );
  }

  // now what happens when we replace the last TWO frames of
  // computed 'c' with a new track 'a'?  Three frames will overlap
  // gt's 'b'; computed 'c' should still be dominant
  {
    string gt_tracks = "a.a.a.a.a.....b.b.b.b.b";  // two targets, 5 frames each
    string c_tracks  = "c.c.c.c.c.c.c.c.c.c.a.a";  // two tracks, 10 and 2 frames
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd = 1, fa = 0  (unchanged from #1)
    // det pd = 1, fa = 2 (unchanged from #1)
    // track continuity: 2 (c -> a,b) + 1 (a -> b) = 3/2
    // track purity:
    //   1) track c's predominant target is a; 5 / 10 = 0.5;
    //   2) track a's predominant target is b; 2 / 2  = 1.0; => 1.5/2
    // target continuity: a => 1, b => 2 (tracks c and a); 3/2
    // target purity:
    //   1) target a's predominant track is c; 5/5
    //   2) target b's predominant track is c; 3/5 ... (5/5)+(3/5) / 2 => 0.8
    ostringstream oss;
    oss << "1 0   1 2   1.5 0.75  1.5 0.8";
    verify_scores( "dominance test 3", s, oss.str() );
  }

  // ensure that reversing gt and c makes sense
  {
    string gt_tracks = "c.c.c.c.c.c.c.c.c.c.a.a";  // two targets, 10 and 2 frames
    string c_tracks  = "a.a.a.a.a.....b.b.b.b.b";  // two tracks, 5 frames each
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd = 1, fa = 0 (unchanged from #2)
    // det pd = 10/12, fa = 0 (unchanged from #2)
    // track continuity: a => 1, b => 2 (targets c and a); 3/2
    // track purity:
    //   1) track a's predominant target is c; 5/5
    //   2) track b's predominant target is c; 3/5 ... (5/5)+(3/5) / 2 => 0.8
    // target continuity: 2 (c -> a,b) + 1 (a -> b) = 3/2
    // target purity:
    //   1) target c's predominant track is a; 5 / 10 = 0.5;
    //   2) target a's predominant track is b; 2 / 2  = 1.0; => 1.5/2
    double expected_detection_pd = 10.0/12.0;
    ostringstream oss;
    oss << "1 0   " << expected_detection_pd << " 0   1.5 0.8    1.5 0.75";
    verify_scores( "dominance test 4", s, oss.str() );
  }

  // Finally, if we increase track 'a' from two frames to three,
  // it should become the new dominant track for target b.
  {
    string gt_tracks = "a.a.a.a.a.....b.b.b.b.b";  // two targets, 5 frames each
    string c_tracks  = "c.c.c.c.c.c.c.c.c.a.a.a";  // two tracks, 9 and 3 frames
    scores_type s = compute_stats( p, gt_tracks, c_tracks );
    // track pd = 1, fa = 0  (unchanged from #1)
    // det pd = 1, fa = 2 (unchanged from #1)
    // track continuity: 2 (c -> a,b) + 1 (a -> b) = 3/2 (unchanged from #3)
    // track purity:
    //   1) track c's predominant target is a; 5 / 9;
    //   2) track a's predominant target is b; 3 / 3;
    // target continuity: a => 1, b => 2 (tracks c and a); 3/2 (unchanged from #3)
    // target purity:
    //   1) target a's predominant track is c; 5/5
    //   2) target b's predominant track is a; 3/5 ... (5/5)+(3/5) / 2 => 0.8 (numerically unchanged!)
    double expected_track_purity = (( 5.0/9.0 ) + ( 3.0/3.0 )) / 2.0;
    ostringstream oss;
    oss << "1 0   1 2  1.5 " << expected_track_purity << "  1.5 0.8";
    verify_scores( "dominance test 5", s, oss.str() );
  }

  // ...and ensure it makes sense if we reverse gt and c.
  {
    string gt_tracks = "c.c.c.c.c.c.c.c.c.a.a.a";  // two targets, 9 and 3 frames
    string c_tracks  = "a.a.a.a.a.....b.b.b.b.b";  // two tracks, 5 frames each
    scores_type s = compute_stats( p, gt_tracks, c_tracks, true );
    // track pd = 1, fa = 0  (unchanged from #4)
    // det pd = 10/12, fa = 0 (unchanged from #4)
    // track continuity: a => 1, b => 2 (targets c and a); 3/2 (unchanged from #4)
    // track purity:
    //   1) track a's predominant target is c; 5/5
    //   2) track b's predominant target is a; 3/5 ... (5/5)+(3/5) / 2 => 0.8 (numerically unchanged!)
    // target continuity: 2 (c -> a,b) + 1 (a -> b) = 3/2 (unchanged from #4)
    // target purity:
    //   1) target c's predominant track is a; 5 / 9;
    //   2) target a's predominant track is b; 3 / 3;
    double expected_detection_pd = 10.0/12.0;
    double expected_target_purity = (( 5.0/9.0 ) + ( 3.0/3.0 )) / 2.0;
    ostringstream oss;
    oss << "1 0   " << expected_detection_pd << " 0   1.5 0.8   1.5 " << expected_target_purity;
    verify_scores( "dominance test 6", s, oss.str() );
  }

}

} // anon

int test_continuity_purity( int, char*[] )
{
  testlib_test_start( "test_continuity_purity" );

  test_sanity_stats();
  test_basic_stats();
  test_basic_multitrack_stats();
  test_naresh_scenario();
  test_dominance();

  return testlib_test_summary();
}
