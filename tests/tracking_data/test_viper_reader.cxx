/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/io/track_reader.h>
#include <testlib/testlib_test.h>

using namespace vidtk;

int test_viper_reader( int argc, char *argv[] )
{
  testlib_test_start( "testViperReader" );
  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }
  std::string xmlPath(argv[1]);
  xmlPath += "/viper-test.xml";
  {
    std::vector< vidtk::track_sptr > tracks;
    vidtk::track_reader reader( xmlPath );
    if ( ! reader.open() )
    {
      std::ostringstream oss;
      oss << "Failed to open '" << xmlPath << "'";
      TEST(oss.str().c_str(), false, true);
      return 1;
    }

    TEST( "Loaded viper XML okay", reader.read_all( tracks ), 38 );
    TEST( "Loaded 38 tracks", (tracks.size() == 38), true );
    TEST( "210 non occluded tracks", (tracks[0]->history().size()) == 210, true);
    TEST( "42 non occluded tracks", (tracks[3]->history().size()) == 42, true);
    TEST( "Track 3 is id 30", tracks[3]->id() == 30, true);
    TEST( "Track 35 is id 71", tracks[35]->id() == 71, true);
    TEST( "Track 3 history 30 is framenum 339", tracks[3]->history()[30]->time_.frame_number() == 339, true);
    TEST( "Track 3 history 30 loc x", tracks[3]->history()[30]->loc_[0], 598.0);
    TEST( "Track 3 history 30 loc y", tracks[3]->history()[30]->loc_[1], 437.0);
    TEST( "Track 3 history 30 loc z", tracks[3]->history()[30]->loc_[2], 0.0);
    TEST( "Track 3 history 30 min pos x", tracks[3]->history()[30]->amhi_bbox_.min_x(), 588);
    TEST( "Track 3 history 30 min pos y", tracks[3]->history()[30]->amhi_bbox_.min_y(), 426);
    TEST( "Track 3 history 30 max pos x", tracks[3]->history()[30]->amhi_bbox_.max_x(), 608);
    TEST( "Track 3 history 30 max pos y", tracks[3]->history()[30]->amhi_bbox_.max_y(), 448);
  }

  {
    std::vector< vidtk::track_sptr > tracks;
    vidtk::track_reader reader ( xmlPath );
    vidtk::ns_track_reader::track_reader_options opt;
    opt.set_ignore_occlusions(true);
    opt.set_ignore_partial_occlusions(true);
    reader.update_options( opt);

    if ( ! reader.open() )
    {
      std::ostringstream oss;
      oss << "Failed to open '" << xmlPath << "'";
      TEST(oss.str().c_str(), false, true);
    }
    else
    {
      TEST( "Loaded viper XML okay without occlusions", reader.read_all(tracks), 38 );
      TEST( "13 non occluded states", (tracks[37]->history().size()) == 13, true);
    }
  }

  return testlib_test_summary();
}
