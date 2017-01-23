/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <iostream>
#include <fstream>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_random.h>

#include <testlib/testlib_test.h>

#include <learning/svm_learner.h>
#include <learning/learner_data_class_vector.h>
#include <learning/learner_data_class_single_vector.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

void test_svm_multiclass()
{
  unsigned int number_of_samples_per_class = 200;

  vnl_random rand(1337);
  std::vector<vidtk::learner_training_data_sptr> training_examples;
  int num_classes = 3;

  // generate 1-D training examples, with classes like this: ..._3_2_1_2_3_...
  for(unsigned int i = 0; i < number_of_samples_per_class; ++i)
  {
    // class 1
    training_examples.push_back(new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()-1), 1));

    // remaining classes
    for (int j = 2; j <= num_classes; j++)
    {
      double offset = 3 * (j - 1);
      if(rand.drand32() < 0.5)
      {
        training_examples.push_back(new vidtk::value_learner_data(vnl_vector<double>(1, rand.normal() + offset), j));
      }
      else
      {
        training_examples.push_back(new vidtk::value_learner_data(vnl_vector<double>(1, rand.normal() - (offset+1)), j));
      }
    }
  }

  // train svm model
  vidtk::svm_learner sl;
  sl.set_training_kernel_rbf(1.0/training_examples[0]->number_of_components(0));
  sl.set_training_use_probabilities(true);
  sl.train(training_examples);

  // test 1: probabilities
  // note that these are in the order of how the training labels were seen
  std::vector<int> labels_in_model;
  sl.get_labels_in_model(labels_in_model);
  vidtk::learner_data_sptr t1 = new vidtk::value_learner_data(vnl_vector<double>(1, 0.5));
  std::vector<double> probs1;
  sl.probability(*t1, probs1);
  std::cout << " probabilties: ";
  for (unsigned int i = 0; i < probs1.size(); i++)
  {
    std::cout << "class " << labels_in_model[i] << " ";
    std::cout << probs1[i] << "   ";
  }
  std::cout << std::endl;

  TEST( "probability estimate for class 1 > 0.9", probs1[0] > 0.9, true );

  // test 2: prediction
  int number_correct = 0;

  for(unsigned int i = 0; i < number_of_samples_per_class; ++i)
  {
    // class 1
    vidtk::learner_data_sptr t = new vidtk::value_learner_data(vnl_vector<double>(1,rand.normal()-1));
    if(sl.classify(*t) == 1)
    {
      number_correct++;
    }

    // remaining classes
    for (int j = 2; j <= num_classes; j++)
    {
      double offset = 3 * (j - 1);
      if(rand.drand32() < 0.5)
      {
        t = new vidtk::value_learner_data(vnl_vector<double>(1, rand.normal() + offset));
      }
      else
      {
        t = new vidtk::value_learner_data(vnl_vector<double>(1, rand.normal() - (offset+1)));
      }

      if(sl.classify(*t) == j)
      {
        number_correct++;
      }
    }
  }

  double accuracy = number_correct * 1.0 / (num_classes * number_of_samples_per_class);
  std::cout << " classification: accuracy " << accuracy << " ( " << number_correct << " of " << (num_classes * number_of_samples_per_class) << " predicted )" << std::endl;
  TEST( "SVM accuracy is greater than 0.9", accuracy > 0.9, true );

}

} // end anonymous namespace

// --------------------------------------------------------
/// entry point
int test_svm_learner( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_svm" );

  test_svm_multiclass();

  return testlib_test_summary();
}
