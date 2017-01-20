/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <sstream>
#include <vector>
#include <vul/vul_temp_filename.h>
#include <vpl/vpl.h>
#include <testlib/testlib_test.h>

#include <utilities/large_file_ifstream.h>
#include <utilities/large_file_ofstream.h>

#include <boost/filesystem.hpp>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {


void
test_large_file()
{
  std::cout << "Test large file\n";

  std::string temp_filename = vul_temp_filename();
  std::cout << "Temp file is " << temp_filename << "\n";

  boost::filesystem::path file(temp_filename);
  std::cerr << "file: " << file << std::endl;

  boost::filesystem::path parent = file.parent_path();
  std::cerr << "parent: " << parent << std::endl;

  boost::filesystem::space_info space_inf = boost::filesystem::space(parent);
  std::cerr << "free space: " << space_inf.free << std::endl;

  // Add test for free space so we can get a failure message out to alert us. May as well make sure.
  // we have a little extra space to spare.
  TEST("Testing that we have enough free space to create the file", space_inf.free > vxl_int_64(4000)*1000*1000, true);

  // Wrap everything in a try catch so we don't exit this block abnormally.
  // We want to make sure we delete the very large tmp file at the end.
  try
  {
    // Compute 3000000000 because integer literals are likely 32-bit in the
    // compiler.
    vxl_int_64 large_location = vxl_int_64(3000)*1000*1000;

    // Write the large file
    {
      vidtk::large_file_ofstream f(temp_filename.c_str(), std::ios::binary);

      std::vector<char> content;
      content.resize( 1024*1024 );

      for( unsigned i = 0; i < 3000; ++i )
      {
        TEST("Writing to the file", !f.write(&content[0], content.size()), false);
        if (i%500==0)
        {
          std::cout << "Finished writing chunk " << i << "\n";
          TEST("File offset is good", f.tellp(), (i+1)*vxl_int_64(content.size()));
        }
      }

      f.seekp(1000);
      TEST("Seek to 1000", f.good(), true);
      f.write("hello", 5);
      TEST("Wrote hello", f.good(), true);

      f.seekp( large_location );
      TEST("Seek to 3000000000", f.good(), true);
      f.write("world", 5);
      TEST("Wrote world", f.good(), true);
    }

    // Read from the large file
    {
      vidtk::large_file_ifstream f(temp_filename.c_str(), std::ios::binary);

      TEST("Opened file", f.good(), true);
      if (!f)
      {
        // Try to delete the file before we exit
        vpl_unlink( temp_filename.c_str() );
        return;
      }

      char value[6] = "abcde";

      f.seekg(1000);
      TEST("Seek to 1000", f.good(), true);
      f.read(value, 5);
      TEST("Read 5 bytes", f.good(), true);
      TEST("Contents are correct", std::string("hello"), std::string(value));

      f.seekg( large_location );
      TEST("Seek to 3000000000", f.good(), true);
      f.read(value, 5);
      TEST("Read 5 bytes", f.good(), true);
      TEST("Contents are correct", std::string("world"), std::string(value));
      TEST("Stream position is correct", f.tellg(), large_location+5);
    }

    TEST( "Delete temp file", vpl_unlink( temp_filename.c_str() ), 0 );
  }
  catch (std::exception const& ex)
  {
    // Try to delete the file before we exit
    vpl_unlink( temp_filename.c_str() );
  }
}


} // end anonymous namespace

int test_large_file_fstream_with_large_file( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "large_file_fstream_with_large_file" );

  test_large_file();

  return testlib_test_summary();
}
