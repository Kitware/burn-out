/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/video_metadata_util.h>
#include <utilities/filter_video_metadata_process.h>
#include <utilities/config_block.h>
#include <utilities/video_metadata_writer_process.h>
#include <utilities/write_videometa_kml.h>

#include <algorithm>
#include <string>
#include <cstdio>
#include <fstream>

namespace //anon
{

using namespace vidtk;

void read_metadata(std::string fname, timestamp & ts, video_metadata & vm)
{
  std::ifstream in;
  in.open(fname.c_str());
  unsigned long frame_number;
  vxl_uint_64 timecode;
  in >> frame_number;
  in >> timecode;
  ascii_deserialize(in, vm);
  ts.set_frame_number(frame_number);
  double t = static_cast< double>( timecode );
  ts.set_time(t);
}

void test_config()
{
  video_metadata_writer_process vmwp("test");
  config_block blk = vmwp.params();
  blk.set("type", "vidtk");
  TEST("can set type to vidtk", vmwp.set_params(blk), true);
  blk.set("type", "kml");
  TEST("can set type to kml", vmwp.set_params(blk), true);
  blk.set("type", "BAD");
  TEST("fails to set an invalid type", vmwp.set_params(blk), false);
}

std::string fnames[] = { "lair_00626.vidtk_meta", "lair_00627.vidtk_meta", "lair_00628.vidtk_meta", "lair_00629.vidtk_meta" };

void test_kml(std::string directory)
{
  //test bad file
  {
    write_videometa_kml meta_writer;
    TEST("Fail to open a kml file", meta_writer.open("DIRECTORY_DOES_NOT_EXIST/test.kml"), false);
  }
  //write files
  {
    video_metadata vm;
    timestamp ts;
    write_videometa_kml meta_writer;
    meta_writer.open("test.kml");
    TEST("is open", meta_writer.is_open(), true);
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      TEST("can write vm", meta_writer.write_corner_point(ts, vm), true);
    }
  }
  //test write without fov
  {
    video_metadata vm;
    timestamp ts;
    write_videometa_kml meta_writer;
    meta_writer.open("test.kml");
    TEST("is open", meta_writer.is_open(), true);
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      TEST("can write vm", meta_writer.write_comp_transform_points(ts, 100, 100, vm), true);
    }
  }
  //Test empty vm with corner writer
  {
    video_metadata vm;
    timestamp ts;
    write_videometa_kml meta_writer;
    meta_writer.open("test.kml");
    TEST("is open", meta_writer.is_open(), true);
    TEST("cannot vm", meta_writer.write_corner_point(ts, vm), false);
    TEST("cannot vm", meta_writer.write_comp_transform_points(ts, 100, 100, vm), false);
  }
}

void test_writer_process(std::string directory)
{
  {
    video_metadata_writer_process vmwp("test");
    config_block blk = vmwp.params();
    blk.set("disabled", "true");
    TEST("can set disable", vmwp.set_params(blk), true);
    TEST("can init disable", vmwp.initialize(), true);
    TEST("can step disable", vmwp.step(), true);
  }
  {
    video_metadata_writer_process vmwp("test");
    config_block blk = vmwp.params();
    blk.set("type", "kml");
    blk.set("write_given_corner_points", "false");
    blk.set("kml_fname", "DIRECTORY_DOES_NOT_EXIST/test.kml");
    TEST("can set kml", vmwp.set_params(blk), true);
    TEST("init fails because of bad filename", vmwp.initialize(), false);
    video_metadata vm;
    timestamp ts;
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      vmwp.set_meta(vm);
      vmwp.set_timestamp(ts);
      std::cout << vm << std::endl;
      TEST("cannot write vm", vmwp.step(), false);
    }

    blk.set("kml_fname", "test.kml");
    TEST("can set kml", vmwp.set_params(blk), true);
    TEST("init passes", vmwp.initialize(), true);
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      vmwp.set_meta(vm);
      vmwp.set_timestamp(ts);
      std::cout << vm << std::endl;
      TEST("can write vm", vmwp.step(), true);
    }
    TEST("Resets and handles invalid vm", vmwp.step(), true);
  }
  {
    video_metadata_writer_process vmwp("test");
    config_block blk = vmwp.params();
    blk.set("type", "kml");
    blk.set("write_given_corner_points", "true");
    blk.set("kml_fname", "DIRECTORY_DOES_NOT_EXIST/test.kml");
    TEST("can set kml", vmwp.set_params(blk), true);
    TEST("init fails because of bad filename", vmwp.initialize(), false);
    video_metadata vm;
    timestamp ts;
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      vmwp.set_meta(vm);
      vmwp.set_timestamp(ts);
      TEST("cannot write vm", vmwp.step(), false);
    }

    blk.set("kml_fname", "test.kml");
    TEST("can set kml", vmwp.set_params(blk), true);
    TEST("init passes", vmwp.initialize(), true);
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      vmwp.set_meta(vm);
      vmwp.set_timestamp(ts);
      TEST("can write vm", vmwp.step(), true);
    }
    TEST("Resets and handles invalid vm", vmwp.step(), true);
  }
  {
    video_metadata_writer_process vmwp("test");
    config_block blk = vmwp.params();
    blk.set("type", "vidtk");
    TEST("can set kml", vmwp.set_params(blk), true);
    TEST("can init", vmwp.initialize(), true);
    video_metadata vm;
    timestamp ts;
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      vmwp.set_meta(vm);
      vmwp.set_timestamp(ts);
      TEST("can write vm", vmwp.step(), true);
    }
    {
      video_metadata vm1, vm2;
      timestamp ts1, ts2;
      read_metadata(directory+"/"+fnames[0], ts1, vm1);
      read_metadata("out0626.vidtk_meta", ts2, vm2);
      TEST("read and write are the same", vm1, vm2);
      TEST("read and write are the same", ts1, ts2);
      read_metadata(directory+"/"+fnames[1], ts1, vm1);
      read_metadata("out0627.vidtk_meta", ts2, vm2);
      TEST("read and write are the same", vm1, vm2);
      TEST("read and write are the same", ts1, ts2);
      read_metadata(directory+"/"+fnames[2], ts1, vm1);
      read_metadata("out0628.vidtk_meta", ts2, vm2);
      TEST("read and write are the same", vm1, vm2);
      TEST("read and write are the same", ts1, ts2);
      read_metadata(directory+"/"+fnames[3], ts1, vm1);
      read_metadata("out0629.vidtk_meta", ts2, vm2);
      TEST("read and write are the same", vm1, vm2);
      TEST("read and write are the same", ts1, ts2);
    }
    TEST("Resets and handles invalid vm", vmwp.step(), true);
  }
  {
    video_metadata_writer_process vmwp("test");
    config_block blk = vmwp.params();
    blk.set("type", "vidtk");
    blk.set("pattern", "DIRECTORY_DOES_NOT_EXIST/out%2$04d.vidtk_meta");
    TEST("can set kml", vmwp.set_params(blk), true);
    TEST("can init", vmwp.initialize(), true);
    video_metadata vm;
    timestamp ts;
    for(size_t i = 0; i < 4; ++i)
    {
      read_metadata(directory+"/"+fnames[i], ts, vm);
      vmwp.set_meta(vm);
      vmwp.set_timestamp(ts);
      TEST("cannot write vm", vmwp.step(), false);
    }
  }

  {
    video_metadata vm;
    TEST( "VM initial valid", vm.is_valid(), true );
    TEST( "VM initial invalid corners", vm.has_valid_corners(), false );

    timestamp ts;
    read_metadata(directory+"/"+fnames[0], ts, vm);
    TEST( "VM valid", vm.is_valid(), true );
    TEST( "VM valid corners", vm.has_valid_corners(), true );

    vm.corner_ul( vm.corner_ll() );
    TEST( "VM invalid corners", vm.has_valid_corners(), false );
  }


}

}

int test_video_metadata_writing( int argc, char* argv[] )
{
  testlib_test_start( "Testing writing video metadata" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    test_config();
    test_kml(argv[1]);
    test_writer_process(argv[1]);
  }

  return testlib_test_summary();
}
