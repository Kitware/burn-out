/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <string>
#include <vul/vul_file.h>

#include <pipeline_framework/sync_pipeline.h>
#include <process_framework/process.h>
#include <utilities/config_block.h>
#include <utilities/kw_archive_index_reader.h>
#include <utilities/kw_archive_writer_process.h>

// Use anonymous namespace so that different tests won't conflict.
namespace
{

// ----------------------------------------------------------------------
// Test uncompressed
void test_writer(int argc, char *argv[], bool compressed=false)
{

  // Get inputs
  TEST("Input arguments provided", argc >= 1, true);
  std::string data_dir(argv[1]);
  std::cout << "Using data_dir: " << data_dir << std::endl;

  // Delete existing kwa files (if any)
  vul_file::delete_file_glob("test_kwa_*compressed.*");

   vidtk::kw_archive_writer_process<vxl_byte> *kwa_writer =
     new vidtk::kw_archive_writer_process<vxl_byte>("kwa_writer");
   vidtk::config_block blk = kwa_writer->params();
   blk.set_value("disabled", false);
   blk.set_value("compress_image", compressed);
   std::string base_filename = compressed ? "test_kwa_compressed" : "test_kwa_uncompressed";
   blk.set_value("base_filename", base_filename);
   kwa_writer->set_params(blk);
   kwa_writer->initialize();

  // Instantiate frame reader
  vidtk::kw_archive_index_reader *kwa_reader = new vidtk::kw_archive_index_reader();
  std::string path = data_dir + "/kwa_compressed.index";
  bool ok = kwa_reader->open(path);
  TEST("Open input file", ok, true);

  vidtk::image_to_utm_homography frame_to_ref;
  frame_to_ref.set_valid(true);

   while (kwa_reader->read_next_frame())
   {
     std::cout << kwa_reader->timestamp() << std::endl;

     kwa_writer->set_input_timestamp(kwa_reader->timestamp());
     kwa_writer->set_input_image(kwa_reader->image());
     kwa_writer->set_input_src_to_ref_homography(kwa_reader->frame_to_reference());
     kwa_writer->set_input_corner_points(kwa_reader->corner_points());
     kwa_writer->set_input_world_units_per_pixel(kwa_reader->gsd());

     TEST("Step", kwa_writer->step(), true);
   }
   delete kwa_writer;
   delete kwa_reader;

   // Check that .data file created
   std::string data_filename = base_filename + ".data";
   unsigned long datafile_size = vul_file::size(data_filename);
   std::cout << "Output " << data_filename << " size " << datafile_size << std::endl;
   TEST("kwa .data file generated", datafile_size > 0, true);
}

}  // namespace

int test_kwa_writer_process(int argc, char *argv[])
{
  test_writer(argc, argv, false);  // uncompressed image out
  test_writer(argc, argv, true);   // compressed image out
  return testlib_test_summary();
}
