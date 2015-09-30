/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/viper_reader.h>
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
  vcl_string xmlPath(argv[1]);
  xmlPath += "/viper-test.xml";
  vcl_vector< vidtk::track_sptr > tracks;
  vidtk::viper_reader *reader = new vidtk::viper_reader(xmlPath.c_str());
  TEST( "Loaded viper XML okay", reader->read(tracks), true );
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

  reader->set_filename(xmlPath.c_str());
  reader->set_ignore_occlusions(true);
  reader->set_ignore_partial_occlusions(true);

  TEST( "Loaded viper XML okay without occlusions", reader->read(tracks), true );
  TEST( "13 non occluded tracks", (tracks[37]->history().size()) == 13, true);

  delete reader;
  return testlib_test_summary();
}
