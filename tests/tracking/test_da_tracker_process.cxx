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
#include <tracking/da_tracker_process.h>
#include <tracking/track.h>
#include <tracking/tracking_keys.h>
#include <tracking/da_so_tracker_generator.h>

#include <utilities/ring_buffer_process.h>
#include <utilities/timestamp.h>

#include <pipeline/sync_pipeline.h>

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



void test_final_terminated_tracks()
{
  vcl_cout << "\n\nLast iteration terminates all active tracks\n\n";

  da_tracker_process tracker( "trk" );
  vgl_h_matrix_2d<double> H;
  H.set_identity();
  tracker.set_world2image_homography(H);

  // Initialize with two tracks
  da_so_tracker_generator generator;
  generator.set_to_default();
  vcl_vector< track_sptr > init_trks;
  vcl_vector< da_so_tracker_sptr > init_trkers;
  vcl_vector< da_so_tracker_sptr > empty_init_trkers;
  {
    track_sptr trk( new track );
    track_state_sptr st( new track_state );
    st->vel_ = vnl_double_3( 0.0, 0.0, 0.0 );
    st->loc_ = vnl_double_3( 2.0, 2.0, 0.0 );
    timestamp ts;
    ts.set_frame_number( 0 );
    ts.set_time( 0 );
    st->time_ = ts;
    trk->add_state( st );
    init_trks.push_back( trk );
    init_trkers.push_back( generator.generate(trk) );
  }
  {
    track_sptr trk( new track );
    track_state_sptr st( new track_state );
    st->vel_ = vnl_double_3( 0.0, 0.0, 0.0 );
    st->loc_ = vnl_double_3( 2.0, 4.0, 0.0 );
    timestamp ts;
    ts.set_frame_number( 0 );
    ts.set_time( 0 );
    st->time_ = ts;
    trk->add_state( st );
    init_trks.push_back( trk );
    init_trkers.push_back( generator.generate(trk) );
  }

  TEST( "Set parameters",
        tracker.set_params(
          tracker.params()
          .set_value( "terminate:missed_count", 0 ) ),
        true );
  TEST( "Initialize", tracker.initialize(), true );

  vcl_vector< track_sptr > active_trks;

  // Frame 1: both exist
  {
    tracker.set_new_trackers( init_trkers );

    timestamp ts;
    ts.set_frame_number( 1 );
    ts.set_time( 1 );

    vcl_vector< image_object_sptr > objs;
    objs.push_back( object2d( 2.0, 2.0 ) );
    objs.push_back( object2d( 2.0, 4.0 ) );

    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );

    TEST( "Step 1", tracker.step(), true );
    TEST( "2 active tracks", tracker.active_tracks().size(), 2 );
    TEST( "0 terminated tracks", tracker.terminated_tracks().size(), 0 );
    active_trks = tracker.active_tracks();
  }

  // Frame 2: both exist
  {
    timestamp ts;
    ts.set_frame_number( 2 );
    ts.set_time( 2 );

    vcl_vector< image_object_sptr > objs;
    objs.push_back( object2d( 2.0, 2.0 ) );
    objs.push_back( object2d( 2.0, 4.0 ) );

    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 2", tracker.step(), true );
    TEST( "2 active tracks", tracker.active_tracks().size(), 2 );
    TEST( "0 terminated tracks", tracker.terminated_tracks().size(), 0 );
    active_trks = tracker.active_tracks();
  }

  // Frame 3: first dies
  {
    timestamp ts;
    ts.set_frame_number( 3 );
    ts.set_time( 3 );

    vcl_vector< image_object_sptr > objs;
    objs.push_back( object2d( 2.0, 4.0 ) );

    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 3", tracker.step(), true );
    TEST( "1 active tracks", tracker.active_tracks().size(), 1 );
    TEST( "1 terminated tracks", tracker.terminated_tracks().size(), 1 );
    bool okay = false;
    if( tracker.terminated_tracks().size() == 1 )
    {
      okay = ( tracker.terminated_tracks()[0]->history().size() == 3 );
    }
    TEST( "Terminated track has length 3", okay, true );
    active_trks = tracker.active_tracks();
  }

  // Frame 4: second exists
  {
    timestamp ts;
    ts.set_frame_number( 4 );
    ts.set_time( 4 );

    vcl_vector< image_object_sptr > objs;
    objs.push_back( object2d( 2.0, 4.0 ) );

    tracker.set_active_tracks( tracker.active_tracks() );
    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 4", tracker.step(), true );
    TEST( "1 active tracks", tracker.active_tracks().size(), 1 );
    TEST( "0 terminated tracks", tracker.terminated_tracks().size(), 0 );
    active_trks = tracker.active_tracks();
  }

  // Frame 5: second exists
  {
    timestamp ts;
    ts.set_frame_number( 5 );
    ts.set_time( 5 );

    vcl_vector< image_object_sptr > objs;
    objs.push_back( object2d( 2.0, 4.0 ) );

    tracker.set_active_tracks( tracker.active_tracks() );
    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 5", tracker.step(), true );
    TEST( "1 active tracks", tracker.active_tracks().size(), 1 );
    TEST( "0 terminated tracks", tracker.terminated_tracks().size(), 0 );
    active_trks = tracker.active_tracks();
  }

  // No frame: all terminated
  {
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step no data", tracker.step(), true );
    vcl_cout << "Active track count = " << tracker.active_tracks().size() << "\n";
    vcl_cout << "Terminated track count = " << tracker.terminated_tracks().size() << "\n";
    TEST( "0 active tracks", tracker.active_tracks().size(), 0 );
    TEST( "1 terminated tracks", tracker.terminated_tracks().size(), 1 );
    bool okay = false;
    if( tracker.terminated_tracks().size() == 1 )
    {
      okay = ( tracker.terminated_tracks()[0]->history().size() == 6 );
    }
    TEST( "Terminated track has length 6", okay, true );
    active_trks = tracker.active_tracks();
  }

  // Next step
  {
    tracker.set_new_trackers( empty_init_trkers );
    tracker.set_active_tracks( active_trks );
    TEST( "Step one-after final fails", tracker.step(), false );
  }
}


struct obj_det_process
  : public process
{
  typedef obj_det_process self_type;

  obj_det_process()
    : process( "objdet", "obj_det_process" ),
      count_( 0 )
  {
  }

  config_block params() const
  {
    return config_block();
  }

  bool set_params( config_block const& )
  {
    return true;
  }

  bool initialize()
  {
    count_ = 0;
    return true;
  }

  bool step()
  {
    if( count_ < 4 )
    {
      ++count_;
      return true;
    }
    else
    {
      count_ = 0;
      return false;
    }
  }

  timestamp time() const
  {
    timestamp ts;
    ts.set_frame_number( count_ );
    ts.set_time( double(count_) / 10.0 );
    return ts;
  }

  VIDTK_OUTPUT_PORT( timestamp, time );

  vcl_vector<image_object_sptr> objs() const
  {
    vcl_vector<image_object_sptr> o;
    o.push_back( object2d( 0.0, 0.0 ) );
    if( count_ < 3 )
    {
      o.push_back( object2d( 0.0, 4.0 ) );
    }
    return o;
  }

  VIDTK_OUTPUT_PORT( vcl_vector<image_object_sptr>, objs );

  unsigned count_;
};


struct track_writer_process
  : public process
{
  typedef track_writer_process self_type;

  track_writer_process()
    : process( "trkwriter", "track_writer_process" ),
      count_( 0 ),
      total_length_( 0 )
  {
  }

  config_block params() const
  {
    return config_block();
  }

  bool set_params( config_block const& )
  {
    return true;
  }

  bool initialize()
  {
    count_ = 0;
    total_length_ = 0;
    return true;
  }

  bool step()
  {
    return true;
  }

  void set_tracks( vcl_vector< track_sptr > term_tracks )
  {
    count_ += term_tracks.size();
    for( unsigned i = 0; i < term_tracks.size(); ++i )
    {
      total_length_ += term_tracks[i]->history().size();
    }
  }

  VIDTK_INPUT_PORT( set_tracks, vcl_vector< track_sptr > );

  unsigned count_;
  unsigned total_length_;
};


void test_pipeline_gets_final_tracks()
{
  vcl_cout << "\n\nLast iteration terminates all active tracks\n\n";

  sync_pipeline p;

  obj_det_process objs;
  p.add( &objs );

  da_tracker_process tracker( "trk" );
  vgl_h_matrix_2d<double> H;
  H.set_identity();
  tracker.set_world2image_homography(H);
  p.add( &tracker );
  p.connect( objs.time_port(), tracker.set_timestamp_port() );
  p.connect( objs.objs_port(), tracker.set_objects_port() );

  track_writer_process trkwriter;
  p.add( &trkwriter );
  p.connect( tracker.terminated_tracks_port(), trkwriter.set_tracks_port() );


  config_block config = p.params();
  config.set_value( "trk:terminate:missed_count", 0 );

  TEST( "Set params", p.set_params( config ), true );


  // Initialize with two tracks
  da_so_tracker_generator generator;
  generator.set_to_default();
  vcl_vector< track_sptr > init_trks;
  vcl_vector< da_so_tracker_sptr > init_trkers;
  {
    track_sptr trk( new track );
    track_state_sptr st( new track_state );
    st->vel_ = vnl_double_3( 0.0, 0.0, 0.0 );
    st->loc_ = vnl_double_3( 0.0, 0.0, 0.0 );
    timestamp ts;
    ts.set_frame_number( 0 );
    ts.set_time( 0 );
    st->time_ = ts;
    trk->add_state( st );
    init_trks.push_back( trk );
    init_trkers.push_back(generator.generate(trk));
  }
  {
    track_sptr trk( new track );
    track_state_sptr st( new track_state );
    st->vel_ = vnl_double_3( 0.0, 0.0, 0.0 );
    st->loc_ = vnl_double_3( 0.0, 4.0, 0.0 );
    timestamp ts;
    ts.set_frame_number( 0 );
    ts.set_time( 0 );
    st->time_ = ts;
    trk->add_state( st );
    init_trks.push_back( trk );
    init_trkers.push_back(generator.generate(trk));
  }
  tracker.set_new_trackers( init_trkers );

  vcl_vector< track_sptr > active_trks;
  vcl_vector< da_so_tracker_sptr > empty_trkrs;

  tracker.set_active_tracks( vcl_vector< track_sptr >() );

  vcl_cout << "Executing pipeline\n";
  unsigned step_count = 0;
  while( p.execute() )
  {
    tracker.set_new_trackers( empty_trkrs );
    active_trks = tracker.active_tracks();
    tracker.set_active_tracks( active_trks );
    ++step_count;
    vcl_cout << "External step count = " << step_count << "\n";
    vcl_cout << "Obj processes frame count = " << objs.time().frame_number() << "\n";
    vcl_cout << "\n\n\n";
  }

  vcl_cout << "Num terminated tracks = " << trkwriter.count_ << "\n";
  vcl_cout << "Total length = " << trkwriter.total_length_ << "\n";

  TEST( "2 terminated tracks", trkwriter.count_, 2 );
  TEST( "Total length 8", trkwriter.total_length_, 8 );
}


} // end anonymous namespace


int test_da_tracker_process( int /*argc*/, char* /*argv*/[] )
{
  using namespace vidtk;

  testlib_test_start( "da tracker process" );

  test_final_terminated_tracks();
  test_pipeline_gets_final_tracks();

  return testlib_test_summary();
}
