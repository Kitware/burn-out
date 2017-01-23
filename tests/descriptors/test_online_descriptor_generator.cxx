/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <testlib/testlib_test.h>

#include <descriptors/online_descriptor_generator.h>

// This file compliments the other online descriptor generator tests, which lie in
// the event manager folder.

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

class failing_descriptor : public online_descriptor_generator
{
public:

  failing_descriptor( unsigned thread_count )
  {
    generator_settings settings;
    settings.thread_count = thread_count;
    this->set_operating_mode( settings );
  }

  ~failing_descriptor() {}

  virtual external_settings* create_settings() { return 0; }

protected:

  virtual bool frame_update_routine()
  {
    return true;
  }

  virtual bool track_update_routine( track_sptr const&, track_data_sptr )
  {
    return false;
  }

};

void test_online_descriptor_generator()
{
  failing_descriptor threaded_test(4);
  failing_descriptor unthreaded_test(1);

  frame_data_sptr dummy_frame1( new frame_data() );
  frame_data_sptr dummy_frame2( new frame_data() );

  vcl_vector< track_sptr > empty_track_vector;
  vcl_vector< track_sptr > dummy_tracks;

  dummy_tracks.push_back( track_sptr( new track() ) );
  dummy_tracks[0]->set_id( 1 );

  dummy_frame1->set_active_tracks( empty_track_vector );
  dummy_frame2->set_active_tracks( dummy_tracks );

  threaded_test.set_input_frame( dummy_frame1 );
  unthreaded_test.set_input_frame( dummy_frame1 );

  TEST( "Threaded fail check 1", threaded_test.step(), true );
  TEST( "Unthreaded fail check 1", unthreaded_test.step(), true );

  threaded_test.set_input_frame( dummy_frame2 );
  unthreaded_test.set_input_frame( dummy_frame2 );

  TEST( "Threaded fail check 2", threaded_test.step(), false );
  TEST( "Unthreaded fail check 2", unthreaded_test.step(), false );
}

} // end anonymous namespace

int test_online_descriptor_generator( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_online_descriptor_generator" );

  test_online_descriptor_generator();

  return testlib_test_summary();
}
