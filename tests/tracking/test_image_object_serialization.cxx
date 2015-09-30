/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vul/vul_temp_filename.h>
#include <vul/vul_file.h>
#include <vpl/vpl.h>
#include <testlib/testlib_test.h>

#include <tracking/image_object.h>
#include <tracking/image_object_reader_process.h>
#include <tracking/image_object_writer_process.h>

using namespace vidtk;

// put everything in an anonymous namespace to avoid conflicts with
// other tests
namespace
{


image_object_sptr
new_object( double n )
{
  image_object_sptr o = new image_object;
  o->area_ = n;
  return o;
}


bool
test_objects( vcl_vector< image_object_sptr > const& objs,
              unsigned N, double num[] )
{
  bool good = true;
  if( objs.size() == N )
  {
    for( unsigned i = 0; i < N; ++i )
    {
      if( num[i] != objs[i]->area_ )
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
    vcl_cout << "Expected:";
    for( unsigned i = 0; i < N; ++i )
    {
      vcl_cout << " " << num[i];
    }
    vcl_cout << "\nGot:";
    for( unsigned i = 0; i < objs.size(); ++i )
    {
      vcl_cout << " " << objs[i]->area_;
    }
    vcl_cout << "\n";
  }

  return good;
}


void
write_out_objects( vcl_string const& fn )
{
  vcl_cout << "\n\nWriting out in vsl format\n\n";

  image_object_writer_process wr( "wr" );
  config_block blk = wr.params();
  blk.set( "disabled", "false" );
  blk.set( "format", "vsl" );
  blk.set( "filename", fn );

  TEST( "Set params", wr.set_params( blk ), true );
  TEST( "Initialize", wr.initialize(), true );

  image_object_sptr A = new_object( 1.0 );

  // Step 1
  {
    timestamp ts;
    ts.set_time( 2.3 );
    vcl_vector< image_object_sptr > objs;
    objs.push_back( new_object( 2.0 ) );
    objs.push_back( new_object( 3.0 ) );
    wr.set_timestamp( ts );
    wr.set_image_objects( objs );
    TEST( "Step", wr.step(), true );
  }

  // Step 2
  {
    timestamp ts;
    ts.set_time( 2.4 );
    vcl_vector< image_object_sptr > objs;
    objs.push_back( new_object( 3.0 ) );
    objs.push_back( new_object( 4.0 ) );
    objs.push_back( A );
    wr.set_timestamp( ts );
    wr.set_image_objects( objs );
    TEST( "Step", wr.step(), true );
  }

  // Step 3
  {
    timestamp ts;
    ts.set_time( 2.5 );
    vcl_vector< image_object_sptr > objs;
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
read_in_objects( vcl_string const& fn )
{
  vcl_cout << "\n\nReading in vsl format\n\n";

  image_object_reader_process rd( "rd" );
  config_block blk = rd.params();
  blk.set( "disabled", "false" );
  blk.set( "format", "vsl" );
  blk.set( "filename", fn );

  TEST( "Set params", rd.set_params( blk ), true );
  TEST( "Initialize", rd.initialize(), true );

  image_object_sptr A;

  // Step 1
  {
    TEST( "Step", rd.step(), true );
    double exp[] = { 2.0, 3.0 };
    TEST( "Values okay", test_objects( rd.objects(), 2, exp ), true );
  }

  // Step 2
  {
    TEST( "Step", rd.step(), true );
    double exp[] = { 3.0, 4.0, 1.0 };
    TEST( "Values okay", test_objects( rd.objects(), 3, exp ), true );
    A = rd.objects()[2];
  }

  // Step 3
  {
    TEST( "Step", rd.step(), true );
    double exp[] = { 5.0, 6.0, 1.0 };
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
test_disabled_by_default( vcl_string const& fn )
{
  vcl_cout << "\n\nEnsure writer is disabled by default\n\n";

  image_object_writer_process wr( "wr" );
  TEST( "Initialize as is", wr.initialize(), true );
  TEST( "Is disabled", wr.is_disabled(), true );
  TEST( "Reset to default param", wr.set_params( wr.params() ), true );
  TEST( "Initialize again", wr.initialize(), true );
  TEST( "Is still disabled", wr.is_disabled(), true );

  vcl_cout << "\n\nEnsure reader is enabled by default\n\n";

  image_object_reader_process rd( "rd" );
  config_block params = rd.params();
  params.set( "filename", fn );
  TEST( "Set param", rd.set_params( params ), true );
  TEST( "Initialize", rd.initialize(), true );
  TEST( "Is not disabled", rd.is_disabled(), false );
}


// fn should be a file that exists. It will get overwritten.
void
test_overwrite_existing( vcl_string const& fn, vcl_string const& format )
{
  vcl_cout << "\n\nEnsure writer is doesn't overwrite by default (format = "
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

  vcl_vector< image_object_sptr > objs;
  objs.push_back( new_object( 2.0 ) );
  wr.set_image_objects( objs );
  TEST( "Can write to this stream", wr.step(), true );
}


} // end anonymous namesapce


int test_image_object_serialization( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "image object serialization" );

  vcl_string fn = vul_temp_filename();
  vcl_cout << "Using temp file " << fn << "\n";

  write_out_objects( fn );
  read_in_objects( fn );
  test_disabled_by_default( fn );
  test_overwrite_existing( fn, "0" );
  test_overwrite_existing( fn, "vsl" );

  TEST( "Delete temp file", vpl_unlink( fn.c_str() ), 0 );

  return testlib_test_summary();
}
