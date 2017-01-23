/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_temp_filename.h>
#include <vul/vul_file.h>
#include <vpl/vpl.h>
#include <testlib/testlib_test.h>

#include <tracking_data/image_object.h>
#include <tracking_data/io/image_object_reader_process.h>
#include <tracking_data/io/image_object_writer_process.h>

#include <iostream>
#include <vector>
#include <string>


using namespace vidtk;

// put everything in an anonymous namespace to avoid conflicts with
// other tests
namespace
{


image_object_sptr
new_object( double n )
{
  image_object_sptr o = new image_object;
  o->set_area( n );
  // for testing purposes, we want each new object to have a location
  // even if it's meaningless
  o->set_image_loc(1,1);
  return o;
}


bool
test_objects( std::vector< image_object_sptr > const& objs,
              unsigned N, double num[] )
{
  bool good = true;
  if( objs.size() == N )
  {
    for( unsigned i = 0; i < N; ++i )
    {
      if( num[i] != objs[i]->get_area() )
      {
        good = false;
        break;
      }
    }
  }
  else
  {
    good = false;
  }

  if( ! good )
  {
    std::cout << "Expected:";
    for( unsigned i = 0; i < N; ++i )
    {
      std::cout << " " << num[i];
    }
    std::cout << "\nGot:";
    for( unsigned i = 0; i < objs.size(); ++i )
    {
      std::cout << " " << objs[i]->get_area();
    }
    std::cout << "\n";
  }

  return good;
}


void
write_out_objects( std::string const& fn, std::string const& format )
{
  std::cout << "\n\nWriting out in " << format << " format\n\n";

  image_object_writer_process wr( "wr" );
  config_block blk = wr.params();
  blk.set( "disabled", "false" );
  blk.set( "format", format );
  blk.set( "filename", fn );

  TEST( "Set params", wr.set_params( blk ), true );
  TEST( "Initialize", wr.initialize(), true );

  image_object_sptr A = new_object( 1.0 );

  // Step 1
  {
    timestamp ts( 2.3e6, 1 );
    std::vector< image_object_sptr > objs;
    objs.push_back( new_object( 2.0 ) );
    objs.push_back( new_object( 3.0 ) );
    wr.set_timestamp( ts );
    wr.set_image_objects( objs );
    TEST( "Step", wr.step(), true );
  }

  // Step 2
  {
    timestamp ts( 2.4e6, 2 );
    std::vector< image_object_sptr > objs;
    objs.push_back( new_object( 3.0 ) );
    objs.push_back( new_object( 4.0 ) );
    objs.push_back( A );
    wr.set_timestamp( ts );
    wr.set_image_objects( objs );
    TEST( "Step", wr.step(), true );
  }

  // Step 3
  {
    timestamp ts( 2.5e6, 3 );
    std::vector< image_object_sptr > objs;
    objs.push_back( new_object( 5.0 ) );
    objs.push_back( new_object( 6.0 ) );
    // Push the *same* object in, to simulate the case when a new
    // object happens to be allocated at the same location as an old
    // object.  As far as the stream is concerned, this should be a
    // new MOD, unrelated to the previous one.
    objs.push_back( A );
    wr.set_timestamp( ts );
    wr.set_image_objects( objs );
    TEST( "Step", wr.step(), true );
  }
}


void
read_in_objects( std::string const& fn )
{
  std::cout << "\n\nReading in " << fn << "\n\n";

  image_object_reader_process rd( "rd" );
  config_block blk = rd.params();
  blk.set( "disabled", "false" );
  blk.set( "filename", fn );

  TEST( "Set params", rd.set_params( blk ), true );
  const bool status = rd.initialize();
  TEST( "Initialize", status, true );
  if ( ! status ) return;

  image_object_sptr A;

  // Step 1
  {
    TEST( "Step", rd.step(), true );
    double exp[] = { 2.0, 3.0 };
    TEST( "timestamp check", rd.timestamp().frame_number(), 1 );
    TEST( "Values okay", test_objects( rd.objects(), 2, exp ), true );
  }

  // Step 2
  {
    TEST( "Step", rd.step(), true );
    double exp[] = { 3.0, 4.0, 1.0 };
    TEST( "timestamp check", rd.timestamp().frame_number(), 2 );
    TEST( "Values okay", test_objects( rd.objects(), 3, exp ), true );
    A = rd.objects()[2];
  }

  // Step 3
  {
    TEST( "Step", rd.step(), true );
    double exp[] = { 5.0, 6.0, 1.0 };
    TEST( "timestamp check", rd.timestamp().frame_number(), 3 );
    TEST( "Values okay", test_objects( rd.objects(), 3, exp ), true );
    image_object_sptr A2 = rd.objects()[2];
    TEST( "Objects were duplicated", A == A2, false );
  }

  // Step 4
  {
    TEST( "Step fails at end of data", rd.step(), false );
  }
}


// fn should be a file that exists.
void
test_disabled_by_default( std::string const& fn )
{
  std::cout << "\n\nEnsure writer is disabled by default\n\n";

  image_object_writer_process wr( "wr" );
  TEST( "Initialize as is", wr.initialize(), true );
  TEST( "Is disabled", wr.is_disabled(), true );
  TEST( "Reset to default param", wr.set_params( wr.params() ), true );
  TEST( "Initialize again", wr.initialize(), true );
  TEST( "Is still disabled", wr.is_disabled(), true );

  std::cout << "\n\nEnsure reader is enabled by default\n\n";

  image_object_reader_process rd( "rd" );
  config_block params = rd.params();
  params.set( "filename", fn );
  TEST( "Set param", rd.set_params( params ), true );
  TEST( "Initialize", rd.initialize(), true );
  TEST( "Is not disabled", rd.is_disabled(), false );
}


// fn should be a file that exists. It will get overwritten.
void
test_overwrite_existing( std::string const& fn, std::string const& format )
{
  std::cout << "\n\nEnsure writer is doesn't overwrite by default (format = "
           << format << ")\n\n";

  TEST( "File exists initially", vul_file::exists( fn ), true );

  image_object_writer_process wr( "wr" );
  config_block params = wr.params();
  params.set( "disabled", "false" );
  params.set( "format", format );
  params.set( "filename", fn );
  TEST( "Set parameters, overwrite = default", wr.set_params( params ), true );
  TEST( "Refuses to initialize", wr.initialize(), false );

  params = wr.params();
  params.set( "overwrite_existing", "true" );
  TEST( "Set parameters, overwrite = true", wr.set_params( params ), true );
  TEST( "Initializes", wr.initialize(), true );

  std::vector< image_object_sptr > objs;
  objs.push_back( new_object( 2.0 ) );
  wr.set_image_objects( objs );
  TEST( "Can write to this stream", wr.step(), true );
}


} // end anonymous namesapce


// ----------------------------------------------------------------
/** Main test driver.
 *
 *
 */
int test_image_object_serialization( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "image object serialization" );

  std::string fn = vul_temp_filename();
  std::cout << "Using temp file " << fn << "\n";

  write_out_objects( fn, "kw18" );
  write_out_objects( fn, "det" );
  write_out_objects( fn, "vsl" );

#ifdef USE_PROTOBUF
  write_out_objects( fn, "protobuf" );
#endif

  std::string vfn (fn);
  vfn = vfn + ".kw18";
  read_in_objects( vfn );

  vfn = fn + ".det";
  read_in_objects( vfn );

  vfn = fn + ".vsl";
  read_in_objects( vfn );

#ifdef USE_PROTOBUF
  vfn =  fn + ".kwpc";
  read_in_objects( vfn );
#endif

  test_disabled_by_default( vfn );

  std::string dfn (fn);
  dfn = dfn + ".det";
  test_overwrite_existing( dfn, "0" );
  test_overwrite_existing( vfn, "vsl" );

  std::string kfn (fn);
  kfn = kfn + ".kw18";
  test_overwrite_existing( kfn, "kw18" );

  TEST( "Delete temp files", vul_file::delete_file_glob (fn + ".*"), true);

  return testlib_test_summary();
}
