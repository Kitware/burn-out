/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking_data/raw_descriptor.h>
#include <tracking_data/track_sptr.h>

#include <testlib/testlib_test.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{


} // end anonymous namespace


int test_raw_descriptor( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_raw_descriptor" );

  vidtk::track_sptr t( new vidtk::track() );
  t->set_id( 1 );

  vidtk::descriptor_sptr d1 = vidtk::raw_descriptor::create( "test_desc_type" );
  TEST( "Descriptor d1 created", d1.ptr() != NULL, true );
  TEST( "Descriptor type is test_desc_type", d1->get_type(), "test_desc_type" );
  TEST( "Descriptor is not pvo by default", d1->is_pvo(), false );
  TEST( "Descriptor has 0 track ids", d1->get_track_ids().size(), 0 );

  d1->set_pvo_flag( false );
  TEST( "Descriptor is not pvo when set to false", d1->is_pvo(), false );
  d1->set_pvo_flag( true );
  TEST( "Descriptor is pvo when set to true", d1->is_pvo(), true );

  d1->add_track_id( 2 );
  std::vector< vidtk::track::track_id_t > ids = d1->get_track_ids();
  TEST( "Descriptor has 1 track id", ids.size(), 1 );
  TEST( "Descriptor track id is 2", ids[0], 2 );

  ids.push_back(3);
  TEST( "Vector of track ids is const&", d1->get_track_ids().size(), 1 );

  vidtk::descriptor_data_t data;
  data.push_back(2.0);
  d1->set_features( data );
  vidtk::descriptor_data_t& back = d1->get_features();
  TEST("Descriptor returned 1 feature with 1 element", back.size(), 1);

  back.push_back(3.0);
  TEST("Descriptor allows unfettered access to internal features", d1->get_features().size(), 2);

  TEST("Descriptor features_size call returns correct size.", d1->features_size(), 2);
  try
  {
    double &e = d1->at( 0 );
    TEST("Feature element 0 == 2.0", e, 2.0);
    e = 5.2;
    e = d1->at( 0 );
    TEST("Descriptor allows ufettered access to feature element through at call.", e, 5.2);

    e = d1->at( 3 );
    TEST("Descriptor at call out of range", false, true);
  }
  catch( std::out_of_range const& e )
  {
    TEST("Descriptor at call throws out_of_range exception", true, true);
  }

  d1->resize_features(1);
  TEST("Features_resize call shrinks data vector.", d1->features_size(), 1);

  d1->resize_features(2, 0.0);
  TEST("Features_resize increase data vector.", d1->features_size(), 2);
  TEST("Features_resize initializes new position to 0.", d1->at( 1 ), 0.0);

  return testlib_test_summary();
}
