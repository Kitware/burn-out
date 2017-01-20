/*ckwg +5
 * Copyright 2011-2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include <track_oracle/track_oracle.h>
#include <track_oracle/track_base.h>
#include <track_oracle/track_field.h>

#include <track_oracle/track_xgtf/track_xgtf.h>
#include <track_oracle/track_xgtf/file_format_xgtf.h>
#include <track_oracle/scoring_framework/aries_interface/aries_interface.h>

#include <testlib/testlib_test.h>


using std::cerr;
using std::cout;
using std::ostringstream;
using std::string;

using namespace vidtk;

int test_track_xgtf( int argc, char *argv[])
{
  cout << "Testing the xgtf reader...\n";

  if(argc < 2)
  {
      cerr << "Need the data directory as argument\n";
      return EXIT_FAILURE;
  }

  vidtk::track_handle_list_type tracks;
  string dir = argv[1];
  string track_file_fn(dir + "/short.xgtf" );

  const size_t VEHICLE_MOVING_INDEX = vidtk::aries_interface::activity_to_index( "VehicleMoving" );
  const size_t VEHICLE_TURNING_INDEX = vidtk::aries_interface::activity_to_index( "VehicleTurning" );

  file_format_xgtf xgtf_reader;
  xgtf_reader.options().set_promote_pvmoving( false ); // default
  xgtf_reader.read( track_file_fn.c_str(), tracks );

  bool got_two_tracks = (tracks.size() == 2);
  TEST("Loaded two tracks from xgtf file", got_two_tracks, true );

  if (got_two_tracks)
  {
    vidtk::track_xgtf_type xgtf_schema;

    {
      ostringstream oss;
      vidtk::frame_handle_list_type frames = track_oracle::get_frames( tracks[0] );
      oss << "First track has " << frames.size() << " frames; expecting 50";
      TEST( oss.str().c_str(), (frames.size() == 50), true );
    }

    {
      vidtk::frame_handle_type frame( xgtf_schema.frame_number.lookup( 812, tracks[1] ));
      TEST( "Second track has a frame at 812", frame.is_valid(), true );
      unsigned act_id = xgtf_schema( tracks[1] ).activity();
      double act_prob = xgtf_schema( tracks[1] ).activity_probability();
      TEST( "Second track is 'VehicleTurning'", act_id, VEHICLE_TURNING_INDEX );
      TEST( "Second track probability is 1", act_prob, 1.0 );
    }
  }

  // now again, but with PVPromotion
  tracks.clear();
  xgtf_reader.options().set_promote_pvmoving( true ); // default
  xgtf_reader.read( track_file_fn.c_str(), tracks );
  got_two_tracks = (tracks.size() == 2);
  TEST("Loaded two tracks from xgtf file", got_two_tracks, true );

  if (got_two_tracks)
  {
    vidtk::track_xgtf_type xgtf_schema;
    {
      vidtk::frame_handle_type frame( xgtf_schema.frame_number.lookup( 812, tracks[1] ));
      TEST( "Second track has a frame at 812", frame.is_valid(), true );
      unsigned act_id = xgtf_schema( tracks[1] ).activity();
      double act_prob = xgtf_schema( tracks[1] ).activity_probability();
      TEST( "Second track is 'VehicleMoving'", act_id, VEHICLE_MOVING_INDEX );
      TEST( "Second track probability is 1", act_prob, 1.0 );
    }
  }


  return testlib_test_summary();
}

