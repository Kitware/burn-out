/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vul/vul_temp_filename.h>
#include <vpl/vpl.h>
#include <vsl/vsl_binary_io.h>
#include <testlib/testlib_test.h>

#include <utilities/timestamp.h>
#include <utilities/vsl/timestamp_io.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void
test_lessthan()
{
  vcl_cout << "Test operator<\n";

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    t2.set_time( 6.0 );
    TEST( "t1<t2, time based", t1 < t2, true );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_frame_number( 5 );
    t2.set_frame_number( 6 );
    TEST( "t1<t2, frame number based", t1 < t2, true );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    t2.set_time( 6.0 );
    t1.set_frame_number( 7 );
    t2.set_frame_number( 6 );
    TEST( "t1<t2, time trumps frame number", t1 < t2, true );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    TEST( "Both or neither must have time", t1 < t2, false );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    t1.set_frame_number( 6 );
    t2.set_frame_number( 7 );
    TEST( "Both or neither must have time, even if both have frame num", t1 < t2, false );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_frame_number( 7 );
    TEST( "Both or neither must have frame number (if no time)", t1 < t2, false );
  }

  {
    vidtk::timestamp t1, t2;
    TEST( "Empty times don't compare", t1 < t2, false );
  }
}


void
test_equality()
{
  vcl_cout << "Test operator==\n";

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    t2.set_time( 6.0 );
    TEST( "t1!=t2, time based", t1 == t2, false );
    t2.set_time( 5.0 );
    TEST( "t1==t2, time based", t1 == t2, true );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_frame_number( 5 );
    t2.set_frame_number( 6 );
    TEST( "t1!=t2, frame number based", t1 == t2, false );
    t2.set_frame_number( 5 );
    TEST( "t1==t2, frame number based", t1 == t2, true );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    t2.set_time( 5.0 );
    t1.set_frame_number( 7 );
    t2.set_frame_number( 6 );
    TEST( "t1==t2, time trumps frame number", t1 == t2, true );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    TEST( "Both or neither must have time", t1 == t2, false );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_time( 5.0 );
    t1.set_frame_number( 6 );
    t2.set_frame_number( 6 );
    TEST( "Both or neither must have time, even if both have frame num", t1 == t2, false );
  }

  {
    vidtk::timestamp t1, t2;
    t1.set_frame_number( 7 );
    TEST( "Both or neither must have frame number (if no time)", t1 == t2, false );
  }

  {
    vidtk::timestamp t1, t2;
    TEST( "Empty times don't compare equal", t1 == t2, false );
  }
}


void
test_vsl_io()
{
  vcl_cout << "Test vsl io\n";

  vcl_string fn = vul_temp_filename();
  vcl_cout << "Using temp file " << fn << "\n";

  {
    vsl_b_ofstream bfstr( fn );

    TEST( "Open temp stream for writing", !bfstr, false );

    vidtk::timestamp ts0;
    vidtk::timestamp ts1;
    ts1.set_time( 6.5 );
    vidtk::timestamp ts2;
    ts2.set_frame_number( 5 );
    vidtk::timestamp ts3;
    ts3.set_time( 5.2 );
    ts3.set_frame_number( 2 );

    vsl_b_write( bfstr, ts0 );
    vsl_b_write( bfstr, ts1 );
    vsl_b_write( bfstr, ts2 );
    vsl_b_write( bfstr, ts3 );
  }

  {
    vsl_b_ifstream bfstr( fn );

    TEST( "Open temp stream for reading", !bfstr, false );

    {
      vidtk::timestamp ts;
      vsl_b_read( bfstr, ts );
      TEST( "Read 1 successful", !bfstr, false );
      TEST( "Value correct", ! ts.has_time() && ! ts.has_frame_number(), true );
    }

    {
      vidtk::timestamp ts;
      vsl_b_read( bfstr, ts );
      TEST( "Read 2 successful", !bfstr, false );
      TEST( "Value correct", true,
            ts.has_time() && ts.time() == 6.5 &&
            ! ts.has_frame_number() );
    }


    {
      vidtk::timestamp ts;
      vsl_b_read( bfstr, ts );
      TEST( "Read 3 successful", !bfstr, false );
      TEST( "Value correct", true,
            ! ts.has_time() &&
            ts.has_frame_number() && ts.frame_number() == 5 );
    }

    {
      vidtk::timestamp ts;
      vsl_b_read( bfstr, ts );
      TEST( "Read 4 successful", !bfstr, false );
      TEST( "Value correct", true,
            ts.has_time() && ts.time() == 5.2 &&
            ts.has_frame_number() && ts.frame_number() == 2 );
    }
  }

  // Test failure when reading non-timestamps

  {
    vsl_b_ofstream bfstr( fn );
    TEST( "Open temp stream for writing", !bfstr, false );
    double x = 4.2;
    vsl_b_write( bfstr, x );
  }
  {
    vsl_b_ifstream bfstr( fn );
    TEST( "Open temp stream for reading", !bfstr, false );
    vidtk::timestamp ts;
    vsl_b_read( bfstr, ts );
    TEST( "Read 1 unsuccessful", !bfstr, true );
  }

  // write a non-timestamp, but the initial piece looks like a valid
  // version number for the timestamp.
  {
    vsl_b_ofstream bfstr( fn );
    TEST( "Open temp stream for writing", !bfstr, false );
    int x = 1;
    vsl_b_write( bfstr, x );
  }
  {
    vsl_b_ifstream bfstr( fn );
    TEST( "Open temp stream for reading", !bfstr, false );
    vidtk::timestamp ts;
    vsl_b_read( bfstr, ts );
    TEST( "Read 1 unsuccessful", !bfstr, true );
  }


  TEST( "Delete temp file", vpl_unlink( fn.c_str() ), 0 );
}


} // end anonymous namespace

int test_timestamp( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "timestamp" );

  test_lessthan();
  test_equality();
  test_vsl_io();

  return testlib_test_summary();
}
