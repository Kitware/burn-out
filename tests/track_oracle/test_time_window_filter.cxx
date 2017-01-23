/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <sstream>
#include <map>
#include <track_oracle/track_oracle_api_types.h>
#include <track_oracle/data_terms/data_terms.h>
#include <track_oracle/scoring_framework/track_synthesizer.h>
#include <track_oracle/scoring_framework/time_window_filter.h>

#include <testlib/testlib_test.h>


using std::map;
using std::ostringstream;
using std::string;

namespace { // anon

using namespace vidtk;

void
test_twf( const string& twf_str,
          const track_handle_list_type& tracks,
          const string& expected_ids_to_pass )
{
  time_window_filter twf;
  track_field< dt::tracking::external_id > track_id;
  bool rc = twf.set_from_string( twf_str );
  string msg( "Set twf from '"+twf_str+"' passes" );
  TEST( msg.c_str(), rc, true );
  if (! rc) return;

  map< unsigned, bool > expected_to_pass;
  for (size_t i=0; i<expected_ids_to_pass.size(); ++i)
  {
    unsigned id = expected_ids_to_pass[i] - '0';
    expected_to_pass[ id ] = true;
  }

  map< unsigned, bool > passed;
  for (size_t i=0; i<tracks.size(); ++i)
  {
    bool p = twf.track_passes_filter( tracks[i] );
    passed[ track_id( tracks[i].row ) ] = p;
  }

  for (map< unsigned, bool>::const_iterator i=passed.begin(); i != passed.end(); ++i)
  {
    ostringstream oss;
    oss << "twf '" << twf_str << "': track " << i->first
        << " expected to pass: " << expected_to_pass[ i->first ]
        << " ; did pass: " << i-> second;
    TEST( oss.str().c_str(), expected_to_pass[ i->first ], i->second );
  }
}

} // anon namespace

int test_time_window_filter( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_time_window_filter" );

  track_synthesizer_params p( 10, 5, 1.0 ); // 1.0 fps
  track_synthesizer ts( p );
  track_handle_list_type tracks;

  bool rc = ts.make_tracks( "...aaa...bbb...xxx...ccc...zzz...ccc...", tracks);
  // i.e.
  // a -> [3..17]
  // b -> [9..29]
  // c -> [21..35]
  TEST( "make_tracks succeeds", rc, true );
  {
    ostringstream oss;
    oss << "Got " << tracks.size() << " tracks; expected 3";
    TEST( oss.str().c_str(), tracks.size(), 3 );
  }

  // null cases
  test_twf( "if:", tracks, "123" );
  test_twf( "xt:", tracks, "123" );
  test_twf( "iT:", tracks, "123" );

  // end cases
  test_twf( "if0:2", tracks, "" );
  test_twf( "xf:2", tracks, "" );
  test_twf( "if36:", tracks, "" );

  // partial bounds
  test_twf( "xf9:", tracks, "23" );
  test_twf( "xf:18", tracks, "1" );
  test_twf( "if9:", tracks, "123" );
  test_twf( "if:18", tracks, "12" );

  // boundary cases
  test_twf( "xf3:17", tracks, "1" );
  test_twf( "xf4:17", tracks, "" );
  test_twf( "xf3:15", tracks, "" );
  test_twf( "if3:17", tracks, "12" );
  test_twf( "if4:17", tracks, "12" );
  test_twf( "if3:15", tracks, "12" );

  // reversed
  test_twf( "xf17:3", tracks, "1" );

  // more tracks!
  test_twf( "xf3:29", tracks, "12" );
  test_twf( "xf4:30", tracks, "2" );
  test_twf( "xf21:35", tracks, "3" );
  test_twf( "xf8:36", tracks, "23" );
  test_twf( "if3:29", tracks, "123" );
  test_twf( "if4:30", tracks, "123" );
  test_twf( "if21:35", tracks, "23" );
  test_twf( "if8:36", tracks, "123" );

  // timestamps
  test_twf( "xt3000000:17000000", tracks, "1" );
  test_twf( "xT3:17", tracks, "1" );
  test_twf( "xt3000001:17000000", tracks, "" );
  test_twf( "xT3.1:17", tracks, "" );
  test_twf( "xt2999999:17000000", tracks, "1" );
  test_twf( "xT2.999:17.1", tracks, "1" );
  test_twf( "iT2.999:17.1", tracks, "12" );
  test_twf( "iT2.999:21.1", tracks, "123" );

  return testlib_test_summary();
}
