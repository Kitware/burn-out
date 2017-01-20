/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vil/vil_image_view.h>

#include <iostream>
#include <fstream>

#include <testlib/testlib_test.h>

#include <descriptors/online_track_type_classifier.h>

namespace
{

using namespace vidtk;

// Test class to make sure each track sees the correct number of descriptors
class simple_test_classifier_1 : public online_track_type_classifier
{
public:

  simple_test_classifier_1() {}
  ~simple_test_classifier_1() {}

  object_class_sptr_t classify( const track_sptr_t& track_object,
                                const raw_descriptor::vector_t& descriptors )
  {
    object_class_sptr_t new_score( new object_class_t( 0.33, 0.33, 0.33 ) );
    observed_descriptors[ track_object->id() ] += descriptors.size();
    return new_score;
  }

  std::map< track::track_id_t, unsigned > observed_descriptors;
};

// Test class to make sure classifications from prior states are re-appended
// to tracks when there is no classification made on the current frame
class simple_test_classifier_2 : public online_track_type_classifier
{
public:

  simple_test_classifier_2() {}
  ~simple_test_classifier_2() {}

  object_class_sptr_t classify( const track_sptr_t& track_object,
                                const raw_descriptor::vector_t& descriptors )
  {
    if( descriptors.empty() || observed_tracks.find( track_object->id() ) != observed_tracks.end() )
    {
      return object_class_sptr_t( NULL );
    }

    object_class_sptr_t new_score( new object_class_t( 0.2, 0.2, 0.6 ) );
    observed_tracks.insert( track_object->id() );
    return new_score;
  }

  void terminate_track( const track_sptr_t& track_object )
  {
    terminated_tracks.insert( track_object->id() );
  }

  std::set< track::track_id_t > terminated_tracks;
  std::set< track::track_id_t > observed_tracks;
};

void test_descriptor_alignment()
{
  track_sptr track1( new track() );
  track_sptr track2( new track() );
  track_sptr track3( new track() );

  track1->set_id( 1 );
  track2->set_id( 2 );
  track3->set_id( 3 );

  descriptor_sptr descriptor1 = create_descriptor( "test1", track1 );
  descriptor_sptr descriptor2 = create_descriptor( "test2", track1 );
  descriptor_sptr descriptor3 = create_descriptor( "test3", track2 );
  descriptor_sptr descriptor4 = create_descriptor( "test4", track2 );
  descriptor_sptr descriptor5 = create_descriptor( "test5", track3 );
  descriptor_sptr descriptor6 = create_descriptor( "test6", track3 );
  descriptor_sptr descriptor7 = create_descriptor( "test7", track3 );

  track::vector_t frame_1_tracks;
  track::vector_t frame_2_tracks;

  raw_descriptor::vector_t frame_1_descriptors;
  raw_descriptor::vector_t frame_2_descriptors;

  frame_1_tracks.push_back( track1 );
  frame_1_tracks.push_back( track2 );

  frame_2_tracks.push_back( track1 );
  frame_2_tracks.push_back( track2 );
  frame_2_tracks.push_back( track3 );

  frame_1_descriptors.push_back( descriptor1 );
  frame_2_descriptors.push_back( descriptor2 );

  frame_1_descriptors.push_back( descriptor3 );
  frame_1_descriptors.push_back( descriptor4 );

  frame_2_descriptors.push_back( descriptor5 );
  frame_2_descriptors.push_back( descriptor6 );
  frame_2_descriptors.push_back( descriptor7 );

  simple_test_classifier_1 test_classifier;

  test_classifier.apply( frame_1_tracks, frame_1_descriptors );

  TEST( "Descriptor count 1", test_classifier.observed_descriptors[1], 1 );
  TEST( "Descriptor count 2", test_classifier.observed_descriptors[2], 2 );
  TEST( "Descriptor count 3", test_classifier.observed_descriptors[3], 0 );

  test_classifier.apply( frame_2_tracks, frame_2_descriptors );

  TEST( "Descriptor count 4", test_classifier.observed_descriptors[1], 2 );
  TEST( "Descriptor count 5", test_classifier.observed_descriptors[2], 2 );
  TEST( "Descriptor count 6", test_classifier.observed_descriptors[3], 3 );
}

void test_skipped_classification()
{
  track_sptr track1( new track() );
  track_sptr track2( new track() );
  track_sptr track3( new track() );

  track1->set_id( 1 );
  track2->set_id( 2 );
  track3->set_id( 3 );

  descriptor_sptr descriptor1 = create_descriptor( "desc1", track1 );
  descriptor_sptr descriptor2 = create_descriptor( "desc2", track2 );
  descriptor_sptr descriptor3 = create_descriptor( "desc3", track1 );

  track::vector_t frame_1_tracks;
  track::vector_t frame_2_tracks;
  track::vector_t frame_3_tracks;

  raw_descriptor::vector_t frame_1_descriptors;
  raw_descriptor::vector_t frame_2_descriptors;
  raw_descriptor::vector_t frame_3_descriptors;

  frame_1_tracks.push_back( track1 );
  frame_1_tracks.push_back( track2 );
  frame_1_tracks.push_back( track3 );

  frame_1_descriptors.push_back( descriptor1 );
  frame_1_descriptors.push_back( descriptor2 );

  simple_test_classifier_2 test_classifier;

  test_classifier.apply( frame_1_tracks, frame_1_descriptors );

  pvo_probability pvo;
  TEST( "Valid Classifier Value 1", track1->get_pvo( pvo ), true );
  TEST( "Valid Classifier Value 2", track2->get_pvo( pvo ), true );
  TEST( "Valid Classifier Value 3", track3->get_pvo( pvo ), false );

  track1 = track_sptr( new track() );
  track2 = track_sptr( new track() );
  track3 = track_sptr( new track() );

  track1->set_id( 1 );
  track2->set_id( 2 );
  track3->set_id( 3 );

  frame_2_tracks.push_back( track1 );
  frame_2_tracks.push_back( track2 );
  frame_2_tracks.push_back( track3 );

  frame_2_descriptors.push_back( descriptor3 );

  test_classifier.apply( frame_2_tracks, frame_2_descriptors );

  TEST( "Valid Classifier Value 4", track1->get_pvo( pvo ), true );
  TEST( "Valid Classifier Value 5", track2->get_pvo( pvo ), true );
  TEST( "Valid Classifier Value 6", track3->get_pvo( pvo ), false );

  test_classifier.apply( frame_3_tracks, frame_3_descriptors );

  TEST( "Terminate Called 1", test_classifier.terminated_tracks.count( 1 ) > 0, true );
  TEST( "Terminate Called 2", test_classifier.terminated_tracks.count( 2 ) > 0, true );
  TEST( "Terminate Called 3", test_classifier.terminated_tracks.count( 3 ) > 0, true );
}

} // end anonymous namespace

int test_online_track_type_classifier( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_online_track_type_classifier" );

  test_descriptor_alignment();
  test_skipped_classification();

  return testlib_test_summary();
}
