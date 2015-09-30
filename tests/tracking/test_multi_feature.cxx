/*ckwg +5
* Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
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

#include <vil/vil_load.h>

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

  image_object_sptr object2d( double x, double y, double area,
                              vil_image_view<vxl_byte> & img)
  {
    image_object_sptr obj = new image_object;
    obj->img_loc_ = vnl_double_2(x,y);
    obj->world_loc_ = vnl_double_3(x,y,0);
    obj->area_ = area;

    image_histogram<vxl_byte, bool> hist( img,
    vcl_vector<unsigned>( img.nplanes(), 8 ) );
    obj->data_.set( tracking_keys::histogram, hist );

    return obj;
  }


  void test_multi_feature(vcl_string & data_directory)
  {
    da_tracker_process tracker( "trk" );


    //// Two image objects, one for true target, one for false detection
    vil_image_view<vxl_byte> img_target;
    vil_image_view<vxl_byte> img_false_detection;
    vcl_string img_target_filename;
    vcl_string img_false_detection_filename;
    img_target_filename = data_directory + "/target.png";
    img_false_detection_filename = data_directory + "/false_detection.png";
    img_target = vil_load(img_target_filename.c_str());
    TEST("Load target object image: ", (img_target != NULL) ,
      true);
    img_false_detection = vil_load(img_false_detection_filename.c_str());
    TEST("Load false detection object image: ", (img_false_detection != NULL) ,
      true);


    //// Set the multi-feature configurations
    vcl_string feature_filename;
    feature_filename = data_directory + "/online_features.txt";
    multiple_features mf;
    mf.w_area = 0.2;
    mf.w_color = 0.3;
    mf.w_kinematics = 0.5;
    mf.test_enabled = true;
    TEST("Load multi-feature files: ", mf.load_cdfs_params(
      feature_filename.c_str()) ,
      true);
    tracker.set_mf_params(mf);
    TEST( "Set parameters min_area_similarity_for_da to 0.0",
        tracker.set_params(
          tracker.params()
          .set_value( "min_area_similarity_for_da", 0.0 ) ),
        true );
    TEST( "Set parameters min_color_similarity_for_da to 0.0",
        tracker.set_params(
          tracker.params()
          .set_value( "min_color_similarity_for_da", 0.0 ) ),
        true );


    //// Initialize with one track
    da_so_tracker_generator generator;
    generator.set_to_default();
    vcl_vector< track_sptr > init_trks;
    vcl_vector< da_so_tracker_sptr > init_trkers;
    vcl_vector< da_so_tracker_sptr > empty_init_trkers;

    track_sptr trk( new track );
    track_state_sptr st( new track_state );
    st->vel_ = vnl_double_3( 0.0, 0.0, 0.0 );
    st->loc_ = vnl_double_3( 2.0, 2.0, 0.0 );
    timestamp ts;
    ts.set_frame_number( 0 );
    ts.set_time( 0 );
    st->time_ = ts;
    // create image object and meanwhile compute the histogram
    image_object_sptr obj_init_target = object2d( 2.0, 2.0, 2.0, img_target);
    image_object_sptr obj_init_false_detection = object2d( 2.0, 2.0, 2.0, img_false_detection);

    st->data_.set(tracking_keys::img_objs,
      vcl_vector< image_object_sptr >( 1, obj_init_target ) );
    trk->add_state( st );
    init_trks.push_back( trk );
    init_trkers.push_back( generator.generate(trk) );
    TEST( "Initialize", tracker.initialize(), true );
    vcl_vector< track_sptr > active_trks;
    tracker.set_new_trackers( init_trkers );



    // Frame 1: There is true target detection,
    // with one confusing detection having same
    // location and area, but different color
    // multi-feature should pick the true target detection
    vcl_vector< image_object_sptr > objs;
    vcl_vector< image_object_sptr > track_objs;
    image_histogram<vxl_byte, bool> hist;
    image_histogram<vxl_byte, bool> track_hist;
    ts.set_frame_number( 1 );
    ts.set_time( 1 );
    objs.clear();
    objs.push_back( object2d( 2.0, 2.0, 2.0, img_target) );
    objs.push_back( object2d( 2.0, 2.0, 2.0, img_false_detection ) );
    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    TEST( "Step 1, data association", tracker.step(), true );
    track_objs.clear();
    tracker.active_trackers()[0]->get_track()->history().back()->data_.get(tracking_keys::img_objs,track_objs);
    track_objs[0]->data_.get(tracking_keys::histogram,track_hist);
    objs[0]->data_.get(tracking_keys::histogram,hist);
    bool ret = vcl_fabs(hist.compare(track_hist.get_h()) - 1.0) < 1e-6;
    TEST( "Distinguish color difference, find true object",
          ret,
          true );
    active_trks = tracker.active_tracks();


    // Frame 2: There is true target detection,
    // with one confusing detection having same
    // location and color, but different area
    // multi-feature should pick the true target detection
    ts.set_frame_number( 2 );
    ts.set_time( 2 );
    objs.clear();
    objs.push_back( object2d( 2.0, 2.0, 2.0, img_target) );
    objs.push_back( object2d( 2.0, 2.0, 4.0, img_target ) );
    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 2, data association", tracker.step(), true );
    objs.clear();
    tracker.active_trackers()[0]->get_track()->history().back()->data_.get(tracking_keys::img_objs,objs);
    ret = vcl_fabs(objs[0]->area_- 2.0) < 1e-6;
    TEST( "Distinguish area difference, find true object",
          ret,
          true );
    active_trks = tracker.active_tracks();


    // Frame 3: There is true target detection,
    // with one confusing detection having same
    // area and color, but different location
    // multi-feature should pick the true target detection
    ts.set_frame_number( 3 );
    ts.set_time( 3 );
    objs.clear();
    objs.push_back( object2d( 2.0, 2.0, 2.0, img_target) );
    objs.push_back( object2d( 1.9, 1.9, 2.0, img_target ) );
    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 3, data association", tracker.step(), true );
    vnl_double_2 posterior_loc;
    tracker.active_trackers()[0]->get_current_location(posterior_loc);
    ret = (posterior_loc - vnl_double_2(2.0, 2.0)).magnitude() < 1e-6;
    TEST( "Distinguish location difference, find true object",
           ret,
           true );
    active_trks = tracker.active_tracks();


    // Frame 4: There is no true target detection,
    // there is one confusing detection having same
    // area and location but "very" different color,
    // multi-feature should reject this detection
    TEST( "Set parameters min_color_similarity_for_da to 0.5",
        tracker.set_params(
          tracker.params()
          .set_value( "min_color_similarity_for_da", 0.5 ) ),
        true );
    ts.set_frame_number( 4 );
    ts.set_time( 4 );
    objs.clear();
    objs.push_back( object2d( 2.0, 2.0, 2.0, img_false_detection) );
    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 4: data association", tracker.step(), true );
    TEST( "Reject false detecion due to color",
          (tracker.active_trackers()[0]->missed_count() == 1),
          true );
    active_trks = tracker.active_tracks();

    // Frame 5: There is no true target detection,
    // there is one confusing detection having same
    // color and location but "very" different area,
    // multi-feature should reject this detection
    TEST( "Set parameters min_area_similarity_for_da to 0.5",
        tracker.set_params(
          tracker.params()
          .set_value( "min_area_similarity_for_da", 0.5 ) ),
        true );
    ts.set_frame_number( 5 );
    ts.set_time( 5 );
    objs.clear();
    objs.push_back( object2d( 2.0, 2.0, 20.0, img_target) );
    tracker.set_timestamp( ts );
    tracker.set_objects( objs );
    tracker.set_active_tracks( active_trks );
    tracker.set_new_trackers( empty_init_trkers );
    TEST( "Step 5: data association", tracker.step(), true );
    TEST( "Reject false detecion due to area",
          tracker.active_trackers()[0]->missed_count() == 2,
          true );
    active_trks = tracker.active_tracks();
  }
} // end anonymous namespace


int test_multi_feature( int argc, char* argv[] )
{
  using namespace vidtk;

  testlib_test_start( "multi feature" );
  if( argc < 2)
  {
    TEST( "DATA directory not specified", false, true);
    return EXIT_FAILURE;
  }

  vcl_string data_directory(argv[1]);
  test_multi_feature(data_directory);

  return testlib_test_summary();
}
