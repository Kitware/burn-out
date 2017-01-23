/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>
#include <tracking_data/io/track_reader_process.h>
#include <utilities/config_block.h>

#define LOCAL_DEBUG 0

#define TEST_ACTIVE_TRACK_COUNT(F,C)                                    \
TEST("Active tracks at frame " # F, trp.active_tracks().size(), (C)); \
TEST( "Correct timestamp at frame " # F, trp.get_current_timestamp().frame_number(), F )

using namespace vidtk;

namespace {
std::string g_data_dir;


// ----------------------------------------------------------------
/**
 *
 *
 */
void simple_kw18_latlon_test()
{
  std::cout << "simple KW18 lat/lon test: start\n";

  track_reader_process trp( "track_reader" );
  trp.set_batch_mode( true );

  config_block blk = trp.params();

  blk.set( "format","kw18" );
  blk.set( "filename", g_data_dir+"/track_reader_data.kw18" );
  blk.set( "read_lat_lon_for_world", "true" );

  config_block dummy_blk = blk;
  config_block to_remove;
  to_remove.add_parameter( "read_lat_lon_for_world", "false", "" );
  dummy_blk.remove( to_remove );
  TEST("set params unexpected", trp.set_params( dummy_blk ), false);

  // Test normal operation
  TEST("set params expected", trp.set_params( blk ), true);
  TEST("intialize", trp.initialize(), true );
  TEST("step", trp.step(), true );
  std::vector< track_sptr > trks = trp.tracks();

  TEST("Read all 3 tracks in file", trks.size(), 3 );
  double lat,lon;
  TEST(" Retrieve lat/lon value 1",
       trks[0]->last_state()->latitude_longitude( lat, lon ), true );
  TEST("Checking read in lat/lon value 1",
       lat == 39.767219577760698712 && lon == -84.146318363345500302, true );

  TEST(" Retrieve lat/lon value 2",
       trks[1]->last_state()->latitude_longitude( lat, lon ), true );
  TEST("Checking read in lat/lon value 2",
       lat == 39.76696993782349665 && lon == -84.145888117371299586, true );

  // Now make sure that lat/lon is not populated unnecessarily
  blk.set( "read_lat_lon_for_world", "false" );
  TEST("set params expected", trp.set_params( blk ), true);
  TEST("intialize", trp.initialize(), true );
  TEST("step", trp.step(), true );
  trks = trp.tracks();
  TEST(" Try retrieve invalid lat/lon value 1",
       trks[0]->last_state()->latitude_longitude( lat, lon ), false );

  std::cout << "simple KW18 lat/lon test: finish\n";
}

bool verify_tracks (std::vector< unsigned > & ids, std::vector<vidtk::track_sptr> const& tracks)
{
  for (size_t i = 0; i < tracks.size(); i++)
  {
    std::vector< unsigned >::iterator ix = find( ids.begin(), ids.end(), tracks[i]->id() );
    if (ix == ids.end() )
    {

#if LOCAL_DEBUG
      std::cout << "Track " << tracks[i]->id() << " not found\n";
#endif

      return false;
    }

    ids.erase(ix);
  } // end for

#if LOCAL_DEBUG
  if (! ids.empty() )
  {
    std::cout << "Tracks still left: ";
    std::vector< unsigned >::iterator ix;
    for (ix = ids.begin(); ix != ids.end(); ix++)
    {
      std::cout << *ix << " ";
    }
    std::cout << std::endl;
  }
#endif

  return ids.empty();
}


// ----------------------------------------------------------------
/**
 *
 *
 */
void test_disabled()
{
  track_reader_process trp( "track_reader" );

  config_block blk = trp.params();

  blk.set( "format", "mit" );
  blk.set( "disabled", "true" );

  TEST( "Set parameters", trp.set_params( blk ), true );
  TEST( "Initialize", trp.initialize(), true );
  TEST( "step", trp.step(), true );

  TEST( "Check disabled mode", trp.is_disabled(), true );
}


void test_batch_active()
{
  track_reader_process trp( "track_reader" );

  config_block blk = trp.params();

  blk.set( "format","kw18" );
  blk.set( "filename", g_data_dir+"/track_reader_data.kw18" );
  blk.set( "active_tracks_disable", "false" );
  trp.set_batch_mode( true );

  TEST( "Set parameters", trp.set_params( blk ), true );
  TEST( "Initialize", trp.initialize(), true );
  TEST( "step", trp.step(), true );

  TEST( "Test batch mode", trp.is_batch_mode(), true );

  // there should be no active tracks in batch mode
  TEST( "Track count", trp.tracks().size(), 3 );
  TEST( "Active track count", trp.active_tracks().size(), 0 );
  TEST( "Test second step", trp.step(), false);
}


void test_file_does_not_exist()
{
  track_reader_process trp( "track_reader" );

  config_block blk = trp.params();

  blk.set( "format","kw18" );
  blk.set( "filename", g_data_dir+"/DOES_NOT_EXIST.kw18" );
  blk.set( "active_tracks_disable", "false" );
  trp.set_batch_mode( true );

  TEST( "Set parameters", trp.set_params( blk ), true );
  TEST( "Initialize", trp.initialize(), false );
}



} // end namespace



using namespace vidtk;

int test_track_reader_process( int argc, char* argv[] )
{
  if ( argc < 2 )
  {
    TEST( "DATA directory not specified", false, true );
    return EXIT_FAILURE;
  }

  testlib_test_start( "track reader process" );
  g_data_dir = argv[1];
  std::vector< vidtk::track_sptr > trks;

  simple_kw18_latlon_test();
  test_disabled();
  test_batch_active();
  test_file_does_not_exist();

  track_reader_process trp( "track_reader" );

  {
    config_block blk = trp.params();
    blk.set( "format", "mit" );
    blk.set( "filename", g_data_dir + "/mit_layer_annotation_test_tracks.xml" );
    blk.set( "ignore_occlusions", "true" );
    blk.set( "ignore_partial_occlusions", "true" );
    blk.set( "active_tracks_disable", "false" );

    TEST( "Set parameters", trp.set_params( blk ), true );
    TEST( "Initialize", trp.initialize(), true );
  }

  //trks = trp.tracks();
  size_t at = 0;
  bool v;
  unsigned int step_count_ = 0;
  timestamp ts;
  ts.set_frame_number( 0 );
  trp.set_timestamp( ts );
  while ( ( v = trp.step() ) )
  {
    if ( ! trp.tracks().empty() )
    {
      trks = trp.tracks();

#if LOCAL_DEBUG
      std::cout  << "There are " << trks.size()
                << " terminated tracks at step " << step_count_  << "\n";
      for ( size_t i = 0; i < trks.size(); i++ )
      {
        std::cout  << "    Track id: " << trks[i]->id()
                  << " - frame " << trks[i]->first_state()->time_.frame_number()
                  << " to " << trks[i]->last_state()->time_.frame_number()
                  << std::endl;
      }
#endif

      if ( at == 3 )
      {
        TEST( "Object 7 Frame 3 Frame Num", trks[0]->history()[3]->time_.frame_number(), 10 );
      }

      if ( at == 4 )
      {
        //std::cout << trks[0]->history().size() << "   " << trks[0]->history()[0]->loc_[0] << " " <<  (126.0+141.0)/2.0) << std::endl;
        TEST( "Object 0 Frame 0 X Pos", trks[0]->history()[0]->loc_[0], ( 126.0 + 141.0 ) / 2.0 );
        TEST( "Object 0 Frame 59 Y Pos", trks[0]->history()[59]->loc_[1], ( 116.025 + 129.025 ) / 2.0 );
        TEST( "Object 0 Frame 60 Frame Num", trks[0]->history()[60]->time_.frame_number(), 60 );
      }
      if ( at == 5 )
      {
        TEST( "Object 4 index", trks[0]->id(), 4u );
        TEST( "Object 5 Frame 1 x_min", trks[1]->history()[0]->amhi_bbox_.min_x(), 481 );
        TEST( "Object 5 Frame 1 x_max", trks[1]->history()[0]->amhi_bbox_.max_x(), 568 );
        TEST( "Object 5 Frame 1 y_min", trks[1]->history()[0]->amhi_bbox_.min_y(), 127 );
        TEST( "Object 5 Frame 1 y_max", trks[1]->history()[0]->amhi_bbox_.max_y(), 198 );
      }
      at += trp.tracks().size();
    }

    // --- active tracks tests ---

#if LOCAL_DEBUG
    if ( ! trp.active_tracks().empty() )
    {
      std::cout  << "There are " << trp.active_tracks().size()
                << " active tracks at step " << step_count_  << "\n";
      for ( size_t i = 0; i < trp.active_tracks().size(); i++ )
      {
        std::cout  << "    Active Track id: " << trp.active_tracks()[i]->id()
                  << " - frame " << trp.active_tracks()[i]->first_state()->time_.frame_number()
                  << " to " << trp.active_tracks()[i]->last_state()->time_.frame_number()
                  << std::endl;
      }
    }
#endif

    if ( 0 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 0, 4 );
    }
    else if ( 3 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 3, 5 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 0 );
      ttrks.push_back( 1 );
      ttrks.push_back( 3 );
      ttrks.push_back( 6 );
      ttrks.push_back( 2 );
      TEST( "Tracks present at step 3", verify_tracks( ttrks, trp.active_tracks() ), true );

    }
    else if ( 154 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 154, 3 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 2 );
      ttrks.push_back( 4 );
      ttrks.push_back( 5 );
      TEST( "Tracks present at step 154", verify_tracks( ttrks, trp.active_tracks() ), true );
    }
    else if ( 651 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 651, 1 );
    }

    step_count_++;
    if ( step_count_ <= 652 )
    {
      // Pretend that valid input to the process is being provided
      ts.set_frame_number( step_count_ );
      trp.set_timestamp( ts );
    }
  } // end while
  TEST( "652 Steps", step_count_, 654 );

  // ================================================================

  {
    TEST( "Reset", trp.reset(), true );
    config_block blk = trp.params();
    blk.set( "format", "" );
    blk.set( "filename", g_data_dir + "/kw18-test.trk" );
    blk.set( "ignore_occlusions", "true" );
    blk.set( "ignore_partial_occlusions", "true" );
    blk.set( "active_tracks_disable", "false" );

    TEST( "Set parameters", trp.set_params( blk ), true );
    TEST( "Initialize", trp.initialize(), true );
  }

  trks = trp.tracks();
  at = 0;
  size_t count = 0;
  step_count_ = 0;
  ts.set_frame_number( 0 );
  trp.set_timestamp( ts );

  while ( ( v = trp.step() ) )
  {
    trks = trp.tracks();
    count += trks.size();

    if ( ! trks.empty() )
    {

#if LOCAL_DEBUG
      std::cout  << "There are " << trks.size()
                << " terminated tracks at step " << step_count_  << "\n";
      for ( size_t i = 0; i < trks.size(); i++ )
      {
        std::cout  << "    Track id: " << trks[i]->id()
                  << " - frame " << trks[i]->first_state()->time_.frame_number()
                  << " to " << trks[i]->last_state()->time_.frame_number()
                  << std::endl;
      }
#endif

      if ( at == 3 )
      {
        TEST( "Track 3 ID is 800006", trks[0]->id(), 800006 );
      }

      if ( at == 8 )
      {
        TEST( "Track 8 has 33 frames", trks[0]->history().size(), 33 );
      }

      if ( at == 250 )
      {
        TEST( "Track 250 Frame 2 Loc X", trks[0]->history()[2]->loc_[0], 197.768 );
        TEST( "Track 250 Frame 2 Loc Y", trks[0]->history()[2]->loc_[1], 171.715 );
        TEST( "Track 250 Frame 2 Loc Z", trks[0]->history()[2]->loc_[2], 0.0 );

        TEST_NEAR( "Track 250 Frame 2 Vel X", trks[0]->history()[2]->vel_[0], 0.31962200000000002, 1e-6 );
        TEST_NEAR( "Track 250 Frame 2 Vel Y", trks[0]->history()[2]->vel_[1], 0.10968700000000001, 1e-6 );
        TEST_NEAR( "Track 250 Frame 2 Vel Z", trks[0]->history()[2]->vel_[2], 0.0, 1e-6 );

        TEST( "Track 250 Frame 2 min_x", trks[0]->history()[2]->amhi_bbox_.min_x(), 507 );
        TEST( "Track 250 Frame 2 min_y", trks[0]->history()[2]->amhi_bbox_.min_y(), 42 );
        TEST( "Track 250 Frame 2 max_x", trks[0]->history()[2]->amhi_bbox_.max_x(), 519 );
        TEST( "Track 250 Frame 2 max_y", trks[0]->history()[2]->amhi_bbox_.max_y(), 73 );
      }

      at += trks.size();

    }

    // --- active tracks tests ---

#if LOCAL_DEBUG
    if ( ! trp.active_tracks().empty() )
    {
      std::cout  << "There are " << trp.active_tracks().size()
                << " active tracks at step " << step_count_  << "\n";

      for ( size_t i = 0; i < trp.active_tracks().size(); i++ )
      {
        std::cout  << "    Active Track id: " << trp.active_tracks()[i]->id()
                  << " - frame " << trp.active_tracks()[i]->first_state()->time_.frame_number()
                  << " to " << trp.active_tracks()[i]->last_state()->time_.frame_number()
                  << std::endl;
      }
    }
#endif

    if ( 2 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 2, 0 );
    }
    else if ( 3 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 3, 3 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 800002 );
      ttrks.push_back( 800003 );
      ttrks.push_back( 800004 );
      TEST( "Tracks present at step 3", verify_tracks( ttrks, trp.active_tracks() ), true );
    }
    else if ( 37 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 37, 7 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 800002 );
      ttrks.push_back( 800003 );
      ttrks.push_back( 800004 );
      ttrks.push_back( 800005 );
      ttrks.push_back( 800006 );
      ttrks.push_back( 800007 );
      ttrks.push_back( 800008 );
      TEST( "Tracks present at step 37", verify_tracks( ttrks, trp.active_tracks() ), true );
    }
    else if ( 3016 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 3016, 8 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 800288 );
      ttrks.push_back( 800292 );
      ttrks.push_back( 800293 );
      ttrks.push_back( 800295 );
      ttrks.push_back( 800297 );
      ttrks.push_back( 800298 );
      ttrks.push_back( 800300 );
      ttrks.push_back( 800301 );
      TEST( "Tracks present at step 3016", verify_tracks( ttrks, trp.active_tracks() ), true );
    }

    step_count_++;
    if ( step_count_ <= 3018 )
    {
      // Pretend that valid input to the process is being provided
      ts.set_frame_number( step_count_ );
      trp.set_timestamp( ts );
    }
  } // end while

  TEST( "3019 Steps", step_count_, 3019 );

  TEST( "There are 301 Tracks", count, 301 );

  // ================================================================

  {
    TEST( "Reset", trp.reset(), true );
    config_block blk = trp.params();
    blk.set( "format", "viper" );
    blk.set( "filename", g_data_dir + "/viper-test.xml" );
    blk.set( "ignore_occlusions", "false" );
    blk.set( "ignore_partial_occlusions", "false" );
    blk.set( "active_tracks_disable", "false" );

    TEST( "Set parameters", trp.set_params( blk ), true );
    TEST( "Initialize", trp.initialize(), true );
  }

  at = 0;
  count = 0;
  step_count_ = 0;
  ts.set_frame_number( 0 );
  trp.set_timestamp( ts );
  while ( ( v = trp.step() ) )
  {
    trks = trp.tracks();
    count += trks.size();

    if ( ! trks.empty() )
    {

#if LOCAL_DEBUG
      std::cout  << "There are " << trks.size()
                << " terminated tracks at step " << step_count_  << "\n";
      for ( size_t i = 0; i < trks.size(); i++ )
      {
        std::cout  << "    Track id: " << trks[i]->id()
                  << " - frame " << trks[i]->first_state()->time_.frame_number()
                  << " to " << trks[i]->last_state()->time_.frame_number()
                  << std::endl;
      }
#endif

      if ( at == 8 )
      {
        TEST( "210 non occluded tracks", trks[0]->history().size(), 210 );
      }
      if ( at == 2 )
      {
        TEST( "42 non occluded tracks", trks[0]->history().size(), 42 );
        TEST( "Track 3 is id 30", trks[0]->id(), 30 );
        TEST( "Track 3 history 30 is framenum 339", trks[0]->history()[30]->time_.frame_number(), 339 );
        TEST( "Track 3 history 30 loc x", trks[0]->history()[30]->loc_[0], 598.0 );
        TEST( "Track 3 history 30 loc y", trks[0]->history()[30]->loc_[1], 437.0 );
        TEST( "Track 3 history 30 loc z", trks[0]->history()[30]->loc_[2], 0.0 );
        TEST( "Track 3 history 30 min pos x", trks[0]->history()[30]->amhi_bbox_.min_x(), 588 );
        TEST( "Track 3 history 30 min pos y", trks[0]->history()[30]->amhi_bbox_.min_y(), 426 );
        TEST( "Track 3 history 30 max pos x", trks[0]->history()[30]->amhi_bbox_.max_x(), 608 );
        TEST( "Track 3 history 30 max pos y", trks[0]->history()[30]->amhi_bbox_.max_y(), 448 );
      }
      if ( at == 25 )
      {
        TEST( "Track 35 is id 71", trks[0]->id() == 71, true );
      }
      at += trks.size();
    }

    // --- active tracks tests ---

#if LOCAL_DEBUG
    if ( ! trp.active_tracks().empty() )
    {
      std::cout  << "There are " << trp.active_tracks().size()
                << " active tracks at step " << step_count_  << "\n";
      for ( size_t i = 0; i < trp.active_tracks().size(); i++ )
      {
        std::cout  << "    Active Track id: " << trp.active_tracks()[i]->id()
                  << " - frame " << trp.active_tracks()[i]->first_state()->time_.frame_number()
                  << " to " << trp.active_tracks()[i]->last_state()->time_.frame_number()
                  << std::endl;
      }
    }
#endif

    if ( 60 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 60, 0 );
    }
    else if ( 61 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 61, 3 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 10 );
      ttrks.push_back( 150 );
      ttrks.push_back( 170 );
      TEST( "Tracks present at step 3", verify_tracks( ttrks, trp.active_tracks() ), true );
    }
    else if ( 170 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 170, 3 );
    }
    else if ( 171 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 171, 2 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 10 );
      ttrks.push_back( 150 );
      TEST( "Tracks present at step 171", verify_tracks( ttrks, trp.active_tracks() ), true );
    }
    else if ( 8993 == step_count_ )
    {
      TEST_ACTIVE_TRACK_COUNT( 8993, 3 );
      std::vector< unsigned > ttrks;
      ttrks.push_back( 100 );
      ttrks.push_back( 120 );
      ttrks.push_back( 140 );
      TEST( "Tracks present at step 8993", verify_tracks( ttrks, trp.active_tracks() ), true );
    }

    step_count_++;
    if ( step_count_ <= 8995 )
    {
      // Pretend that valid input to the process is being provided
      ts.set_frame_number( step_count_ );
      trp.set_timestamp( ts );
    }

  } // end while

  TEST( "8996 Steps", step_count_, 8996 );

  TEST( "Loaded 35 tracks", count, 35 ); // was 38  some have 0 history

  return testlib_test_summary();
} // test_track_reader_process
