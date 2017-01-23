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

#include <track_oracle/track_kwxml/track_kwxml.h>
#include <track_oracle/track_kwxml/file_format_kwxml.h>

#include <testlib/testlib_test.h>

#include <logger/logger.h>


using std::cout;
using std::ostringstream;
using std::string;

using namespace vidtk;

VIDTK_LOGGER("test_track_kwxml");

int test_track_kwxml( int argc, char *argv[])
{
  if(argc < 2)
    {
      LOG_ERROR("Need the data directory as argument\n");
      return EXIT_FAILURE;
    }

  testlib_test_start( "test_track_kwxml");

  vidtk::track_kwxml_type kwxml;
  string dir = argv[1];
  string single_track_file(dir + "/kwxml_file_test.xml" );

  {
    vidtk::track_handle_list_type tracks;
    vidtk::file_format_kwxml kwxml_reader;
    kwxml_reader.options().set_track_style_filter( "trackDescriptorTEST_NOT_ICSI" );
    kwxml_reader.read( single_track_file.c_str(), tracks );

    TEST("Loaded TEST_NOT_ICSI tracks from kwxml file", (tracks.size() >= 1), true );

    {
      ostringstream oss;
      oss << "Track size " << tracks.size() << " is 1: ";
      TEST( oss.str().c_str(), (tracks.size() == 1), true );
    }

  }

  {
    vidtk::track_handle_list_type tracks;
    vidtk::file_format_kwxml kwxml_reader;
    kwxml_reader.options().set_track_style_filter( "" ); // default
    kwxml_reader.read( single_track_file.c_str(), tracks );

    TEST("Loaded all tracks from kwxml file", (tracks.size() >= 1), true );

    {
      ostringstream oss;
      oss << "Track size " << tracks.size() << " is 689: ";
      TEST( oss.str().c_str(), (tracks.size() == 689), true );
    }

    cout << "Track 1: \n";

    {
      ostringstream oss;
      oss << "Time Stamp " << kwxml(tracks[0]).time_stamp() << " is 2011-Jul-06 23:55:38.272893: ";
      TEST( oss.str().c_str(), (kwxml(tracks[0]).time_stamp() == "2011-Jul-06 23:55:38.272893"), true );
    }

    const float epsilon = 0.10f;

    {
      ostringstream oss;
      oss << "Descriptor icsi_hog first value " << kwxml(tracks[0]).descriptor_icsihog()[0] << " is 0.00109302: ";
      TEST( oss.str().c_str(), (fabs((0.00109302 - kwxml(tracks[0]).descriptor_icsihog()[0]) / 0.00109302) < epsilon), true );
    }

    {
      ostringstream oss;
      oss << "Descriptor metadata gsd " << kwxml(tracks[0]).descriptor_metadata().gsd << " is 0.0449088: ";
      TEST( oss.str().c_str(), (fabs((0.0449088 - kwxml(tracks[0]).descriptor_metadata().gsd) / 0.0449088) < epsilon), true );
    }

    cout << "Frame 1: \n";

    frame_handle_list_type frames = track_oracle::get_frames( tracks[0] );

    {
      ostringstream oss;
      oss << "Frame Number:  " << kwxml[frames[0]].frame_number() << " is 42: ";
      TEST( oss.str().c_str(), (kwxml[frames[0]].frame_number() == 42), true );
    }

    {
      ostringstream oss;
      oss << "Time Stamp Usecs:  " << kwxml[frames[0]].timestamp_usecs() << " is 1221509709420120832: ";
      TEST( oss.str().c_str(), (kwxml[frames[0]].timestamp_usecs() == 1221509709420120832), true );
    }
  }

  return testlib_test_summary();
}
