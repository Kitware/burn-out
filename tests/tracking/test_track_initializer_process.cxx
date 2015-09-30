/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>
#include <testlib/testlib_test.h>

#include <tracking/image_object.h>
#include <tracking/track_initializer_process.h>
#include <tracking/tracking_keys.h>
#include <utilities/ring_buffer_process.h>
#include <utilities/timestamp.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

typedef vcl_vector<image_object_sptr> vec_obj_type;


image_object_sptr object2d( double x, double y )
{
  image_object_sptr o = new image_object;
  o->img_loc_ = vnl_double_2(x,y);
  o->world_loc_ = vnl_double_3(x,y,0);
  return o;
}

// Format is "time x1 y1 x2 y2 ... xN yN"
void add_to_buffers_2d( ring_buffer_process<vec_obj_type>& obj_buffer,
                        ring_buffer_process<timestamp>& time_buffer,
                        ring_buffer_process< vgl_h_matrix_2d<double> >& wld2img_H_buf,
                        vcl_string const& descr )
{
  vcl_istringstream sstr( descr );

  double t;
  sstr >> t;

  double x, y;

  vec_obj_type objs;
  while( sstr >> x >> y )
  {
    objs.push_back( object2d( x, y ) );
  }

  timestamp ts( t );
  time_buffer.set_next_datum( ts );
  obj_buffer.set_next_datum( objs );

  vgl_h_matrix_2d<double> H;
  H.set_identity();
  wld2img_H_buf.set_next_datum( H );

  TEST( "Push into buffer", time_buffer.step() && obj_buffer.step()
                            && wld2img_H_buf.step(), true );
}


// Format is "time x1 y1 U1 x2 y2 U2 ... xN yN UN"
// where U = "x" or "o", with "x"="used in track" and "o"="unused".
void add_to_buffers_2d_used( ring_buffer_process<vec_obj_type>& obj_buffer,
                             ring_buffer_process<timestamp>& time_buffer,
                             ring_buffer_process< vgl_h_matrix_2d<double> > & wld2img_H_buf,
                             vcl_string const& descr )
{
  vcl_istringstream sstr( descr );

  double t;
  sstr >> t;

  double x, y;

  vec_obj_type objs;
  char type;
  while( sstr >> x >> y >> type )
  {
    objs.push_back( object2d( x, y ) );
    if( type == 'x' )
    {
      objs.back()->data_.set( tracking_keys::used_in_track, true );
    }
  }

  timestamp ts( t );
  time_buffer.set_next_datum( ts );
  obj_buffer.set_next_datum( objs );

  vgl_h_matrix_2d<double> H;
  H.set_identity();
  wld2img_H_buf.set_next_datum( H );

  TEST( "Push into buffer", time_buffer.step() && obj_buffer.step()
                            && wld2img_H_buf.step(), true );
}


void test_no_noise()
{
  typedef vcl_vector<image_object_sptr> vec_obj_type;
  typedef vcl_vector<timestamp> vec_ts_type;

  vcl_cout << "\n\nSingle object to initialize, delta=1\n\n";
  {
    ring_buffer_process<vec_obj_type> obj_buffer( "objbuf" );
    obj_buffer.set_params( obj_buffer.params()
                           .set_value( "length", 10 ) );

    ring_buffer_process<timestamp> time_buffer( "timebuf" );
    time_buffer.set_params( time_buffer.params()
                            .set_value( "length", 10 ) );

    ring_buffer_process< vgl_h_matrix_2d<double> > wld2img_H_buf( "homogbuf" );
    wld2img_H_buf.set_params( wld2img_H_buf.params()
                            .set_value( "length", 10 ) );

    track_initializer_process init( "init" );
    init.set_mod_buffer( obj_buffer );
    init.set_timestamp_buffer( time_buffer );
    init.set_wld2img_homog_buffer( wld2img_H_buf );
    init.set_params( init.params()
                     .set_value( "delta", 2 )
                     .set_value( "allowed_miss_count", 0 )
                     .set_value( "kinematics_sigma_gate", 1 ) );

    TEST( "Initialize", init.initialize(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "0 0 0" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "1 1 2" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "2 2 4" );
    TEST( "Step", init.step(), true );
    TEST( "1 new tracks", init.new_tracks().size(), 1 );

    // non-linear time step.
    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "4 4 8" );
    TEST( "Step", init.step(), true );
    TEST( "1 new tracks", init.new_tracks().size(), 1 );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "5 5 80" );
    TEST( "Step", init.step(), true );
    TEST( "no new tracks", init.new_tracks().size(), 0 );
  }

  vcl_cout << "\n\nTwo objects to initialize, delta=1\n\n";
  {
    ring_buffer_process<vec_obj_type> obj_buffer( "objbuf" );
    obj_buffer.set_params( obj_buffer.params()
                           .set_value( "length", 10 ) );

    ring_buffer_process<timestamp> time_buffer( "timebuf" );
    time_buffer.set_params( time_buffer.params()
                            .set_value( "length", 10 ) );

    ring_buffer_process< vgl_h_matrix_2d<double> > wld2img_H_buf( "homogbuf" );
    wld2img_H_buf.set_params( wld2img_H_buf.params()
                            .set_value( "length", 10 ) );

    track_initializer_process init( "init" );
    init.set_mod_buffer( obj_buffer );
    init.set_timestamp_buffer( time_buffer );
    init.set_wld2img_homog_buffer( wld2img_H_buf );
    init.set_params( init.params()
                     .set_value( "delta", 2 )
                     .set_value( "allowed_miss_count", 0 )
                     .set_value( "kinematics_sigma_gate", 1 ) );
    TEST( "Initialize", init.initialize(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "0 0 0 10 10" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "1 1 2 10 9" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "2 2 4 10 8" );
    TEST( "Step", init.step(), true );
    TEST( "2 new tracks", init.new_tracks().size(), 2 );

    // non-linear time step.
    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "4 4 8" );
    TEST( "Step", init.step(), true );
    TEST( "1 new tracks", init.new_tracks().size(), 1 );

    // "switch" objects: use 10,8 from obj 2, 4,8 from obj 1
    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "5 1 8" );
    TEST( "Step", init.step(), true );
    TEST( "1 new tracks", init.new_tracks().size(), 1 );
  }

}


void test_used_in_track()
{
  vcl_cout << "\n\nTest that used_in_track is honoured\n\n";
  {
    ring_buffer_process<vec_obj_type> obj_buffer( "objbuf" );
    obj_buffer.set_params( obj_buffer.params()
                           .set_value( "length", 10 ) );

    ring_buffer_process<timestamp> time_buffer( "timebuf" );
    time_buffer.set_params( time_buffer.params()
                            .set_value( "length", 10 ) );

    ring_buffer_process< vgl_h_matrix_2d<double> > wld2img_H_buf( "homogbuf" );
    wld2img_H_buf.set_params( wld2img_H_buf.params()
                            .set_value( "length", 10 ) );

    track_initializer_process init( "init" );
    init.set_mod_buffer( obj_buffer );
    init.set_timestamp_buffer( time_buffer );
    init.set_wld2img_homog_buffer( wld2img_H_buf );
    init.set_params( init.params()
                     .set_value( "delta", 2 )
                     .set_value( "allowed_miss_count", 0 )
                     .set_value( "kinematics_sigma_gate", 1 ) );
    TEST( "Initialize", init.initialize(), true );

    add_to_buffers_2d_used( obj_buffer, time_buffer, wld2img_H_buf,
                       "0 0 0 o" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d_used( obj_buffer, time_buffer, wld2img_H_buf,
                       "1 1 2 x" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d_used( obj_buffer, time_buffer, wld2img_H_buf,
                       "2 2 4 o" );
    TEST( "Step", init.step(), true );
    TEST( "no new tracks", init.new_tracks().size(), 0 );

    // non-linear time step.
    add_to_buffers_2d_used( obj_buffer, time_buffer, wld2img_H_buf,
                       "4 4 8 o" );
    TEST( "Step", init.step(), true );
    TEST( "0 new tracks", init.new_tracks().size(), 0 );

    add_to_buffers_2d_used( obj_buffer, time_buffer, wld2img_H_buf,
                       "5 5 10 o" );
    TEST( "Step", init.step(), true );
    TEST( "1 new tracks", init.new_tracks().size(), 1 );
  }
}


void test_error_components()
{
  typedef vcl_vector<image_object_sptr> vec_obj_type;
  typedef vcl_vector<timestamp> vec_ts_type;

  vcl_cout << "\n\nDifferent thresholds for along the\n\n";
  {
    vcl_cout << "Without different thresholds\n";
    ring_buffer_process<vec_obj_type> obj_buffer( "objbuf" );
    obj_buffer.set_params( obj_buffer.params()
                           .set_value( "length", 10 ) );

    ring_buffer_process<timestamp> time_buffer( "timebuf" );
    time_buffer.set_params( time_buffer.params()
                            .set_value( "length", 10 ) );

    ring_buffer_process< vgl_h_matrix_2d<double> > wld2img_H_buf( "homogbuf" );
    wld2img_H_buf.set_params( wld2img_H_buf.params()
                            .set_value( "length", 10 ) );

    track_initializer_process init( "init" );
    init.set_mod_buffer( obj_buffer );
    init.set_timestamp_buffer( time_buffer );
    init.set_wld2img_homog_buffer( wld2img_H_buf );
    init.set_params( init.params()
                     .set_value( "delta", 2 )
                     .set_value( "allowed_miss_count", 0 )
                     .set_value( "kinematics_sigma_gate", 2 ) );
    TEST( "Initialize", init.initialize(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "0 0 0" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    // normal error 1, tang error -3.  Computed error = sqrt(10)
    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "1 1 2" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "2 0 10" );
    TEST( "Step", init.step(), true );
    TEST( "0 new tracks", init.new_tracks().size(), 0 );
  }

  {
    vcl_cout << "With different thresholds, acceptable errors\n";
    ring_buffer_process<vec_obj_type> obj_buffer( "objbuf" );
    obj_buffer.set_params( obj_buffer.params()
                           .set_value( "length", 10 ) );

    ring_buffer_process<timestamp> time_buffer( "timebuf" );
    time_buffer.set_params( time_buffer.params()
                            .set_value( "length", 10 ) );

    ring_buffer_process< vgl_h_matrix_2d<double> > wld2img_H_buf( "homogbuf" );
    wld2img_H_buf.set_params( wld2img_H_buf.params()
                            .set_value( "length", 10 ) );

    track_initializer_process init( "init" );
    init.set_mod_buffer( obj_buffer );
    init.set_timestamp_buffer( time_buffer );
    init.set_wld2img_homog_buffer( wld2img_H_buf );
    init.set_params( init.params()
                     .set_value( "delta", 2 )
                     .set_value( "allowed_miss_count", 0 )
                     .set_value( "tangential_sigma", 3 )
                     .set_value( "normal_sigma", 1 )
                     .set_value( "kinematics_sigma_gate", 2 ) );
    TEST( "Initialize", init.initialize(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "0 0 0" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    // normal error 1/1, tang error -3/3.  Computed error = sqrt(2)
    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "1 1 2" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "2 0 10" );
    TEST( "Step", init.step(), true );
    TEST( "1 new tracks", init.new_tracks().size(), 1 );
  }

  {
    vcl_cout << "With different thresholds, acceptable tang error, unacceptable norm error\n";
    ring_buffer_process<vec_obj_type> obj_buffer( "objbuf" );
    obj_buffer.set_params( obj_buffer.params()
                           .set_value( "length", 10 ) );

    ring_buffer_process<timestamp> time_buffer( "timebuf" );
    time_buffer.set_params( time_buffer.params()
                            .set_value( "length", 10 ) );

    ring_buffer_process< vgl_h_matrix_2d<double> > wld2img_H_buf( "homogbuf" );
    wld2img_H_buf.set_params( wld2img_H_buf.params()
                            .set_value( "length", 10 ) );

    track_initializer_process init( "init" );
    init.set_mod_buffer( obj_buffer );
    init.set_timestamp_buffer( time_buffer );
    init.set_wld2img_homog_buffer( wld2img_H_buf );
    init.set_params( init.params()
                     .set_value( "delta", 2 )
                     .set_value( "allowed_miss_count", 0 )
                     .set_value( "tangential_sigma", 3 )
                     .set_value( "normal_sigma", 1 )
                     .set_value( "kinematics_sigma_gate", 2 ) );
    TEST( "Initialize", init.initialize(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "0 0 0" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    // normal error 2/1, tang error -3/3.  Computed error = sqrt(5)
    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "1 2 2" );
    TEST( "Step", init.step(), true );
    TEST( "No new tracks", init.new_tracks().empty(), true );

    add_to_buffers_2d( obj_buffer, time_buffer, wld2img_H_buf,
                       "2 0 10" );
    TEST( "Step", init.step(), true );
    TEST( "0 new tracks", init.new_tracks().size(), 0 );
  }
}


} // end anonymous namespace


int test_track_initializer_process( int /*argc*/, char* /*argv*/[] )
{
  using namespace vidtk;

  testlib_test_start( "track initializer process" );

  test_no_noise();
  test_used_in_track();
  test_error_components();

  return testlib_test_summary();
}
