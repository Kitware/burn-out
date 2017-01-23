/*ckwg +5
 * Copyright 2012 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>
#include <cstdlib>

#include <track_oracle/track_oracle.h>
#include <track_oracle/track_base.h>
#include <track_oracle/track_field.h>

#include <track_oracle/file_format_manager.h>

#include <track_oracle/track_scorable_mgrs/track_scorable_mgrs.h>

#include <testlib/testlib_test.h>


using std::cerr;
using std::ostringstream;
using std::string;
using std::vector;

namespace {  /* anonymous */

void test_mgrs_load( const string& data_dir )
{

  vector< vidtk::track_handle_list_type > tracks;
  vector< string > track_fns;
  track_fns.push_back( data_dir+"/score_tracks_data/GTs/AlignTestGT.kw18" );
  track_fns.push_back( data_dir+"/short.xgtf" );
  // would add apix tracks if we had some non-export-controlled ones in the test data dir...

  for (size_t i=0; i<track_fns.size(); ++i)
  {
    vidtk::track_handle_list_type t;
    bool okay = vidtk::file_format_manager::read( track_fns[i], t );
    ostringstream oss;
    oss << "Loaded tracks from '" << track_fns[i] << "'";
    TEST( oss.str().c_str(), okay, true );
    if (okay)
    {
      tracks.push_back( t );
    }
  }
  if ( tracks.size() != track_fns.size() ) return;

  // KW18 should work
  {
    bool okay = vidtk::track_scorable_mgrs_type::set_from_tracklist( tracks[0] );
    TEST( "KW18 could convert to mgrs", okay, true );
  }
  // xgtf should not
  {
    bool okay = vidtk::track_scorable_mgrs_type::set_from_tracklist( tracks[1] );
    TEST( "XGTF could not convert to mgrs", okay, false );
  }
}


} // anon namespace

int test_track_scorable_mgrs( int argc, char *argv[] )
{
  if (argc < 2)
  {
    cerr << "Need the data directory as argument\n";
    return EXIT_FAILURE;
  }

  testlib_test_start( "test_track_scorable_mgrs" );

  test_mgrs_load( argv[1] );

  return testlib_test_summary();
}
