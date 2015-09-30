/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vxl_config.h>
#include <testlib/testlib_test.h>

#include <tracking/mit_layer_annotation_reader.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

vcl_string g_data_dir;

}

using namespace vidtk;
int test_mit_reader( int argc, char* argv[])
{
  testlib_test_start( "mit_reader");
  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }
  else
  {
    g_data_dir = argv[1];
    vcl_vector<vidtk::track_sptr> trks;
    mit_layer_annotation_reader reader = mit_layer_annotation_reader();
    reader.set_filename((g_data_dir+"/mit_layer_annotation_test_tracks.xml").c_str());
    TEST("Reading XML File",reader.read(trks),true);
    TEST("Object 0 Frame 0 X Pos",trks[0]->history()[0]->loc_[0], (126.0+141.0)/2.0);
    TEST("Object 0 Frame 59 Y Pos",trks[0]->history()[59]->loc_[1], (116.025+129.025)/2.0);
    TEST("Object 0 Frame 60 Frame Num",trks[0]->history()[60]->time_.frame_number(), 60);
    TEST("Object 7 Frame 3 Frame Num",trks[7]->history()[3]->time_.frame_number(), 10);
    TEST("Object 5 Frame 1 x_min", trks[5]->history()[0]->amhi_bbox_.min_x(),481);
    TEST("Object 5 Frame 1 x_max", trks[5]->history()[0]->amhi_bbox_.max_x(),568);
    TEST("Object 5 Frame 1 y_min", trks[5]->history()[0]->amhi_bbox_.min_y(),127);
    TEST("Object 5 Frame 1 y_max", trks[5]->history()[0]->amhi_bbox_.max_y(),198);
    TEST("Object 4 index", trks[4]->id(), (unsigned int) 4);
    reader.set_filename((g_data_dir+"/mit-occluder-test-objects.xml").c_str());
    reader.set_occluder_filename((g_data_dir+"/mit-occluder-test.xml").c_str());
    reader.set_ignore_occlusions(false);
    TEST("Reading XML File with occlusions",reader.read(trks),true);
    TEST("Object 1 has 4 tracks",trks[0]->history().size() == 4,true);
    TEST("Object 2 has 0 tracks",trks[1]->history().size() == 0,true);
    TEST("Object 3 has 9 tracks",trks[2]->history().size() == 9,true);
  }
  return testlib_test_summary();
}

