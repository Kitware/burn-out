/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>

#include <tracking_data/io/raw_descriptor_writer.h>
#include <tracking_data/io/raw_descriptor_writer_process.h>

#include <string>
#include <vector>
#include <sstream>


using namespace vidtk;

namespace
{

void test1()
{
  raw_descriptor_writer rdw;

  // Select writer
  TEST( "Select default format", rdw.set_format( "csv" ), true );

  std::stringstream foo;
  TEST( "Initialize writer", rdw.initialize( &foo ), true );

  // build a raw descriptor
  descriptor_sptr raw = raw_descriptor::create( "test_descrip" );
  raw->add_track_id( 1 );
  raw->add_track_id( 11 );

  descriptor_data_t ddata;
  ddata.push_back( 123 );
  ddata.push_back( 234 );
  raw->set_features( ddata );

  TEST( "Write", rdw.write( raw ), true );

  rdw.finalize();

  // This is a weak test, but it is a place to start
  TEST( "General format", (foo.str()[0] == '#'), true );
}


void test2()
{
  vidtk::raw_descriptor_writer_process rdwp( "the_writer" );

  config_block blk = rdwp.params();

  // blk.print( std::cout );

  blk.set_value( "format", "foo" );
  TEST( "Invalid format", rdwp.set_params( blk ), false );

  blk.set_value( "format", "csv" );
  TEST( "Valid format", rdwp.set_params( blk ), true );

  TEST( "Initialize process", rdwp.initialize(), true );
}


void test3()
{
  raw_descriptor_writer rdw;

  // Select writer
  TEST( "Select JSON writer", rdw.set_format( "json" ), true );

  std::stringstream foo;
  TEST( "Initialize writer", rdw.initialize( &foo ), true );

  // build a raw descriptor
  descriptor_sptr raw = raw_descriptor::create( "test_descrip" );
  raw->add_track_id( 1 );
  raw->add_track_id( 11 );

  descriptor_data_t ddata;
  ddata.push_back( 123 );
  ddata.push_back( 234 );
  ddata.push_back( 24 );
  ddata.push_back( 34 );
  ddata.push_back( 23 );
  raw->set_features( ddata );

  TEST( "Write 1", rdw.write( raw ), true );
  TEST( "Write 2", rdw.write( raw ), true);

  rdw.finalize();

  // Validate JSON via http://jsonlint.com/
}


} // end namespace


int test_raw_descriptor_writer( int /* argc */, char * /* argv */ [] )
{
  testlib_test_start( "test raw descriptor writers" );

  test1();
  test2();
  test3();

  return testlib_test_summary();
}
