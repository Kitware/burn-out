/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_random.h>

#include <testlib/testlib_test.h>

#include <learning/adaboost.h>
#include <learning/histogram_weak_learners.h>
#include <learning/learner_data_class_vector.h>
#include <learning/learner_data_class_single_vector.h>
#include <learning/linear_weak_learners.h>
#include <learning/gaussian_weak_learners.h>
#include <learning/stump_weak_learners.h>
#include <learning/learner_data_class_single_vector.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace {

void test_adaboost_stump()
{
  vnl_random rand(1337);
  vcl_vector<vidtk::learner_training_data_sptr> training_examples;
  unsigned int number_of_samples_per_class = 200;
  for(unsigned int i = 0; i < number_of_samples_per_class; ++i)
  {
    training_examples.push_back(new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()-1), 1));
    if(rand.drand32() < 0.5)
      training_examples.push_back(new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()+3), -1));
    else
      training_examples.push_back(new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()-4), -1));
  }

  vcl_vector<vidtk::weak_learner_sptr> weak_learner_factories;
  vidtk::weak_learner_sptr factory = new vidtk::stump_weak_learner( "stump",0);
  weak_learner_factories.push_back(factory);
  vidtk::adaboost ada(weak_learner_factories,80);
  ada.train(training_examples);
  ada.trainPlatt(training_examples);
  double number_positive = 0;
  double number_negative = 0;

  for(unsigned int i = 0; i < number_of_samples_per_class; ++i)
  {
    vidtk::learner_data_sptr t = new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()-1));
    if(ada.classify(*t) == 1)
    {
      number_positive++;
    }
    if(rand.drand32() < 0.5)
      t = new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()+3));
    else
      t = new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()-4));
    if(ada.classify(*t) == -1)
    {
      number_negative++;
    }
  }

  vcl_cout << number_positive/number_of_samples_per_class << " " << number_negative/number_of_samples_per_class << vcl_endl;

  TEST( "Number correct positive is greater than 0.9", number_positive/number_of_samples_per_class > 0.9, true );
  TEST( "The number of correct negatives is greater than 0.85", number_negative/number_of_samples_per_class > 0.85, true );
}

} // end anonymous namespace

int test_adaboost_learner( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_adaboost" );

  test_adaboost_stump();

  return testlib_test_summary();
}
