/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_temp_filename.h>
#include <vil/vil_image_view.h>
#include <vil/vil_crop.h>
#include <vpl/vpl.h>

#include <iostream>
#include <sstream>

#include <testlib/testlib_test.h>

#include <utilities/oc_timestamp_hack.h>
#include <utilities/timestamp_generator.h>
#include <utilities/timestamp_generator_process.h>
#include <utilities/transform_timestamp_process.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void
test_oc_timestamp_hack( std::string const& dir )
{
  {
    //file exists
    oc_timestamp_hack octh;
    TEST("Can load file", octh.load_file(dir+"/ocean_city_time_file_test.txt"), true);
    double tod;
    TEST("Can get time of day", octh.get_time_of_day(1, tod), true);
    TEST_NEAR("Correct time of day", tod, 400000000, 1e-6);
    TEST("Can get time of day", octh.get_time_of_day(2, tod), true);
    TEST_NEAR("Correct time of day", tod, 500000000, 1e-6);
    TEST("Fails with no valid time of day", octh.get_time_of_day(3, tod), false);

    timestamp ts(300,1);
    TEST("Can set timestamp", octh.set_timestamp(ts), true);
    TEST_NEAR("Correct time of day", ts.time(), 400000000, 1e-6);
    ts = timestamp(3,3);
    TEST("Fails set timestamp", octh.set_timestamp(ts), false);
  }
  {
    //dest bad file
    oc_timestamp_hack octh;
    TEST("Can load file", octh.load_file("DOES_NOT_EXIST.txt"), false);
  }
}

void
test_timestamp_generator( std::string const& dir )
{
  //test oc
  {
    timestamp_generator tgen;
    config_block blk = tgen.params();
    blk.set("filename", dir+"/ocean_city_time_file_test.txt");
    blk.set("method", "ocean_city");
    TEST("can set params for oc", tgen.set_params( blk ), true);
    TEST("can initialize for oc", tgen.initialize(), true);
    timestamp ts(300, 1);
    TEST("can provide ts for oc", tgen.provide_timestamp( ts ), process::SUCCESS);
    TEST_NEAR("Correct time of oc", ts.time(), 400000000, 1e-6);
  }
  //test disable 1 (NO change)
  {
    timestamp_generator tgen;
    config_block blk = tgen.params();
    blk.set("method", "disabled");
    TEST("can set params for disabled (no change)", tgen.set_params( blk ), true);
    TEST("can initialize for disabled (no change)", tgen.initialize(), true);
    timestamp ts(300, 1);
    TEST("can provide ts for disabled (no change)", tgen.provide_timestamp( ts ), process::SUCCESS);
    TEST_NEAR("Correct time for disabled (no change) ", ts.time(), 300, 1e-6);
    TEST_EQUAL("Correct frame number for disable (no change)", ts.frame_number(), 1);
  }
  //test disable 2 (Filtering)
  {
    timestamp_generator tgen;
    config_block blk = tgen.params();
    blk.set("method", "disabled");
    blk.set("skip_frames", "2");
    blk.set("start_frame", "30");
    blk.set("end_frame", "60");
    TEST("can set params for disabled (Filtering)", tgen.set_params( blk ), true);
    TEST("can initialize for disabled (Filtering)", tgen.initialize(), true);
    timestamp ts(300, 1);
    TEST("skips for frames before start for disabled (Filtering)", tgen.provide_timestamp( ts ), process::SKIP);
    ts = timestamp(320, 30);
    TEST("can provide ts for disabled (Filtering)", tgen.provide_timestamp( ts ), process::SUCCESS);
    TEST_NEAR("Correct time for disabled (Filtering)", ts.time(), 320, 1e-6);
    TEST_EQUAL("Correct frame number for disable (Filtering)", ts.frame_number(), 30);
    ts = timestamp(323, 32);
    TEST("skips for frame not divisable by 3 for disabled (Filtering)", tgen.provide_timestamp( ts ), process::SKIP);
    ts = timestamp(330, 61);
    TEST("fails for frames after end for disabled (Filtering)", tgen.provide_timestamp( ts ), process::FAILURE);
  }
  //test disable 2 (Change time)
  {
    timestamp_generator tgen;
    config_block blk = tgen.params();
    blk.set("method", "disabled");
    blk.set("manual_frame_rate", "2");
    blk.set("base_time", "1000");
    TEST("can set params for disabled (Change time)", tgen.set_params( blk ), true);
    TEST("can initialize for disabled (Change time)", tgen.initialize(), true);
    timestamp ts(200.0);
    TEST("Handles no frame properly (Change time)", tgen.provide_timestamp( ts ), process::FAILURE);
    ts = timestamp(320, 30);
    TEST("can provide ts for disabled (Change time)", tgen.provide_timestamp( ts ), process::SUCCESS);
    TEST_NEAR("Correct time for disabled (Change time)", ts.time(), (1015)*1e6, 1e-6);
    TEST_EQUAL("Correct frame number for disable (Change time)", ts.frame_number(), 30);
  }
  //test update_frame_number
  {
    timestamp_generator tgen;
    config_block blk = tgen.params();
    blk.set("method", "update_frame_number");
    blk.set("skip_frames", "2");
    blk.set("start_frame", "30");
    blk.set("end_frame", "60");
    TEST("can set params for update_frame_number", tgen.set_params( blk ), true);
    TEST("can initialize for update_frame_number", tgen.initialize(), true);
    for( unsigned int i = 0; i < 40; ++i)
    {
      timestamp ts(i, i);
      if(i + 30 > 60)
      {
        TEST("step for update_frame_number (FAIL)", tgen.provide_timestamp( ts ), process::FAILURE);
        continue;
      }
      if((i+30)%3)
      {
        TEST("step for update_frame_number (SKIP)", tgen.provide_timestamp( ts ), process::SKIP);
        continue;
      }
      TEST("step for update_frame_number (SUCCESS)", tgen.provide_timestamp( ts ), process::SUCCESS);
      TEST_NEAR("Correct time for update_frame_number ", ts.time(), i, 1e-6);
      TEST_EQUAL("Correct frame number for update_frame_number", ts.frame_number(), i+30);
    }
  }
  //test file
  {
    //lair-frame-time-test.txt
    timestamp_generator tgen;
    config_block blk = tgen.params();
    blk.set("method", "frame_timestamp_pair_file");
    TEST("can config for pair file", tgen.set_params( blk ), true);
    TEST("fails to init for pair file", tgen.initialize(), false);
    blk.set("filename", "DOES_NOT_EXIST.txt");
    TEST("can config for pair file", tgen.set_params( blk ), true);
    TEST("fails to init for pair file", tgen.initialize(), false);
    blk.set("filename", dir+"/lair-frame-time-test.txt");
    TEST("can config for pair file", tgen.set_params( blk ), true);
    TEST("can init for pair file", tgen.initialize(), true);
    timestamp ts;
    TEST("step for pair file", tgen.provide_timestamp( ts ), process::SUCCESS);
    TEST_NEAR("Correct time for update_frame_number ", ts.time(), 1256157126875000, 1e-6);
    TEST_EQUAL("Correct frame number for update_frame_number", ts.frame_number(), 612 );
    TEST("step for pair file", tgen.provide_timestamp( ts ), process::SUCCESS);
    TEST_NEAR("Correct time for update_frame_number ", ts.time(), 1256157127675000, 1e-6);
    TEST_EQUAL("Correct frame number for update_frame_number", ts.frame_number(), 613 );
    unsigned int i = 2;
    for(; i < 512; ++i)
    {
      TEST("step for pair file", tgen.provide_timestamp( ts ), process::SUCCESS);
    }
    TEST("step for pair file", tgen.provide_timestamp( ts ), process::SUCCESS);
    TEST_NEAR("Correct time for update_frame_number ", ts.time(), 1256157537203000, 1e-6);
    TEST_EQUAL("Correct frame number for update_frame_number", ts.frame_number(), 1124 );
    TEST("step for end pair file", tgen.provide_timestamp( ts ), process::FAILURE);
    TEST("step for end pair file", tgen.provide_timestamp( ts ), process::FAILURE);
  }
}

void
test_generator_process( std::string const& dir )
{
  {
    //test bad config
    timestamp_generator_process tgp("timestamper");
    config_block blk = tgp.params();
    blk.set("method", "BOB");
    TEST("Handle bad config value (not valid a method)", tgp.set_params( blk ), false);
  }
  {
    timestamp_generator_process tgp("timestamper");
    config_block blk = tgp.params();
    blk.set("method", "frame_timestamp_pair_file");
    blk.set("filename", dir+"/lair-frame-time-test.txt");
    TEST("can config", tgp.set_params( blk ), true);
    TEST("can init", tgp.initialize(), true);
    TEST("can step", tgp.step2( ), process::SUCCESS);
    TEST_NEAR("Correct time for update_frame_number ", tgp.timestamp().time(), 1256157126875000, 1e-6);
    TEST_EQUAL("Correct frame number for update_frame_number", tgp.timestamp().frame_number(), 612 );
  }
}

void
test_transform_process( )
{
  {
    //test bad config
    transform_timestamp_process<vxl_byte> ttp("timestamper");
    config_block blk = ttp.params();
    blk.set("method", "BOB");
    TEST("Handle bad config value (not valid a method)", ttp.set_params( blk ), false);
  }
  {
    transform_timestamp_process<vxl_byte> ttp("timestamper");
    config_block blk = ttp.params();
    blk.set("method", "disabled");
    blk.set("end_frame", "60");
    TEST("can config", ttp.set_params( blk ), true);
    TEST("can init", ttp.initialize(), true);
    timestamp ts(300, 1);
    video_metadata vm;
    vil_image_view<vxl_byte> img(200, 300);
    ttp.set_source_timestamp(ts);
    ttp.set_source_image(img);
    ttp.set_source_metadata(vm);
    TEST("can step", ttp.step2( ), process::SUCCESS);
    TEST_NEAR("Correct time", ttp.timestamp().time(), 300, 1e-6);
    TEST_EQUAL("Correct frame number", ttp.timestamp().frame_number(), 1 );
    TEST_EQUAL("Current number of video metadata", ttp.metadata_vec().size(), 1);
    TEST_EQUAL("Current image ni", ttp.image().ni(), 200);
    TEST_EQUAL("Current image nj", ttp.image().nj(), 300);
    ts = timestamp(320, 30);
    ttp.set_source_timestamp(ts);
    ttp.set_source_image(img);
    ttp.set_source_metadata(vm);
    TEST("can step", ttp.step2( ), process::SUCCESS);
    TEST_NEAR("Correct time", ttp.timestamp().time(), 320, 1e-6);
    TEST_EQUAL("Correct frame number", ttp.timestamp().frame_number(), 30 );
    TEST_EQUAL("Current number of video metadata", ttp.metadata_vec().size(), 1);
    TEST_EQUAL("Current image ni", ttp.image().ni(), 200);
    TEST_EQUAL("Current image nj", ttp.image().nj(), 300);
    TEST("Test not setting ts", ttp.step2( ), process::FAILURE);
    ts = timestamp(310, 15);
    ttp.set_source_timestamp(ts);
    TEST("Test going back into time", ttp.step2( ), process::FAILURE);
    ts = timestamp(340, 61);
    ttp.set_source_timestamp(ts);
    TEST("Outside range", ttp.step2( ), process::FAILURE);
  }
}


} // end anonymous namespace

int test_timestamp_generation( int argc, char* argv[] )
{
  testlib_test_start( "kwa_index" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    std::string data_dir( argv[1] );
    test_oc_timestamp_hack( data_dir );
    test_timestamp_generator( data_dir );
    test_generator_process( data_dir );
    test_transform_process( );
  }

  return testlib_test_summary();
}
