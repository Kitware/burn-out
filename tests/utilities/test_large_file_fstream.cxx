/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <sstream>
#include <vul/vul_temp_filename.h>
#include <vpl/vpl.h>
#include <testlib/testlib_test.h>

#include <utilities/large_file_ifstream.h>
#include <utilities/large_file_ofstream.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
test_sequential_text_read_to_end(std::string const& data_dir)
{
  std::cout << "Test sequential text read to end\n";

  vidtk::large_file_ifstream f((data_dir+"/small_file.txt").c_str());
  TEST("File opened", f.good(), true);

  for (int i=1; i<=5; ++i)
  {
    std::ostringstream s;
    s << "Line " << i;
    std::string line;
    TEST("Read line", !!std::getline( f, line ), true);
    std::cout << "Expecting \"" << s.str() << "\", read \""
             << line << "\"\n";
    TEST("Good", line==s.str(), true);
  }
  TEST("At EOF", f.eof(), true);
}

void
test_sequential_binary_read_to_end(std::string const& data_dir)
{
  std::cout << "Test sequential binary read to end\n";

  vidtk::large_file_ifstream f((data_dir+"/small_file.dat").c_str(), std::ios::in|std::ios::binary);
  TEST("File opened", f.good(), true);

  for (int i=1; i<=6; ++i)
  {
  char c;
    f >> c;
    std::cout << "Expecting " << i << " read " << int(c);
    TEST("Good", int(c)==i && !f.bad(), true);
  }
  TEST("Not at EOF", f.eof(), false);
  f.peek();
  TEST("At EOF after a peek", f.eof(), true);
}


void
test_seek_binary_read_small(std::string const& data_dir)
{
  std::cout << "Test seeking in a small file\n";

  vidtk::large_file_ifstream f((data_dir+"/small_file.dat").c_str(), std::ios::in|std::ios::binary);
  TEST("File opened", f.good(), true);

  vxl_int_64 positions[] = { 1, 5, 3, 1, 4, 0, 2, 0, -1 };
  for (vxl_int_64* pos=positions; *pos != -1; ++pos)
  {
    std::cout << "Seeking to " << *pos << "\n";
    f.seekg( *pos );
    char c;
    f >> c;
    TEST( "Found expected character", int(c)==*pos+1 && !f.bad(), true);
  }
  TEST("Not at EOF", f.eof(), false);
  f.peek();
  TEST("Not at EOF after a peek", f.eof(), false);

  f.seekg( vxl_int_64(9) );
  f.peek();
  TEST("Peek after an out-of-bounds seek fails", f.good(), false);
  if (f.good())
  {
    std::cout << "After out-of-bounds seek, next char is " << f.get() << "\n";
  }

  f.clear();
  f.seekg( vxl_int_64(1) );
  TEST("Subsequent in-bounds seek is fine", f.good(), true);
  TEST("and points to the right data", f.get(), 2);
}


void
test_sequential_write(bool binary)
{
  std::cout << "Test sequential write, no truncation (binary=" << binary << "\n";

  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode(0);

  std::string temp_filename = vul_temp_filename();
  std::cout << "Temp file is " << temp_filename << "\n";

  vidtk::large_file_ofstream f(temp_filename.c_str(), std::ios::out|mode);
  TEST("File opened for writing", f.good(), true);

  for (int i=1; i<=6; ++i)
  {
    std::ostringstream ss;
    ss << "Line " << i;
    f << ss.str() << "\n";
  }
  f.close();
  TEST("Wrote a set of lines", f.good(), true);

  std::ifstream fin(temp_filename.c_str(), std::ios::in|mode);
  TEST("File re-opened for reading", fin.good(), true);
  std::string line, lineprev;
  while (std::getline(fin,line))
  {
    lineprev = line;
  }
  TEST("Last line matches", line, "");
  TEST("Next-to-last line matches", lineprev, "Line 6");
  fin.close();

  TEST( "Delete temp file", vpl_unlink( temp_filename.c_str() ), 0 );
}


void
test_sequential_write_seek(bool binary)
{
  std::cout << "Test sequential write seek (binary=" << binary << ")\n";

  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode(0);

  std::string temp_filename = vul_temp_filename();
  std::cout << "Temp file is " << temp_filename << "\n";

  // Prime up a test file
  {
    std::ofstream f(temp_filename.c_str(), std::ios::out|mode);
    f << "Initial contents\n";
    TEST("Created initial contents", f.good(), true);
  }

  vidtk::large_file_ofstream f(temp_filename.c_str(), std::ios::out|mode);
  TEST("File opened for writing", f.good(), true);

  f << "Modified";
  TEST("Wrote modification", f.good(), true);
  f.seekp(2);
  f << "l";
  TEST("Seeked and modified again", f.good(), true);
  f.close();
  TEST("Writing done", f.good(), true);

  std::ifstream fin(temp_filename.c_str(), std::ios::in|mode);
  TEST("File re-opened for reading", fin.good(), true);
  std::string line;

  std::getline(fin,line);
  std::cout << "First line is \"" << line << "\"\n";
  TEST("Original contents are modified", line, "Molified");

  line="";
  std::getline(fin,line);
  std::cout << "Second line is \"" << line << "\"\n";
  TEST("Writes do not append", line, "");

  TEST("At EOF", fin.eof(), true);
  TEST("State is not bad", fin.bad(), false);
  fin.close();

  TEST( "Delete temp file", vpl_unlink( temp_filename.c_str() ), 0 );
}


void
test_sequential_write_seek_and_append(bool binary)
{
  std::cout << "Test sequential write append (binary=" << binary << ")\n";

  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode(0);

  std::string temp_filename = vul_temp_filename();
  std::cout << "Temp file is " << temp_filename << "\n";

  // Prime up a test file
  {
    std::ofstream f(temp_filename.c_str(), std::ios::out|mode);
    f << "Initial contents\n";
    TEST("Created initial contents", f.good(), true);
  }

  vidtk::large_file_ofstream f(temp_filename.c_str(), std::ios::out|mode|std::ios::app);
  TEST("File opened for writing", f.good(), true);

  f << "Modified";
  TEST("Wrote modification", f.good(), true);
  f.seekp(2);
  f << "l";
  TEST("Seeked and modified again", f.good(), true);
  f.close();
  TEST("Writing done", f.good(), true);

  std::ifstream fin(temp_filename.c_str(), std::ios::in|mode);
  TEST("File re-opened for reading", fin.good(), true);
  std::string line;

  std::getline(fin,line);
  TEST("Original contents are preserved", line, "Initial contents");

  line="";
  std::getline(fin,line);
  std::cout << "Second line is \"" << line << "\"\n";
  TEST("Writes append", line, "Modifiedl");

  TEST("At EOF", fin.eof(), true);
  TEST("State is not bad", fin.bad(), false);
  fin.close();

  TEST( "Delete temp file", vpl_unlink( temp_filename.c_str() ), 0 );
}


void test_read_nonexistent(bool binary)
{
  std::cout << "Test read non-existent file (binary="<<binary<<")\n";

  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode(0);

  vidtk::large_file_ifstream f("/this/should/be/a/non-existent/file", mode);

  TEST("Couldn't open file", f.good(), false);
}


} // end anonymous namespace

int test_large_file_fstream( int argc, char* argv[] )
{
  testlib_test_start( "large_file_fstream" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    std::string data_dir( argv[1] );

    test_read_nonexistent( /*binary=*/ true );
    test_read_nonexistent( /*binary=*/ false );
    test_sequential_text_read_to_end( data_dir );
    test_sequential_binary_read_to_end( data_dir );
    test_seek_binary_read_small( data_dir );

    test_sequential_write( /*binary=*/ true );
    test_sequential_write( /*binary=*/ false );

    test_sequential_write_seek( /*binary=*/ true );
    test_sequential_write_seek( /*binary=*/ false );

    test_sequential_write_seek_and_append( /*binary=*/ true );
    test_sequential_write_seek_and_append( /*binary=*/ false );
  }

  return testlib_test_summary();
}
