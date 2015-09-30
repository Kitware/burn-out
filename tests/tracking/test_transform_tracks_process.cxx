/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <testlib/testlib_test.h>
#include <tracking/transform_tracks_process.h>

#include <pipeline/sync_pipeline.h>
#include <process_framework/process.h>
#include <tracking/vsl/image_object_io.h>
#include <tracking/tracking_keys.h>
#include <vcl_limits.h>
#include <vcl_string.h>

#include <boost/lexical_cast.hpp>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

vcl_string g_data_dir;

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

    out_tracks_ = term_tracks;

    for( unsigned i = 0; i < term_tracks.size(); ++i )
    {
      total_length_ += term_tracks[i]->history().size();
    }

  }

  VIDTK_INPUT_PORT( set_tracks, vcl_vector< track_sptr > );

  unsigned count_;
  unsigned total_length_;
  vcl_vector<track_sptr> out_tracks_;
};


void test_crop_tracks_by_frame()
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_crop_frame("ptcf");
  p.add( &proc_test_crop_frame );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_crop_frame.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "ptcf:crop_track_frame_range:disabled" , false );
  config.set_value( "ptcf:crop_track_frame_range:start_frame_crop" , 20 );
  config.set_value( "ptcf:crop_track_frame_range:end_frame_crop" , 40 );

  TEST( "Set params", p.set_params( config ), true );

  /*
    create tracks that:
    1) are before the range and optimized out
    2) after the range and optimized out
    3) within the range and not touched (boundary will test that corner case as well)
    4) intersect start of range so beginning states removed
    5) intersect end of range so end states are removed
    6) Has a gap in the middle that is bigger than range so all states are individually removed and track is NULLed
  */

  vcl_vector<track_sptr> init_trks;
  //1)  Filtered out completely
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    timestamp ts1;
    timestamp ts2;
    ts1.set_frame_number( 0 );
    ts1.set_time( 0 );
    ts2.set_frame_number( 10 );
    ts2.set_time( 10 );
    st1->time_ = ts1;
    st2->time_ = ts2;
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //2) Filtered out completely
  {
    track_sptr trk( new track );
    trk->set_id(2);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    timestamp ts1;
    timestamp ts2;
    ts1.set_frame_number( 50 );
    ts1.set_time( 0 );
    ts2.set_frame_number( 60 );
    ts2.set_time( 10 );
    st1->time_ = ts1;
    st2->time_ = ts2;
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //3) Fully included, 2 states
  {
    track_sptr trk( new track );
    trk->set_id(3);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    timestamp ts1;
    timestamp ts2;
    ts1.set_frame_number( 20 );
    ts1.set_time( 0 );
    ts2.set_frame_number( 40 );
    ts2.set_time( 10 );
    st1->time_ = ts1;
    st2->time_ = ts2;
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //4) Partially filtered 1 state, frame 30
  {
    track_sptr trk( new track );
    trk->set_id(4);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    timestamp ts1;
    timestamp ts2;
    ts1.set_frame_number( 10 );
    ts1.set_time( 0 );
    ts2.set_frame_number( 30 );
    ts2.set_time( 10 );
    st1->time_ = ts1;
    st2->time_ = ts2;
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //5) Partially filtered 1 state, frame 30
  {
    track_sptr trk( new track );
    trk->set_id(5);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    timestamp ts1;
    timestamp ts2;
    ts1.set_frame_number( 30 );
    ts1.set_time( 0 );
    ts2.set_frame_number( 50 );
    ts2.set_time( 10 );
    st1->time_ = ts1;
    st2->time_ = ts2;
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //6) Filtered out completely
  {
    track_sptr trk( new track );
    trk->set_id(6);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    timestamp ts1;
    timestamp ts2;
    ts1.set_frame_number( 10 );
    ts1.set_time( 0 );
    ts2.set_frame_number( 50 );
    ts2.set_time( 10 );
    st1->time_ = ts1;
    st2->time_ = ts2;
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }

  proc_test_crop_frame.in_tracks( init_trks );

  while( p.execute() )
  {

  }

  int c = trkwriter.count_;
  TEST("Cropping kept 3 tracks", c == 3, true );
  vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

  track_sptr t = out_tracks[0];
  TEST("First track is id 3", t->id() == 3 , true);
  TEST("Track 3 has 2 states", t->history().size() == 2, true);

  t = out_tracks[1];
  TEST("Second track is id 4", t->id() == 4 , true);
  TEST("Track 4 has 1 state", t->history().size() == 1, true);

  t = out_tracks[2];
  TEST("Second track is id 5", t->id() == 5 , true);
  TEST("Track 4 has 1 state", t->history().size() == 1, true);
}

void test_origin_translate()
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_trans_origin("ptto");
  vgl_h_matrix_2d< double > t;
  t.set_identity();
  t.set_translation( -20, 30 );
  image_to_image_homography H;
  H.set_transform( t );
  proc_test_trans_origin.set_img_homography( H );
  proc_test_trans_origin.set_wld_homography( H );

  p.add( &proc_test_trans_origin );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_trans_origin.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "ptto:transform_image_location:disabled" , false );
  config.set_value( "ptto:transform_image_location:transform_world_loc" , true );
  //  config.set_value( "ptto:transform_image_location:translate_origin_delta_x", -20 );
  //  config.set_value( "ptto:transform_image_location:translate_origin_delta_y", 30 );

  TEST( "Set params", p.set_params( config ), true );

  /*
     1) simple shift
     2) bbox shift rolls below 0 (unsigned)
  */

  vcl_vector<track_sptr> init_trks;
  //1)  Simple shift
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    vcl_vector< image_object_sptr > imgs;
    imgs.push_back(new image_object);

    st1->loc_[0] = 30;
    st1->loc_[1] = 40;
    st1->loc_[2] = 0;

    imgs[0]->img_loc_[0] = 80;
    imgs[0]->img_loc_[1] = 70;
    imgs[0]->img_loc_[2] = 0;

    imgs[0]->world_loc_[0] = 15;
    imgs[0]->world_loc_[1] = 20;
    imgs[0]->world_loc_[2] = 0;

    vgl_box_2d<unsigned> bbox(
      vgl_point_2d<unsigned int>(70, 60),
      vgl_point_2d<unsigned int>(100, 90) );
    imgs[0]->bbox_ = bbox;

    st1->data_.set(tracking_keys::img_objs, imgs);
    trk->add_state( st1 );
    init_trks.push_back( trk );
  }
  //2)  Shift bbox rolls below 0
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    vcl_vector< image_object_sptr > imgs;
    imgs.push_back(new image_object);

    st1->loc_[0] = 60;
    st1->loc_[1] = 80;
    st1->loc_[2] = 0;

    imgs[0]->img_loc_[0] = 30;
    imgs[0]->img_loc_[1] = 40;
    imgs[0]->img_loc_[2] = 0;

    imgs[0]->world_loc_[0] = 30;
    imgs[0]->world_loc_[1] = 40;
    imgs[0]->world_loc_[2] = 0;

    vgl_box_2d<unsigned> bbox(
      vgl_point_2d<unsigned int>(0, 60),
      vgl_point_2d<unsigned int>(0, 90) );
    imgs[0]->bbox_ = bbox;

    st1->data_.set(tracking_keys::img_objs, imgs);
    trk->add_state( st1 );
    init_trks.push_back( trk );
  }

  proc_test_trans_origin.in_tracks( init_trks );

  while( p.execute() )
  {

  }
  int c = trkwriter.count_;

}

void test_crop_tracks_img_space_with_splitting( bool split_tracks )
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_crop_img_space("ptcis");
  p.add( &proc_test_crop_img_space );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_crop_img_space.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "ptcis:crop_track_img_space:disabled" , false );
  config.set_value( "ptcis:crop_track_img_space:split_cropped_tracks", split_tracks );
  config.set_value( "ptcis:crop_track_img_space:img_crop_min_x" , 20 );
  config.set_value( "ptcis:crop_track_img_space:img_crop_min_y" , 20 );
  config.set_value( "ptcis:crop_track_img_space:img_crop_max_x" , 40 );
  config.set_value( "ptcis:crop_track_img_space:img_crop_max_y" , 40 );

  TEST( "Set params", p.set_params( config ), true );

  /*
    create tracks that:

  */
  vcl_vector<track_sptr> init_trks;

  //1) 3 states all in, no change to track
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    track_state_sptr st3( new track_state );

    timestamp t1(1);
    timestamp t2(2);
    timestamp t3(3);

    st1->time_ = t1;
    st2->time_ = t2;
    st3->time_ = t3;

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs3;
    imgs3.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(20, 20),
      vgl_point_2d<unsigned int>(25, 25)
      );

    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(30, 30)
      );

    vgl_box_2d<unsigned int> testbox3(
      vgl_point_2d<unsigned int>(35, 35),
      vgl_point_2d<unsigned int>(40, 40)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;
    imgs3[0]->bbox_ = testbox3;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    st3->data_.set(tracking_keys::img_objs, imgs3);
    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    init_trks.push_back( trk );
  }
  //2) 3 states middle is cropped.  Track split or not depending on option
  {
    track_sptr trk( new track );
    trk->set_id(2);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    track_state_sptr st3( new track_state );

    timestamp t1(1);
    timestamp t2(2);
    timestamp t3(3);

    st1->time_ = t1;
    st2->time_ = t2;
    st3->time_ = t3;

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs3;
    imgs3.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );

    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(45, 45),
      vgl_point_2d<unsigned int>(50, 50)
      );

    vgl_box_2d<unsigned int> testbox3(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;
    imgs3[0]->bbox_ = testbox3;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    st3->data_.set(tracking_keys::img_objs, imgs3);
    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    init_trks.push_back( trk );
  }
  //3) 5 states, 2 cropped.  Produces 1 track with 3 state or 3 tracks with 1
  {
    track_sptr trk( new track );
    trk->set_id(3);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    track_state_sptr st3( new track_state );
    track_state_sptr st4( new track_state );
    track_state_sptr st5( new track_state );

    timestamp t1(1);
    timestamp t2(2);
    timestamp t3(3);
    timestamp t4(4);
    timestamp t5(5);

    st1->time_ = t1;
    st2->time_ = t2;
    st3->time_ = t3;
    st4->time_ = t4;
    st5->time_ = t5;

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs3;
    imgs3.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs4;
    imgs4.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs5;
    imgs5.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(30, 30),
      vgl_point_2d<unsigned int>(35, 35)
      );

    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(50, 50),
      vgl_point_2d<unsigned int>(55, 55)
      );

    vgl_box_2d<unsigned int> testbox3(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );

    vgl_box_2d<unsigned int> testbox4(
      vgl_point_2d<unsigned int>(60, 60),
      vgl_point_2d<unsigned int>(65, 65)
      );

    vgl_box_2d<unsigned int> testbox5(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;
    imgs3[0]->bbox_ = testbox3;
    imgs4[0]->bbox_ = testbox4;
    imgs5[0]->bbox_ = testbox5;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    st3->data_.set(tracking_keys::img_objs, imgs3);
    st4->data_.set(tracking_keys::img_objs, imgs4);
    st5->data_.set(tracking_keys::img_objs, imgs5);
    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    trk->add_state( st4 );
    trk->add_state( st5 );
    init_trks.push_back( trk );
  }
  //4) Crop beginning, no impact from splitting
  {
    track_sptr trk( new track );
    trk->set_id(4);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    track_state_sptr st3( new track_state );

    timestamp t1(1);
    timestamp t2(2);
    timestamp t3(3);

    st1->time_ = t1;
    st2->time_ = t2;
    st3->time_ = t3;

   vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs3;
    imgs3.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(10, 10),
      vgl_point_2d<unsigned int>(20, 20)
      );

    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );

    vgl_box_2d<unsigned int> testbox3(
      vgl_point_2d<unsigned int>(35, 35),
      vgl_point_2d<unsigned int>(40, 40)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;
    imgs3[0]->bbox_ = testbox3;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    st3->data_.set(tracking_keys::img_objs, imgs3);
    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    init_trks.push_back( trk );
  }
  //5) Crops end, no impact from splitting
  {
    track_sptr trk( new track );
    trk->set_id(5);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    track_state_sptr st3( new track_state );

    timestamp t1(1);
    timestamp t2(2);
    timestamp t3(3);

    st1->time_ = t1;
    st2->time_ = t2;
    st3->time_ = t3;

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs3;
    imgs3.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(20, 20),
      vgl_point_2d<unsigned int>(25, 25)
      );

    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );

    vgl_box_2d<unsigned int> testbox3(
      vgl_point_2d<unsigned int>(40, 40),
      vgl_point_2d<unsigned int>(45, 45)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;
    imgs3[0]->bbox_ = testbox3;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    st3->data_.set(tracking_keys::img_objs, imgs3);
    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );

    init_trks.push_back( trk );
  }

  //6) All cropped
  {
    track_sptr trk( new track );
    trk->set_id(6);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );
    track_state_sptr st3( new track_state );

    timestamp t1(1);
    timestamp t2(2);
    timestamp t3(3);

    st1->time_ = t1;
    st2->time_ = t2;
    st3->time_ = t3;

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs3;
    imgs3.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(40, 40),
      vgl_point_2d<unsigned int>(45, 45)
      );

    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(45, 45),
      vgl_point_2d<unsigned int>(50, 50)
      );

    vgl_box_2d<unsigned int> testbox3(
      vgl_point_2d<unsigned int>(35, 35),
      vgl_point_2d<unsigned int>(45, 45)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;
    imgs3[0]->bbox_ = testbox3;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    st3->data_.set(tracking_keys::img_objs, imgs3);
    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );

    init_trks.push_back( trk );
  }

  proc_test_crop_img_space.in_tracks( init_trks );

  while( p.execute() )
  {

  }
  int c = trkwriter.count_;

  vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

  if ( !split_tracks )
  {
    TEST("Cropping kept 5 tracks", c == 5, true );
  }
  else
  {
    TEST("Cropping kept 8 tracks", c == 8, true );
  }

  track_sptr t = out_tracks[0];
  if ( t )
  {
    TEST("First track is id 1", t->id() == 1 , true);
    TEST("Track 1 has 3 states", t->history().size() == 3, true);
  }

  t = out_tracks[1];
  if ( t )
  {
    TEST("Second track is id 2", t->id() == 2 , true);
    if ( !split_tracks )
    {
      TEST("Track 2 has 2 states", t->history().size() == 2, true);
    }
    else
    {
      TEST("Track 2 has 1 state", t->history().size() == 1, true);
    }
  }

  if ( !split_tracks )
  {
    t = out_tracks[2];
    if ( t )
    {
      TEST("Third track is id 3", t->id() == 3 , true);
      TEST("Track 3 has 3 states", t->history().size() == 3, true);
    }

    t = out_tracks[3];
    if ( t )
    {
      TEST("Fourth track is id 4", t->id() == 4 , true);
      TEST("Track 4 has 2 states", t->history().size() == 2, true);
    }

    t = out_tracks[4];
    if ( t )
    {
      TEST("Fifth track is id 5", t->id() == 5 , true);
      TEST("Track 5 has 2 states", t->history().size() == 2, true);
    }
  }
  else
  {
    t = out_tracks[2];
    if ( t )
    {
      TEST("Third track is id 3", t->id() == 3 , true);
      TEST("Track 3 has 1 states", t->history().size() == 1, true);
    }

    t = out_tracks[3];
    if ( t )
    {
      TEST("Fourth track is id 4", t->id() == 4 , true);
      TEST("Track 4 has 2 states", t->history().size() == 2, true);
    }

    t = out_tracks[4];
    if ( t )
    {
      TEST("Fifth track is id 5", t->id() == 5 , true);
      TEST("Track 5 has 2 states", t->history().size() == 2, true);
    }

    t = out_tracks[5];
    if ( t )
    {
      TEST("Sixth track is id 6", t->id() == 7 , true);
      TEST("Track 6 has 1 state resulting from splitting track 2", t->history().size() == 1, true);
    }

    t = out_tracks[6];
    if ( t )
    {
      TEST("Sevent track is id 7", t->id() == 8 , true);
      TEST("Track 7 has 1 state resulting from splitting track 3", t->history().size() == 1, true);
    }

    t = out_tracks[7];
    if ( t )
    {
      TEST("Fifth track is id 8", t->id() == 9 , true);
      TEST("Track 8 has 1 state resulting from splitting track 3", t->history().size() == 1, true);
    }

  }

}

void test_crop_tracks_img_space( bool split_tracks )
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_crop_img_space("ptcis");
  p.add( &proc_test_crop_img_space );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_crop_img_space.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "ptcis:crop_track_img_space:disabled" , false );
  config.set_value( "ptcis:crop_track_img_space:split_cropped_tracks", split_tracks );
  config.set_value( "ptcis:crop_track_img_space:img_crop_min_x" , 20 );
  config.set_value( "ptcis:crop_track_img_space:img_crop_min_y" , 20 );
  config.set_value( "ptcis:crop_track_img_space:img_crop_max_x" , 40 );
  config.set_value( "ptcis:crop_track_img_space:img_crop_max_y" , 40 );

  TEST( "Set params", p.set_params( config ), true );

  /*
    create tracks that:

  */
  vcl_vector<track_sptr> init_trks;

  //1) border cases, should remain a 2 state track
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(20, 20),
      vgl_point_2d<unsigned int>(40, 40)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox1;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //2) middle cases, should remain a 2 state track
  {
    track_sptr trk( new track );
    trk->set_id(2);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox1;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //3) crops one state on lower boundary
  {
    track_sptr trk( new track );
    trk->set_id(3);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(10, 10),
      vgl_point_2d<unsigned int>(35, 35)
      );
    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );
    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }
  //4) crops one state on upper boundary
  {
    track_sptr trk( new track );
    trk->set_id(4);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(35, 35)
      );
    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(25, 25),
      vgl_point_2d<unsigned int>(55, 55)
      );
    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }

  //5) crops both states, track should be NULL
  {
    track_sptr trk( new track );
    trk->set_id(5);
    track_state_sptr st1( new track_state );
    track_state_sptr st2( new track_state );

    vcl_vector< image_object_sptr > imgs1;
    imgs1.push_back(new image_object);

    vcl_vector< image_object_sptr > imgs2;
    imgs2.push_back(new image_object);

    vgl_box_2d<unsigned int> testbox1(
      vgl_point_2d<unsigned int>(15, 15),
      vgl_point_2d<unsigned int>(55, 55)
      );
    vgl_box_2d<unsigned int> testbox2(
      vgl_point_2d<unsigned int>(15, 15),
      vgl_point_2d<unsigned int>(55, 55)
      );

    imgs1[0]->bbox_ = testbox1;
    imgs2[0]->bbox_ = testbox2;

    st1->data_.set(tracking_keys::img_objs, imgs1);
    st2->data_.set(tracking_keys::img_objs, imgs2);
    trk->add_state( st1 );
    trk->add_state( st2 );
    init_trks.push_back( trk );
  }

  proc_test_crop_img_space.in_tracks( init_trks );

  while( p.execute() )
  {

  }
  int c = trkwriter.count_;
  TEST("Cropping kept 4 tracks", c == 4, true );
  vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

  track_sptr t = out_tracks[0];
  if ( t )
  {
    TEST("First track is id 1", t->id() == 1 , true);
    TEST("Track 1 has 2 states", t->history().size() == 2, true);
  }

  t = out_tracks[1];
  if ( t )
  {
    TEST("Second track is id 2", t->id() == 2 , true);
    TEST("Track 2 has 2 states", t->history().size() == 2, true);
  }

  t = out_tracks[2];
  if ( t )
  {
    TEST("Third track is id 3", t->id() == 3 , true);
    TEST("Track 3 has 1 state", t->history().size() == 1, true);
  }

  t = out_tracks[3];
  if ( t )
  {
    TEST("Fourth track is id 4", t->id() == 4 , true);
    TEST("Track 4 has 1 state", t->history().size() == 1, true);
  }
}


void test_reassign_ids()
{
  {
    sync_pipeline p;

    transform_tracks_process< vxl_byte > proc_test_renumber_tracks("ptrt");
    p.add( &proc_test_renumber_tracks );

    track_writer_process trkwriter;
    p.add( &trkwriter );

    p.connect( proc_test_renumber_tracks.out_tracks_port(), trkwriter.set_tracks_port() );

    config_block config = p.params();
    config.set_value( "ptrt:reassign_track_ids:disabled" , false );

    TEST( "Set params", p.set_params( config ), true );
    TEST( "Initialize pipeline", p.initialize(), true );

    vcl_vector<track_sptr> init_trks;
    //track_id 3
    {
      track_sptr trk( new track );
      trk->set_id(3);
      init_trks.push_back( trk );
    }
    //track_id 4
    {
      track_sptr trk( new track );
      trk->set_id(4);
      init_trks.push_back( trk );
    }
    proc_test_renumber_tracks.in_tracks( init_trks );

    while( p.execute() )
    {

    }

    int c = trkwriter.count_;
    TEST("Renumbering kept 2 tracks", c == 2, true );
    vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

    //renumbering base id was not configured which means id = 1 default.
    // the above reflects the exising behavior before the base_id concept was added.
    track_sptr t = out_tracks[0];
    if ( t )
    {
      TEST("First track is id 1", t->id() == 1, true);
    }
    t = out_tracks[1];
    if ( t )
    {
      TEST("Second track is id 2", t->id() == 2, true);
    }

  }

  {
    sync_pipeline p;

    transform_tracks_process< vxl_byte > proc_test_renumber_tracks("ptrt");
    p.add( &proc_test_renumber_tracks );

    track_writer_process trkwriter;
    p.add( &trkwriter );

    p.connect( proc_test_renumber_tracks.out_tracks_port(), trkwriter.set_tracks_port() );

    config_block config = p.params();
    config.set_value( "ptrt:reassign_track_ids:disabled" , false );

   vcl_vector<track_sptr> init_trks;
    //track_id 3
    {
      track_sptr trk( new track );
      trk->set_id(3);
      init_trks.push_back( trk );
    }
    //track_id 4
    {
      track_sptr trk( new track );
      trk->set_id(4);
      init_trks.push_back( trk );
    }

    //now let's configure the pipeline to renumber from 20
    config.set_value( "ptrt:reassign_track_ids:base_track_id" , 20 );
    TEST( "Set params again", p.set_params( config ), true );
    TEST( "Initialize pipeline", p.initialize(), true );

    proc_test_renumber_tracks.in_tracks( init_trks );

    while( p.execute() )
    {

    }

    int  c = trkwriter.count_;
    TEST("Renumbering kept 2 tracks", c == 2, true );
    vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

    track_sptr t = out_tracks[0];
    if ( t )
    {
      TEST("First track is id 20", t->id() == 20, true);
    }
    t = out_tracks[1];
    if ( t )
    {
      TEST("Second track is id 21", t->id() == 21, true);
    }

  }

}


} // end anonymous namespace


void test_time_increase()
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_trans_time( "pttt" );
  p.add( &proc_test_trans_time );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_trans_time.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "pttt:update_track_time:disabled", false );
  config.set_value( "pttt:update_track_time:time_offset", 1311 );

  TEST( "Set params", p.set_params( config ), true );


  vcl_vector<track_sptr> init_trks;
  //1)  Calculate new increased time
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    st1->time_.set_time( 0 );
    st1->time_.set_frame_number( 0 );
    track_state_sptr st2( new track_state );
    st2->time_.set_time( 0.5 * 1e6 );
    st2->time_.set_frame_number( 1 );
    track_state_sptr st3( new track_state );
    st3->time_.set_time( 1.0 * 1e6 );
    st3->time_.set_frame_number( 2 );

    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    init_trks.push_back( trk );
  }

  proc_test_trans_time.in_tracks( init_trks );

  while( p.execute() )
  {

  }
  vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

  double true_track_times [] = { 1311, 1311.5, 1312 };
  int c = 0;
  track_sptr t = out_tracks[0];
  vcl_vector<track_state_sptr> const& hist = t->history();
  vcl_vector<track_state_sptr>::const_iterator i = hist.begin();
  for( i = hist.begin(); i != hist.end(); ++i )
  {
    vcl_cout<<(*i)->time_.time_in_secs() << " " <<  true_track_times[c]<<vcl_endl;
    vcl_string msg = "Track time is " + boost::lexical_cast<vcl_string>(true_track_times[c]);
    TEST( msg.c_str(), (*i)->time_.time_in_secs() ==  true_track_times[c], true );
    c++;
  }
}

void test_time_decrease()
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_trans_time( "pttt" );
  p.add( &proc_test_trans_time );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_trans_time.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "pttt:update_track_time:disabled", false );
  config.set_value( "pttt:update_track_time:time_offset", -1311 );

  TEST( "Set params", p.set_params( config ), true );

  vcl_vector<track_sptr> init_trks;
  //1)   Calculate new decreased time
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    st1->time_.set_time( 1311 * 1e6);
    st1->time_.set_frame_number( 0 );
    track_state_sptr st2( new track_state );
    st2->time_.set_time( 1311.5 * 1e6 );
    st2->time_.set_frame_number( 1 );
    track_state_sptr st3( new track_state );
    st3->time_.set_time( 1312.0 * 1e6 );
    st3->time_.set_frame_number( 2 );

    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    init_trks.push_back( trk );
  }

  proc_test_trans_time.in_tracks( init_trks );

  while( p.execute() )
  {

  }
  vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

  double true_track_times [] = { 0, 0.5, 1.0 };
  int c = 0;
  track_sptr t = out_tracks[0];
  vcl_vector<track_state_sptr> const& hist = t->history();
  vcl_vector<track_state_sptr>::const_iterator i = hist.begin();
  for( i = hist.begin(); i != hist.end(); ++i )
  {
    vcl_string msg = "Track time is " + boost::lexical_cast<vcl_string>(true_track_times[c]);
    TEST( msg.c_str(), (*i)->time_.time_in_secs() ==  true_track_times[c], true );
    c++;
  }
}

void test_time_frame_rate_increase()
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_trans_time_frame_rate( "ptttfr" );
  p.add( &proc_test_trans_time_frame_rate );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_trans_time_frame_rate.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "ptttfr:update_track_time:disabled", false );
  config.set_value( "ptttfr:update_track_time:frame_rate", 4.0 );
  config.set_value( "ptttfr:update_track_time:time_offset", 1311 );

  TEST( "Set params", p.set_params( config ), true );

  vcl_vector<track_sptr> init_trks;
  //1)  Calculate relative time using frame rate then increase time.
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    st1->time_.set_time( 0 );
    st1->time_.set_frame_number( 0 );
    track_state_sptr st2( new track_state );
    st2->time_.set_time( 0.5 * 1e6 );
    st2->time_.set_frame_number( 1 );
    track_state_sptr st3( new track_state );
    st3->time_.set_time( 1.0 * 1e6 );
    st3->time_.set_frame_number( 2 );

    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    init_trks.push_back( trk );
  }

  proc_test_trans_time_frame_rate.in_tracks( init_trks );

  while( p.execute() )
  {

  }

  vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

  double true_track_times [] = { 1311, 1311.25, 1311.50 };
  track_sptr t = out_tracks[0];
  vcl_vector<track_state_sptr> const& hist = t->history();
  vcl_vector<track_state_sptr>::const_iterator i = hist.begin();
  int c = 0;
  for( ; i != hist.end(); ++i )
  {
    vcl_string msg = "Track time is " + boost::lexical_cast<vcl_string>(true_track_times[c]);
    TEST( msg.c_str(), (*i)->time_.time_in_secs() ==  true_track_times[c], true );
    c++;
  }
}

void test_time_frame_rate_decrease()
{
  sync_pipeline p;

  transform_tracks_process< vxl_byte > proc_test_trans_time_frame_rate( "ptttfr" );
  p.add( &proc_test_trans_time_frame_rate );

  track_writer_process trkwriter;
  p.add( &trkwriter );

  p.connect( proc_test_trans_time_frame_rate.out_tracks_port(), trkwriter.set_tracks_port() );

  config_block config = p.params();
  config.set_value( "ptttfr:update_track_time:disabled", false );
  config.set_value( "ptttfr:update_track_time:frame_rate", 4.0 );
  config.set_value( "ptttfr:update_track_time:time_offset", -1311 );

  TEST( "Set params", p.set_params( config ), true );

  vcl_vector<track_sptr> init_trks;
  //1)  Calculate new time
  {
    track_sptr trk( new track );
    trk->set_id(1);
    track_state_sptr st1( new track_state );
    st1->time_.set_time( 1311 * 1e6);
    st1->time_.set_frame_number( 0 );
    track_state_sptr st2( new track_state );
    st2->time_.set_time( 1311.5 * 1e6 );
    st2->time_.set_frame_number( 1 );
    track_state_sptr st3( new track_state );
    st3->time_.set_time( 1312.0 * 1e6 );
    st3->time_.set_frame_number( 2 );

    trk->add_state( st1 );
    trk->add_state( st2 );
    trk->add_state( st3 );
    init_trks.push_back( trk );
  }


  proc_test_trans_time_frame_rate.in_tracks( init_trks );

  while( p.execute() )
  {

  }

  vcl_vector<track_sptr> out_tracks = trkwriter.out_tracks_;

  double true_track_times [] = { -1311, -1310.75, -1310.50 };
  track_sptr t = out_tracks[0];
  vcl_vector<track_state_sptr> const& hist = t->history();
  vcl_vector<track_state_sptr>::const_iterator i = hist.begin();
  int c = 0;
  for( ; i != hist.end(); ++i )
  {
    vcl_string msg = "Track time is " + boost::lexical_cast<vcl_string>(true_track_times[c]);
    TEST( msg.c_str(), (*i)->time_.time_in_secs() ==  true_track_times[c], true );
    c++;
  }
}

//template <class PixType >
int test_transform_tracks_process( int argc, char* argv[] )
{
  testlib_test_start( "transform tracks process" );

  if( argc < 2 )
  {
    TEST( "Data directory not specified", false, true );
  }
  else
  {
    g_data_dir = argv[1];

    test_crop_tracks_by_frame();

    test_origin_translate();

    //make sure the original tests work with track splitting on and off
    test_crop_tracks_img_space( false );
    test_crop_tracks_img_space( true );

    // .. and make sure when tracks are cropped in the middle
    test_crop_tracks_img_space_with_splitting( true );
    test_crop_tracks_img_space_with_splitting( false );


    test_time_increase();

    test_time_decrease();

    test_time_frame_rate_increase();

    test_time_frame_rate_decrease();

    test_reassign_ids();

  }
  return testlib_test_summary();
}
