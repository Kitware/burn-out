/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_iostream.h>
#include <vcl_fstream.h>
#include <vcl_vector.h>

#include <testlib/testlib_test.h>

#include <descriptors/bag_of_words_model.h>

// Put everything in an anonymous namespace so that different tests
// won't conflict.
namespace
{

using namespace vidtk;

void test_bag_of_words_model()
{
  // BoW mapping settings.
  bow_mapping_settings options;
  options.voting_method_ = bow_mapping_settings::SINGLE_VOTE_UNWEIGHTED;
  options.norm_type_ = bow_mapping_settings::L1;

  // Create dummy external BoW model as opposed to loading from a file.
  vcl_vector< vcl_vector< double > > model( 3 );
  model[0].push_back( 2 );
  model[1].push_back( 6 );
  model[2].push_back( 50 );

  bag_of_words_model< double, double > bow_vocab;
  TEST( "Set external model test", bow_vocab.set_model( model ), true );

  // Compute descriptor
  vcl_vector< double > input_features( 10, 40 );
  input_features[0] = 3;
  input_features[1] = 7;
  input_features[2] = 7;
  vcl_vector< double > output_descriptor;
  bool status = bow_vocab.map_to_model( input_features, options, output_descriptor );

  TEST( "Was mapping successful", status, true );
  TEST( "Output descriptor size", output_descriptor.size(), 3 );
  TEST( "Output descriptor value 1", output_descriptor[0], 0.1 );
  TEST( "Output descriptor value 2", output_descriptor[1], 0.2 );
  TEST( "Output descriptor value 3", output_descriptor[2], 0.7 );
}

} // end anonymous namespace

int test_bag_of_words_model( int /*argc*/, char* /*argv*/[] )
{
  testlib_test_start( "test_bag_of_words_model" );

  test_bag_of_words_model();

  return testlib_test_summary();
}
