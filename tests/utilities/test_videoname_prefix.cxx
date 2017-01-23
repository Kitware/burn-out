/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include <iostream>
#include <testlib/testlib_test.h>

#include <utilities/videoname_prefix.h>
#include <utilities/config_block.h>


int test_videoname_prefix( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "videoname prefix" );

  vidtk::videoname_prefix* shingle = vidtk::videoname_prefix::instance();

  std::string name = vidtk::videoname_prefix::instance()->get_videoname_prefix();
  TEST( "Video name initial state", name.size(), 0 );

  vidtk::videoname_prefix::instance()->set_videoname_prefix( "/server/direct/mode/video-name.jpg" );
  name = vidtk::videoname_prefix::instance()->get_videoname_prefix();
  TEST( "Video name being set", name, "video-name" );

  vidtk::config_block blk;
  blk.add_parameter( "output_file", "file name to mess with" );
  blk.set_value( "output_file", "image.png" );

  vidtk::videoname_prefix::instance()->add_videoname_prefix( blk, "output_file" );
  std::string new_name = blk.get< std::string > ( "output_file" );
  TEST( "video prefix applied", new_name, "./video-name_image.png" );

  TEST( "Only one instance", vidtk::videoname_prefix::instance(), shingle );

  return testlib_test_summary();
}
