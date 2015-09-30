/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/pvo_probability.h>
#include <testlib/testlib_test.h>

int test_pvo_class( int /*argc*/, char* /*argv*/[] )
{
  using namespace vidtk;

  testlib_test_start( "Test PVO class" );

  pvo_probability pvo;
  TEST_NEAR( "Default person prob is near default vehicle prob", pvo.get_probability_person(), pvo.get_probability_vehicle(), 0.00001 );
  TEST_NEAR( "Default person prob is near default other prob", pvo.get_probability_person(), pvo.get_probability_other(), 0.00001 );
  pvo = pvo_probability(1., 0.75, 0.25);
  TEST( "Probs are normalized to sum to 1",
        pvo.get_probability( pvo_probability::person ) +
        pvo.get_probability( pvo_probability::vehicle ) +
        pvo.get_probability( pvo_probability::other ), 1.0 );

  vnl_vector<double> v = pvo.get_probabilities();
  TEST_NEAR( "Correct Person: ", v[pvo_probability::person], 0.5, 0.00001 );
  TEST_NEAR( "Correct Vehicle: ", v[pvo_probability::vehicle], 0.375, 0.00001 );
  TEST_NEAR( "Correct Other: ", v[pvo_probability::other], 0.125, 0.00001 );

  pvo.set_probabilities( 0.75, 1., 0.25);
  TEST( "Probs are normalized to sum to 1",
        pvo.get_probability( pvo_probability::person ) +
        pvo.get_probability( pvo_probability::vehicle ) +
        pvo.get_probability( pvo_probability::other ), 1.0 );
  v = pvo.get_probabilities();
  TEST_NEAR( "Correct Person: ", v[pvo_probability::person], 0.375, 0.00001 );
  TEST_NEAR( "Correct Vehicle: ", v[pvo_probability::vehicle], 0.5, 0.00001 );
  TEST_NEAR( "Correct Other: ", v[pvo_probability::other], 0.125, 0.00001 );
  pvo = pvo * pvo_probability();
  v = pvo.get_probabilities();
  TEST_NEAR( "Correct Person: ", v[pvo_probability::person], 0.375, 0.00001 );
  TEST_NEAR( "Correct Vehicle: ", v[pvo_probability::vehicle], 0.5, 0.00001 );
  TEST_NEAR( "Correct Other: ", v[pvo_probability::other], 0.125, 0.00001 );

  pvo.adjust( .5, .25, .25);
  v = pvo.get_probabilities();
  TEST( "Probs are normalized to sum to 1",
        pvo.get_probability( pvo_probability::person ) +
        pvo.get_probability( pvo_probability::vehicle ) +
        pvo.get_probability( pvo_probability::other ), 1.0 );
  TEST_NEAR( "Correct Person: ", v[pvo_probability::person], 0.545454, 0.00001 );
  TEST_NEAR( "Correct Vehicle: ", v[pvo_probability::vehicle], 0.363636, 0.00001 );
  TEST_NEAR( "Correct Other: ", v[pvo_probability::other], 0.09090909, 0.00001 );
  pvo.set_probability_person(.5);
  pvo.set_probability_vehicle(0.375);
  pvo.set_probability_other(0.125);
  v = pvo.get_probabilities();
  TEST_NEAR( "Correct Person: ", v[pvo_probability::person], 0.5, 0.00001 );
  TEST_NEAR( "Correct Vehicle: ", v[pvo_probability::vehicle], 0.375, 0.00001 );
  TEST_NEAR( "Correct Other: ", v[pvo_probability::other], 0.125, 0.00001 );

  return testlib_test_summary();
}
