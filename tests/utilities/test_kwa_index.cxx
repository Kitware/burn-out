/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <sstream>
#include <vul/vul_temp_filename.h>
#include <vil/vil_image_view.h>
#include <vil/vil_crop.h>
#include <vpl/vpl.h>
#include <testlib/testlib_test.h>

#include <utilities/kw_archive_index_reader.h>
#include <utilities/kw_archive_index_writer.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
test_read(std::string const& index_name,
          unsigned frame_count)
{
  vidtk::kw_archive_index_reader reader;

  std::cout << "Test reading " << index_name << "\n";
  TEST("Open file", reader.open(index_name), true);
  TEST("Mission id", reader.mission_id(), "");
  TEST("Stream id", reader.stream_id(), "/data/virat/video/aphill/09172008flight1tape1_5.mpg");

  TEST("Not EOF", reader.eof(), false);
  for (unsigned count=0; count < frame_count; ++count)
  {
    std::cout << "Frame " << count << "\n";
    TEST("Read", reader.read_next_frame(), true);
    TEST("Frame number", reader.timestamp().frame_number(), count+1);
    std::cout << "ts="<<reader.timestamp()<<"\n";
  }
  TEST("Read fails", reader.read_next_frame(), false);
  TEST("EOF", reader.eof(), true);
}


void
test_write_and_read(bool separate_meta,
                    bool compress_image)
{
  std::cout << "Test writing then reading with separate_meta="
           << separate_meta << ", compress_image="
           << compress_image << "\n";
  std::string temp_filename = vul_temp_filename();
  std::cout << "Temp file is " << temp_filename << "\n";

  {
    vidtk::kw_archive_index_writer <vxl_byte> writer;

    TEST("Open file for writing",
         writer.open(vidtk::kw_archive_index_writer<vxl_byte>::open_parameters()
                     .set_base_filename(temp_filename)
                     .set_separate_meta(separate_meta)
                     .set_compress_image(compress_image)),
         true);

    for(unsigned count=0; count < 3; ++count) {
      vidtk::timestamp ts(3000.0*count, count);
      vidtk::video_metadata meta;
      meta.corner_ul(vidtk::geo_coord::geo_lat_lon(count, count+1));
      meta.corner_ur(vidtk::geo_coord::geo_lat_lon(count+2, count+3));
      meta.corner_ll(vidtk::geo_coord::geo_lat_lon(count+4, count+5));
      meta.corner_lr(vidtk::geo_coord::geo_lat_lon(count+6, count+7));
      vidtk::image_to_image_homography homog;
      homog.set_source_reference(ts);
      vil_image_view<vxl_byte> image(15,15);
      image.fill(0);
      vil_crop(image, 7, 5, 7, 5).fill(60);
      std::cout << "Image pixel(0,0)=" << int(image(0,0)) << "\n";
      std::cout << "Image pixel(10,10)=" << int(image(10,10)) << "\n";
      TEST("Write frame",
           writer.write_frame(ts, meta, image, homog, double(count)),
           true);
    }

    // test multiple closes - old bug - caused seg fault
    // Note that the DTOR also does a close, so we are doing three closes.
    writer.close();
    writer.close();
  }

  {
    vidtk::kw_archive_index_reader reader;

    TEST("Open file for reading",
         reader.open(temp_filename+".index"),
         true);

    for(unsigned count=0; count < 3; ++count) {
      TEST("Read frame", reader.read_next_frame(), true);
      TEST("Frame number matches", reader.timestamp().frame_number(), count);
      TEST("Timestamp matches", reader.timestamp().time(), 3000.0*count);
      TEST_NEAR("Image pixel 1 matches", reader.image()(0,0), 0, 2);
      TEST_NEAR("Image pixel 2 matches", reader.image()(10,10), 60, 2);
    }
  }

  TEST( "Delete temp index file", vpl_unlink( (temp_filename+".index").c_str() ), 0 );
  TEST( "Delete temp data file", vpl_unlink( (temp_filename+".data").c_str() ), 0 );
  if (separate_meta)
  {
    TEST( "Delete temp meta file", vpl_unlink( (temp_filename+".meta").c_str() ), 0 );
  }
}



} // end anonymous namespace

int test_kwa_index( int argc, char* argv[] )
{
  testlib_test_start( "kwa_index" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    std::string data_dir( argv[1] );

    test_read( /*kwa=*/ data_dir+"/kwa_uncompressed.index",
               /*frame_count=*/ 1 );
    test_read( /*kwa=*/ data_dir+"/kwa_compressed.index",
               /*frame_count=*/ 5 );
    test_write_and_read(/*separate_meta=*/ false,
                        /*compress_image=*/ false);
    test_write_and_read(/*separate_meta=*/ true,
                        /*compress_image=*/ false);
    test_write_and_read(/*separate_meta=*/ false,
                        /*compress_image=*/ true);
  }

  return testlib_test_summary();
}
