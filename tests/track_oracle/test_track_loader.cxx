/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <cstring>
#include <vul/vul_arg.h>
#include <vul/vul_file.h>

#include <testlib/testlib_test.h>

#include <track_oracle/track_kwxml/track_kwxml.h>
#include <track_oracle/track_kwxml/file_format_kwxml.h>
#include <track_oracle/scoring_framework/score_tracks_loader.h>
#include <track_oracle/scoring_framework/virat_scenario_utilities.h>

#include <logger/logger.h>


using std::make_pair;
using std::pair;
using std::strcpy;
using std::string;
using std::vector;

using namespace vidtk;

VIDTK_LOGGER("test_track_loader");

pair< size_t, size_t >
timestamp_vs_frame_number( const track_handle_list_type& tracks )
{
  track_kwxml_type kwxml;
  size_t ts_fn_same_count = 0;
  size_t ts_fn_differ_count = 0;
  for (size_t i=0; i < tracks.size(); ++i)
  {
    frame_handle_list_type frames = track_oracle::get_frames( tracks[i] );
    for (size_t j=0; j < frames.size(); ++j)
    {
      unsigned fn = kwxml[ frames[j] ].frame_number();
      unsigned long long ts = kwxml[ frames[j] ].timestamp_usecs();
      if (ts == static_cast< unsigned long long >(fn) * 1000 * 1000)
      {
        ++ts_fn_same_count;
      }
      else
      {
        ++ts_fn_differ_count;
      }
    }
  }
  return make_pair( ts_fn_same_count, ts_fn_differ_count );
}

int test_track_loader( int argc, char *argv[])
{
  if(argc < 2)
  {
    LOG_ERROR("Need the data directory as argument\n");
    return EXIT_FAILURE;
  }

  testlib_test_start( "test_track_loader");

  string dir = argv[1];
  string single_track_file(dir + "/kwxml_file_test.xml" );

  //
  // step 1: assert that timestamps and frame numbers are different in kwxml_file_test.xml
  //

  {
    track_handle_list_type tracks;
    file_format_kwxml kwxml_reader;
    kwxml_reader.read( single_track_file, tracks );

    TEST( "Loaded tracks from kwxml file", ( ! tracks.empty() ), true );
    pair<size_t, size_t> stats = timestamp_vs_frame_number( tracks );

    TEST( "Test file: zero frames with same frame number as timestamp", stats.first == 0, true );
    TEST( "Test file: non-zero frames with different frame numbers than timestamp", stats.second > 0, true );
  }

  //
  // First test that disabling fn2ts acts as above
  //


  vector< string > my_arg_strings;
  my_arg_strings.push_back("test_command_name");
  my_arg_strings.push_back("--computed-tracks");
  my_arg_strings.push_back( single_track_file );
  my_arg_strings.push_back("--truth-tracks");
  my_arg_strings.push_back( single_track_file );

  input_args_type iat;
  int my_argc = static_cast<int>( my_arg_strings.size() );
  char **my_argv;
  my_argv = static_cast< char ** >( malloc( sizeof( char * ) * (my_argc + 1) ) );
  for (int i=0; i<my_argc; ++i)
  {
    // <meme> I was told there would be strdup() </meme>
    my_argv[i] = static_cast< char *>( malloc( my_arg_strings[i].size() + 1) );
    strcpy( my_argv[i], my_arg_strings[i].c_str() );
  }
  my_argv[my_argc] = 0;
  vul_arg_parse( my_argc, my_argv );
  iat.ts_from_fn() = false;

  // ...since vul_arg messes around with argv, I'm not sure yet how to
  // tell when it's safe to free my_argv[i] up, so this will leak

  {
    track_handle_list_type c, g;
    bool rc = iat.process( c, g );
    TEST( "Without fn2ts: process() succeeds ", rc, true );
    pair< size_t, size_t > stats = timestamp_vs_frame_number( c );
    TEST( "Without fn2ts: zero frames with same frame number as timestamp", stats.first == 0, true );
    TEST( "Without fn2ts: non-zero frames with different frame number than timestamp", stats.second > 0, true );
  }

  //
  // now set ts2fn and re-run
  //
  iat.ts_from_fn() = true;

  {
    track_handle_list_type c, g;
    bool rc = iat.process( c, g );
    TEST( "With fn2ts: process() succeeds ", rc, true );
    pair< size_t, size_t > stats = timestamp_vs_frame_number( c );
    TEST( "With fn2ts: non-zero frames with same frame number as timestamp", stats.first > 0, true );
    TEST( "With fn2ts: zero frames with different frame number than timestamp", stats.second == 0, true );
  }

  //
  // unset ts2fn; set truth tracks to be a VIRAT scenario
  //
  //
  // ...without --truth-path parameter, should fail
  //
  iat.ts_from_fn() = false;
  iat.truth_tracks_fn() = dir+"/test-virat-scenario.xml";
  TEST( string("virat sample scenario '"+iat.truth_tracks_fn()+"' exists").c_str(), vul_file::exists( iat.truth_tracks_fn() ), true );
  TEST( string("virat sample scenario '"+iat.truth_tracks_fn()+"' probes as a scenario").c_str(), virat_scenario_utilities::fn_is_virat_scenario( iat.truth_tracks_fn() ), true );
  {
    track_handle_list_type c,g;
    bool rc = iat.process( c, g );
    TEST( "Loading virat scenario file without --truth-path parameter fails", rc, false );
  }

  // Set --truth-path
  iat.truth_path() = dir+"/xgtf";
  {
    track_handle_list_type c,g;
    bool rc = iat.process( c, g );
    TEST( "Loading virat scenario file with --truth-path parameter succeeds", rc, true );
  }


  return testlib_test_summary();

}
