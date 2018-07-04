/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <string>
#include <iomanip>
#include <cstdlib>

#include <vgl/vgl_intersection.h>

#include <track_oracle/scoring_framework/track_synthesizer.h>

#include <testlib/testlib_test.h>


using std::map;
using std::ostringstream;
using std::string;
using std::vector;

namespace { // anon

//
// unpack a string such as "a11019;b20012" into a map of
// track_id to vector of expected track lengths:
// 1 -> [1, 1, 0, 1, 9]
// 2 -> [2, 0, 0, 1, 2]
//
map< unsigned, vector<size_t> >
unpack_track_length_string( const string& s )
{
  unsigned current_track_id = 0;
  map<unsigned, vector<size_t> > ret;
  for (size_t i=0; i<s.size(); ++i)
  {
    if ((s[i] == 'a') || (s[i] == 'b') || (s[i] == 'c'))
    {
      current_track_id = s[i] - 'a' + 1;
    }
    else if (('0' <= s[i]) && (s[i] <= '9'))
    {
      ret[ current_track_id ].push_back( s[i] - '0' );
    }
  }
  return ret;
}


void
test_synthesized_boxes( const vidtk::track_synthesizer_params& p,
                        const char* track_strings[],
                        const size_t n_tracks,
                        const string& track_lengths_str,
                        const string& tag )
{
  vidtk::track_synthesizer ts( p );
  vidtk::track_handle_list_type tracks;

  map< unsigned, vector<size_t> > track_lengths
    = unpack_track_length_string( track_lengths_str );

  for (size_t i=0; track_strings[i] != 0; ++i)
  {
    // test that make_tracks succeeds
    bool rc = ts.make_tracks( track_strings[i], tracks );
    {
      ostringstream oss;
      oss << tag << " string '" << track_strings[i] << "' succeeds";
      TEST( oss.str().c_str(), rc, true );
    }
    // test that we received the expected number of tracks
    {
      ostringstream oss;
      oss << tag << " string '" << track_strings[i] << "' produces " << n_tracks << " tracks";
      TEST( oss.str().c_str(), tracks.size() == n_tracks, true );
    }
    // if we expected tracks, and got them, check their lengths
    if ( (n_tracks > 0) && (tracks.size() == n_tracks ) )
    {
      vidtk::scorable_track_type trk;
      for (size_t t = 0; t<n_tracks; ++t)
      {
        unsigned track_id = trk( tracks[t] ).external_id();
        size_t expected_length = track_lengths[ track_id ][ i ];
        size_t actual_length = vidtk::track_oracle::get_n_frames( tracks[t] );
        ostringstream oss;
        oss << tag << " string '" << track_strings[i] << "'; track " << track_id
            << " got " << actual_length << " frames; expected " << expected_length;
        TEST( oss.str().c_str(), actual_length, expected_length );
      }
    }
  }
}

void
test_synthesized_track_creation( const vidtk::track_synthesizer_params& p )
{
  // empty tracks
  {
    const char* s[] = {"", ".", "........", 0};
    string lengths = "";
    test_synthesized_boxes( p, s, 0, lengths, "empty track" );
  }

  // single tracks
  {
    const char* s[] = {"a", "..a", "b.b", "b", "ccccccc", "c", 0 };
    string lengths = "a110000;b002100;c000071";
    test_synthesized_boxes( p, s, 1, lengths, "single track" );
  }

  // two-track strings, explicit
  {
    const char* s[] = {"d", "..y", "e.e", "x", "zzzbcbc", "f", 0 };
    string lengths = "a110101;b102150;c012051";
    test_synthesized_boxes( p, s, 2, lengths, "two-track explicit" );
  }

  // two-track strings, implicit
  {
    const char* s[] = {"ac", "b.a", "c.a", "bc", "abababa", "ab", 0 };
    string lengths = "a111041;b010131;c101100";
    test_synthesized_boxes( p, s, 2, lengths, "two-track implicit" );
  }

  // three-track strings, explicit
  {
    const char* s[] = {"g", "..#", "g.#", "X", "Y", "Z", "g#g#g#g##", 0 };
    string lengths = "a1121119;b1121119;c1121119";
    test_synthesized_boxes( p, s, 3, lengths, "three-track explicit" );
  }

  // three-track strings, implicit
  {
    const char* s[] = {"abc", "babc", "c.ba", 0 };
    string lengths ="a111;b121;c111";
    test_synthesized_boxes( p, s, 3, lengths, "three-track implicit" );
  }

  // incorrect track strings
  {
    vidtk::track_synthesizer ts( p );
    vidtk::track_handle_list_type tracks;
    bool rc = ts.make_tracks( "bogus", tracks );
    TEST( "make tracks fails on bad track descriptor characters", rc, false );
  }
}

void
check_track_overlaps( const vidtk::track_synthesizer_params& p,
                      const string& track_descriptor_string,
                      size_t expected_overlaps )
{
    vidtk::track_synthesizer ts( p );
    vidtk::track_handle_list_type tracks;
    bool rc = ts.make_tracks( track_descriptor_string, tracks );
    {
      ostringstream oss;
      oss << "overlap test string '" << track_descriptor_string << "' succeeds";
      TEST( oss.str().c_str(), rc, true );
    }
    if (! rc ) return;

    // build up a map of all the boxes on each frame
    vidtk::scorable_track_type trk;
    typedef map< unsigned, vector< vgl_box_2d<double> > > box_map_type;
    typedef map< unsigned, vector< vgl_box_2d<double> > >::const_iterator box_map_cit;

    box_map_type box_map;
    for (size_t i=0; i<tracks.size(); ++i)
    {
      vidtk::frame_handle_list_type frames = vidtk::track_oracle::get_frames( tracks[i] );
      for (size_t f=0; f<frames.size(); ++f)
      {
        vidtk::frame_handle_type fh = frames[f];
        if ( trk[ fh ].bounding_box.exists() )
        {
          unsigned frame_num = trk[ fh ].timestamp_frame();
          box_map[ frame_num ].push_back( trk[ fh ].bounding_box() );
          // check the area of the box just to be sure
          ostringstream oss;
          double expected_area = p.box_side_length * p.box_side_length;
          double actual_area = box_map[ frame_num ].back().volume();
          oss << "track index " << i << " frame " << frame_num
              << ": area is " << actual_area << "; expected " << expected_area;
          TEST( oss.str().c_str(), actual_area, expected_area );
        }
      }
    }

    // on each frame, count up how many of the boxes overlap
    for (box_map_cit i = box_map.begin(); i != box_map.end(); ++i)
    {
      size_t n_overlaps = 0;
      const vector< vgl_box_2d<double> >& box_list = i->second;
      size_t n_boxes = box_list.size();
      if ( n_boxes > 1 )
      {
        for (size_t j=0; j < n_boxes-1; ++j)
        {
          const vgl_box_2d<double>& box_A = box_list[j];
          for (size_t k=j+1; k < n_boxes; ++k)
          {
            const vgl_box_2d<double>& box_B = box_list[k];
            vgl_box_2d<double> overlap = vgl_intersection( box_A, box_B );
            if ( ! overlap.is_empty() ) ++n_overlaps;
          }
        }
      }
      ostringstream oss;
      oss << "overlap test: char '" << track_descriptor_string[i->first] << "' has "
          << n_overlaps << " overlaps; expecting " << expected_overlaps;
      TEST( oss.str().c_str(), n_overlaps, expected_overlaps );
    }
}

void
test_box_overlaps( const vidtk::track_synthesizer_params& p )
{
  // these tracks should have no overlaps on any frame
  check_track_overlaps( p, ".abcdefg", 0 );

  // these tracks should have 1 overlap on each frame
  check_track_overlaps( p, "xyzXYZ", 1 );

  // this track should have 3 overlaps
  check_track_overlaps( p, "#", 3 );

  // note, we don't have a situation for producing only 2 overlaps;
  // see the comments in track_synthesizer.h for an explanation why
}

} // anon

int test_track_synthesizer( int , char *[] )
{
  testlib_test_start( "test_track_synthesizer" );
  vidtk::track_synthesizer_params p( 10, 5, 10 );

  test_synthesized_track_creation( p );
  test_box_overlaps( p );

  return testlib_test_summary();
}
