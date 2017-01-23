/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <cstdio>
#include <testlib/testlib_test.h>
#include <descriptors/tot_collector_process.h>
#include <vnl/vnl_int_2.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

int test_tot_collector_prob_product()
{
  tot_collector_process collector_process("tot_collector_prob_product");

  config_block params = collector_process.params();
  params.set( "disabled", "true");
  params.set( "method", "prob_product");
  collector_process.set_params( params );

  TEST( "Prove disabled returns when disabled", true, collector_process.set_params( params ) );

  params.set( "disabled", "false");
  params.print( std::cout );

  TEST( "Set parameters", true, collector_process.set_params( params ) );
  TEST( "Initialize", true, collector_process.initialize() );

  // ground truth
  pvo_probability_sptr v1 = new pvo_probability( 0.6, 0.2, 0.2 );
  pvo_probability_sptr v2 = new pvo_probability( 0.5, 0.4, 0.1 );
  pvo_probability_sptr v3 = new pvo_probability( 0.4, 0.3, 0.3 );

  pvo_probability_sptr gt_track1 = new pvo_probability(
    (v1->get_probability_person() * v2->get_probability_person() * v3->get_probability_person() ),
    (v1->get_probability_vehicle() * v2->get_probability_vehicle() * v3->get_probability_vehicle() ),
    (v1->get_probability_other() * v2->get_probability_other() * v3->get_probability_other() ) );
  gt_track1->normalize();

  pvo_probability_sptr gt_track2 = new pvo_probability(
    (v1->get_probability_person() * v3->get_probability_person() ),
    (v1->get_probability_vehicle() * v3->get_probability_vehicle() ),
    (v1->get_probability_other() * v3->get_probability_other() ) );
  gt_track2->normalize();

  // two tracks and three descriptors
  track_sptr track1( new track() );
  track_sptr track2( new track() );
  track1->set_id( 1 );
  track2->set_id( 2 );

  track::vector_t trks;
  trks.push_back( track1 );
  trks.push_back( track2 );

  // descriptors 1, 2, 3 on track 1
  descriptor_sptr des_1 = create_descriptor( "des_1", track1 );
  descriptor_data_t& d1 = des_1->get_features();
  d1.push_back( v1->get_probability_person() );
  d1.push_back( v1->get_probability_vehicle() );
  d1.push_back( v1->get_probability_other() );

  // descriptor 2
  descriptor_sptr des_2 = create_descriptor( "des_2", track1 );
  descriptor_data_t& d2 = des_2->get_features();
  d2.push_back( v2->get_probability_person() );
  d2.push_back( v2->get_probability_vehicle() );
  d2.push_back( v2->get_probability_other() );

  // descriptor 3
  descriptor_sptr des_3 = create_descriptor( "des_3", track1 );
  descriptor_data_t& d3 = des_3->get_features();
  d3.push_back( v3->get_probability_person() );
  d3.push_back( v3->get_probability_vehicle() );
  d3.push_back( v3->get_probability_other() );

  // descriptors 4 and 5 on track 2
  descriptor_sptr des_4 = create_descriptor( "des_4", track2 );
  descriptor_data_t& d4 = des_4->get_features();
  d4.push_back( v1->get_probability_person() );
  d4.push_back( v1->get_probability_vehicle() );
  d4.push_back( v1->get_probability_other() );

  descriptor_sptr des_5 = create_descriptor( "des_5", track2 );
  descriptor_data_t& d5 = des_5->get_features();
  d5.push_back( v3->get_probability_person() );
  d5.push_back( v3->get_probability_vehicle() );
  d5.push_back( v3->get_probability_other() );

  raw_descriptor::vector_t descriptors;
  descriptors.push_back( des_1 );
  descriptors.push_back( des_4 );
  descriptors.push_back( des_2 );
  descriptors.push_back( des_5 );
  descriptors.push_back( des_3 );

  // probability product
  collector_process.set_source_tracks( trks );
  collector_process.set_descriptors( descriptors );

  TEST("Step", collector_process.step(), true );

  float epsilon = 1e-9;
  pvo_probability pvo;
  // track 1
  TEST( "track 1 has tot", trks[0]->get_pvo( pvo ), true );

  pvo_probability dt1;
  trks[0]->get_pvo( dt1 );
  TEST_NEAR("TOT collector track 1 test probability person", dt1.get_probability_person(),  gt_track1->get_probability_person(), epsilon);
  TEST_NEAR("TOT collector track 1 test probability vehicle", dt1.get_probability_vehicle(),  gt_track1->get_probability_vehicle(), epsilon);
  TEST_NEAR("TOT collector track 1 test probability other", dt1.get_probability_other(),  gt_track1->get_probability_other(), epsilon);

  // track 2
  TEST( "track 2 has tot", trks[1]->get_pvo( pvo ), true );

  pvo_probability dt2;
  trks[1]->get_pvo( dt2 );
  TEST_NEAR("TOT collector track 2 test probability person", dt2.get_probability_person(),  gt_track2->get_probability_person(), epsilon);
  TEST_NEAR("TOT collector track 2 test probability vehicle", dt2.get_probability_vehicle(),  gt_track2->get_probability_vehicle(), epsilon);
  TEST_NEAR("TOT collector track 2 test probability other", dt2.get_probability_other(),  gt_track2->get_probability_other(), epsilon);

  return 1;
}

} // end anonymous namespace

int test_tot_collector_process( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "tot_collector_process" );

  std::cout << "\nTesting TOT Collector by probability product \n";
  test_tot_collector_prob_product();

  // other methods here

  return testlib_test_summary();
}
