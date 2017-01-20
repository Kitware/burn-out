/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include <track_oracle/track_oracle.h>
#include <track_oracle/track_kw18/file_format_kw18.h>
#include <track_oracle/track_scorable_mgrs/track_scorable_mgrs.h>

#include <track_oracle/scoring_framework/phase1_parameters.h>

#include <testlib/testlib_test.h>

#include <logger/logger.h>


using std::map;
using std::ostringstream;
using std::string;
using std::stringstream;

using namespace vidtk;

VIDTK_LOGGER("test_track_oracle_geo_aoi");

namespace // anon
{

//
// Our goal is to create three tracks, which will be tested against
// the geo AOI: track A should definitely be within the AOI, track B
// should definitely be outside, but track C is the tricky one.  This
// track is inside the AOI when the geo box is a single lat/lon
// bounding box, but is (or should be) outside when the geo box is the
// convex polygon of the UTM coordinates of the lat/lon bounding box.
// At the beginning of this branch, testing that C is outside the box
// is the failing test which the branch aims to correct.
//

bool
create_test_tracks( track_handle_list_type& tracks )
{

  // the test tracks are derived from the LAIR dataset Arslan supplied
  // to Roddy as part of the problems triggering this branch.

  stringstream ss;

  // The tricky point C is lon/lat -84.13970741343819 39.77012348814779,
  // whose UTM coords are 16N 744989 4406157.  We generate point A (inside)
  // and point B (outside).

  // track A:
  ss << "1 1 100 "   // track 1, 1 frame long on frame 100
     << "0 0 0 0 "   // loc x,y vel x,y irrelevant
     << "0 0 "       // obj x,y irrelevant
     << "0 0 0 0 "   // bbox irrelevant
     << "0 "         // area irrelevant
     << "-84.1100 39.77012348814779 " // world x,y relevant
     << "0 "         // world z irrelevant
     << "100\n";     // timestamp 100

  // track B:
  ss << "2 1 200 "   // track 2, 1 frame long on frame 200
     << "0 0 0 0 "   // loc x,y vel x,y irrelevant
     << "0 0 "       // obj x,y irrelevant
     << "0 0 0 0 "   // bbox irrelevant
     << "0 "         // area irrelevant
     << "-84.1500 39.77012348814779 " // world x,y relevant
     << "0 "         // world z irrelevant
     << "200\n";       // timestamp 200

  // track C:
  ss << "3 1 300 "   // track 3, 1 frame long on frame 300
     << "0 0 0 0 "   // loc x,y vel x,y irrelevant
     << "0 0 "       // obj x,y irrelevant
     << "0 0 0 0 "   // bbox irrelevant
     << "0 "         // area irrelevant
     << "-84.13970741343819 39.77012348814779 " // world x,y relevant
     << "0 "         // world z irrelevant
     << "300\n";     // timestamp 300

  // read 'em in
  vidtk::file_format_kw18 kw18;
  bool rc = kw18.read( ss, tracks );
  TEST( "kw18 reader succeeded on track stream", rc, true );
  if ( rc )
  {
    ostringstream oss;
    size_t n = tracks.size();
    oss << "Expected 3 tracks; found " << n;
    TEST( oss.str().c_str(), n, 3 );
    rc = (n == 3);
    if ( ! rc )
    {
      vidtk::track_kw18_type trk;
      LOG_DEBUG("tracks:\n");
      for (size_t i=0; i<tracks.size(); ++i)
      {
        LOG_DEBUG( trk( tracks[i] ) );
      }
    }
  }

  if ( rc )
  {
    rc = track_scorable_mgrs_type::set_from_tracklist( tracks );
    TEST( "Added scorablge_mgrs", rc, true );
  }

  return rc;
}

} // anon

int test_track_oracle_geo_aoi( int /* argc */, char * /*argv*/ [] )
{
  testlib_test_start( "test_track_oracle_geo_aoi" );

  track_handle_list_type tracks;
  phase1_parameters p1_params;
  bool rc = create_test_tracks( tracks );
  if ( rc )
  {
    // set up the lat/lon AOI box
    p1_params.radial_overlap = 10.0;
    const string geo_aoi = "-84.139042,39.799339:-84.095042,39.75533";
    rc = p1_params.setAOI( geo_aoi );
    TEST( "Set p1 geo AOI", rc, true );
  }
  if ( rc )
  {
    vidtk::track_kw18_type trk;

    // filter on the AOI
    track_handle_list_type filtered_tracks;
    p1_params.filter_track_list_on_aoi( tracks, filtered_tracks );
    {
      LOG_DEBUG("filtered tracks:\n");
      for (size_t i=0; i<filtered_tracks.size(); ++i)
      {
        LOG_DEBUG( trk( tracks[i] ) );
      }
    }

    size_t n = filtered_tracks.size();
    {
      ostringstream oss;
      oss << "Filtering produced " << n << " tracks; expected 1";
      TEST( oss.str().c_str(), n, 1 );
    }
    {
      map<unsigned, bool> track_present_map;
      for (size_t i=0; i<n; ++i)
      {
        track_present_map[ trk( filtered_tracks[i] ).external_id() ] = true;
      }
      TEST( "Track A is inside AOI", track_present_map[ 1 ], true );
      TEST( "Track B is outside AOI", track_present_map[ 2 ], false );
      TEST( "Track C is outside AOI", track_present_map[ 3 ], false );
    }
  }

  return testlib_test_summary();
}
