/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_string.h>
#include <vcl_cstdio.h>
#include <testlib/testlib_test.h>
#include <descriptors/tot_collector_prob_product.h>
#include <vnl/vnl_int_2.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

using namespace vidtk;

int test_tot_collector()
{

  // ground truth
  pvo_probability_sptr v1 = new pvo_probability( 0.6, 0.2, 0.2 );
  pvo_probability_sptr v2 = new pvo_probability( 0.5, 0.4, 0.1 );
  pvo_probability_sptr v3 = new pvo_probability( 0.4, 0.3, 0.3 );
  pvo_probability_sptr gt = new pvo_probability(
    (v1->get_probability_person() * v2->get_probability_person() * v3->get_probability_person() ),
    (v1->get_probability_vehicle() * v2->get_probability_vehicle() * v3->get_probability_vehicle() ),
    (v1->get_probability_other() * v2->get_probability_other() * v3->get_probability_other() ) );
  gt->normalize();

  // track and three descriptors
  track_sptr trk( new track() );
  trk->set_id( 1 );

  // descriptor 1
  raw_descriptor::vector_t descriptors;
  descriptor_sptr des_1 = create_descriptor( "des_1", trk );
  descriptor_data_t& d1 = des_1->get_features();
  d1.push_back( v1->get_probability_person() );
  d1.push_back( v1->get_probability_vehicle() );
  d1.push_back( v1->get_probability_other() );
  descriptors.push_back( des_1 );

  // descriptor 2
  descriptor_sptr des_2 = create_descriptor( "des_2", trk );
  descriptor_data_t& d2 = des_2->get_features();
  d2.push_back( v2->get_probability_person() );
  d2.push_back( v2->get_probability_vehicle() );
  d2.push_back( v2->get_probability_other() );
  descriptors.push_back( des_2 );

  // descriptor 3
  descriptor_sptr des_3 = create_descriptor( "des_3", trk );
  descriptor_data_t& d3 = des_3->get_features();
  d3.push_back( v3->get_probability_person() );
  d3.push_back( v3->get_probability_vehicle() );
  d3.push_back( v3->get_probability_other() );
  descriptors.push_back( des_3 );

  // probability product
  class tot_collector_prob_product collector;

  pvo_probability_sptr dt = collector.classify(trk, descriptors);

  float epsilon = 1e-9;
  TEST_NEAR("TOT collector test probability person", dt->get_probability_person(),  gt->get_probability_person(), epsilon);
  TEST_NEAR("TOT collector test probability vehicle", dt->get_probability_vehicle(),  gt->get_probability_vehicle(), epsilon);
  TEST_NEAR("TOT collector test probability other", dt->get_probability_other(),  gt->get_probability_other(), epsilon);

  return 1;
}

} // end anonymous namespace

int test_tot_collector_prob_product( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "tot_collector_prob_product" );

  std::cout << "\nTesting TOT Collector by probability product \n";

  test_tot_collector();

  return testlib_test_summary();
}
