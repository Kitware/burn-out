/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>
#include <tracking/track_reader_process.h>

namespace {
vcl_string g_data_dir;
}

using namespace vidtk;

int test_track_reader_process( int /*argc*/, char* argv[] )
{
  testlib_test_start( "track initializer process" );
  g_data_dir = argv[1];
  vcl_vector<vidtk::track_sptr> trks;

  track_reader_process trp("track_reader");
  config_block blk = trp.params();
  blk.set("format","mit");
  blk.set("filename",g_data_dir+"/mit_layer_annotation_test_tracks.xml");
  blk.set("ignore_occlusions","true");
  blk.set("ignore_partial_occlusions","true");

  TEST("Set parameters", trp.set_params(blk),true);
  TEST("Initialize", trp.initialize(), true);

  //trks = trp.tracks();
  unsigned int at = 0;
  bool v;
  unsigned int step_count_ = 0;
  while( (v = trp.step()) )
  {
    step_count_++;
    if(trp.tracks().size())
    {
      trks = trp.tracks();
      if(at == 3)
      {
        TEST("Object 7 Frame 3 Frame Num",trks[0]->history()[3]->time_.frame_number(), 10);
      }
      if(at == 4)
      {
        //vcl_cout << trks[0]->history().size() << "   " << trks[0]->history()[0]->loc_[0] << " " <<  (126.0+141.0)/2.0) << vcl_endl;
        TEST("Object 0 Frame 0 X Pos",trks[0]->history()[0]->loc_[0], (126.0+141.0)/2.0);
        TEST("Object 0 Frame 59 Y Pos",trks[0]->history()[59]->loc_[1], (116.025+129.025)/2.0);
        TEST("Object 0 Frame 60 Frame Num",trks[0]->history()[60]->time_.frame_number(), 60);
      }
      if(at == 5)
      {
        TEST("Object 4 index", trks[0]->id(), (unsigned int) 4);
        TEST("Object 5 Frame 1 x_min", trks[1]->history()[0]->amhi_bbox_.min_x(),481);
        TEST("Object 5 Frame 1 x_max", trks[1]->history()[0]->amhi_bbox_.max_x(),568);
        TEST("Object 5 Frame 1 y_min", trks[1]->history()[0]->amhi_bbox_.min_y(),127);
        TEST("Object 5 Frame 1 y_max", trks[1]->history()[0]->amhi_bbox_.max_y(),198);
      }
      at += trp.tracks().size();
    }
  }
  TEST("653 Steps", step_count_, 653);

  blk.set("format","");
  blk.set("filename",g_data_dir+"/kw18-test.trk");
  blk.set("ignore_occlusions","true");
  blk.set("ignore_partial_occlusions","true");

  TEST("Set parameters", trp.set_params(blk),true);
  TEST("Initialize", trp.initialize(), true);

  trks = trp.tracks();
  at = 0;
  unsigned int count = 0;
  step_count_ = 0;
  while( (v = trp.step()) )
  {
    step_count_++;
    trks = trp.tracks();
    count += trks.size();
    if(trks.size())
    {
      if(at == 3)
      {
        TEST("Track 3 ID is 800006", trks[0]->id(), 800006);
      }
      if(at == 8)
      {
        TEST("Track 8 has 33 frames", trks[0]->history().size(), 33);
      }
      if(at == 250)
      {
        TEST("Track 250 Frame 2 Loc X", trks[0]->history()[2]->loc_[0],197.768);
        TEST("Track 250 Frame 2 Loc Y", trks[0]->history()[2]->loc_[1],171.715);
        TEST("Track 250 Frame 2 Loc Z", trks[0]->history()[2]->loc_[2],0.0);

        TEST("Track 250 Frame 2 Vel X", trks[0]->history()[2]->vel_[0],0.31962200000000002);
        TEST("Track 250 Frame 2 Vel Y", trks[0]->history()[2]->vel_[1],0.10968700000000001);
        TEST("Track 250 Frame 2 Vel Z", trks[0]->history()[2]->vel_[2],0.0);

        TEST("Track 250 Frame 2 min_x", trks[0]->history()[2]->amhi_bbox_.min_x(),507);
        TEST("Track 250 Frame 2 min_y", trks[0]->history()[2]->amhi_bbox_.min_y(),42);
        TEST("Track 250 Frame 2 max_x", trks[0]->history()[2]->amhi_bbox_.max_x(),519);
        TEST("Track 250 Frame 2 max_y", trks[0]->history()[2]->amhi_bbox_.max_y(),73);
      }
      at += trks.size();
    }
  }
  TEST("3018 Steps", step_count_, 3018);

  TEST("There are 301 Tracks", count, 301);

  blk.set("format","viper");
  blk.set("filename",g_data_dir+"/viper-test.xml");
  blk.set("ignore_occlusions","false");
  blk.set("ignore_partial_occlusions","false");

  TEST("Set parameters", trp.set_params(blk),true);
  TEST("Initialize", trp.initialize(), true);

  at = 0;
  count = 0;
  step_count_ = 0;
  while( (v = trp.step()) )
  {
    step_count_++;
    trks = trp.tracks();
    count += trks.size();
    if(trks.size())
    {
      if(at == 8)
      {
        TEST( "210 non occluded tracks", trks[0]->history().size(), 210);
      }
      if(at == 2)
      {
        TEST( "42 non occluded tracks", trks[0]->history().size(), 42);
        TEST( "Track 3 is id 30", trks[0]->id(), 30);
        TEST( "Track 3 history 30 is framenum 339", trks[0]->history()[30]->time_.frame_number(), 339);
        TEST( "Track 3 history 30 loc x", trks[0]->history()[30]->loc_[0], 598.0);
        TEST( "Track 3 history 30 loc y", trks[0]->history()[30]->loc_[1], 437.0);
        TEST( "Track 3 history 30 loc z", trks[0]->history()[30]->loc_[2], 0.0); 
        TEST( "Track 3 history 30 min pos x", trks[0]->history()[30]->amhi_bbox_.min_x(), 588); 
        TEST( "Track 3 history 30 min pos y", trks[0]->history()[30]->amhi_bbox_.min_y(), 426);
        TEST( "Track 3 history 30 max pos x", trks[0]->history()[30]->amhi_bbox_.max_x(), 608); 
        TEST( "Track 3 history 30 max pos y", trks[0]->history()[30]->amhi_bbox_.max_y(), 448);
      }
      if(at == 25)
      {
        TEST( "Track 35 is id 71", trks[0]->id() == 71, true);
      }
    }
    at += trks.size();
  }
  TEST("8995 Steps", step_count_, 8995);

  TEST( "Loaded 35 tracks", count, 35 ); //was 38  some have 0 history

  return testlib_test_summary();
}
