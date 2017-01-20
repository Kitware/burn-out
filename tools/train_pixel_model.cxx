/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/string_to_vector.h>

#include <object_detectors/obj_specific_salient_region_classifier.h>

#include <vil/vil_image_view.h>
#include <vil/vil_math.h>

#include <vul/vul_arg.h>

#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <vector>
#include <limits>

#include <logger/logger.h>

VIDTK_LOGGER( "train_pixel_model_cxx" );

using namespace vidtk;


typedef unsigned label_t;
typedef vxl_byte feature_t;
typedef double weight_t;
typedef std::string string_t;

typedef std::vector< label_t > label_vec_t;
typedef std::vector< feature_t > feature_vec_t;
typedef std::vector< feature_vec_t > feature_array_t;

typedef moving_training_data_container< feature_t > training_data_t;
typedef obj_specific_salient_region_classifier< feature_t, weight_t > model_t;


// Parse input file containing training features
bool parse_training_data( const string_t input_file,
                          const string_t delimiters,
                          const label_vec_t& ignored_features,
                          label_vec_t& labels,
                          feature_array_t& features )
{
  std::ifstream fin( input_file.c_str() );

  labels.clear();
  features.clear();

  if( !fin.is_open() )
  {
    LOG_ERROR( "Unable to open file: " << input_file );
    return EXIT_FAILURE;
  }

  const label_t max_feature_value = std::numeric_limits< feature_t >::max();

  string_t input_line;
  std::size_t feature_count = 0;
  std::size_t line_counter = 0;

  while( std::getline( fin, input_line ) )
  {
    line_counter++;
    label_vec_t parsed_line;

    if( !string_to_vector( input_line, parsed_line, delimiters ) )
    {
      LOG_ERROR( "Unable to parse training file line: " << line_counter );
      return false;
    }

    if( parsed_line.empty() )
    {
      continue;
    }

    if( feature_count == 0 )
    {
      feature_count = parsed_line.size() - 1;

      if( feature_count == 0 )
      {
        LOG_ERROR( "Training file contains no features" );
        return false;
      }
    }

    if( parsed_line.size() - 1 != feature_count )
    {
      LOG_ERROR( "File contains an inconsistent number of features on line " << line_counter );
      return false;
    }

    feature_vec_t line_features;

    for( std::size_t i = 0; i < feature_count; ++i )
    {
      label_t new_feature = parsed_line[ i + 1 ];
      bool ignore_feature = false;

      for( std::size_t j = 0; j < ignored_features.size(); ++j )
      {
        if( i == ignored_features[j] )
        {
          ignore_feature = true;
          break;
        }
      }

      if( ignore_feature )
      {
        line_features.push_back( static_cast< feature_t >( 0 ) );
      }
      else if( new_feature > max_feature_value )
      {
        LOG_WARN( "Feature value should not exceed: " << max_feature_value << ", truncating." );

        line_features.push_back( static_cast< feature_t >( max_feature_value ) );
      }
      else
      {
        line_features.push_back( static_cast< feature_t >( new_feature ) );
      }
    }

    labels.push_back( parsed_line[ 0 ] );
    features.push_back( line_features );
  }

  fin.close();

  if( features.empty() )
  {
    LOG_ERROR( "Input training file is empty" );
    return false;
  }

  return true;
}


// Count the number of instances of labels 'ids' we have in some other vector
std::size_t label_count( const label_vec_t& label_list,
                         const label_vec_t& ids )
{
  std::size_t counter = 0;

  for( std::size_t i = 0; i < label_list.size(); ++i )
  {
    if( std::find( ids.begin(), ids.end(), label_list[ i ] ) != ids.end() )
    {
      counter++;
    }
  }

  return counter;
}


// Return pointers to the start of all feature vectors for all 'ids' labels
std::vector< const feature_t* > data_for_label( const feature_array_t& features,
                                                const label_vec_t& labels,
                                                const label_vec_t& ids )
{
  std::vector< const feature_t* > output;

  for( std::size_t i = 0; i < labels.size(); ++i )
  {
    if( std::find( ids.begin(), ids.end(), labels[ i ] ) != ids.end() )
    {
      output.push_back( &( features[ i ][ 0 ] ) );
    }
  }

  return output;
}


// Refactor training data from moving training data containers to images
void refactor_data( const training_data_t& positive_samples,
                    const training_data_t& negative_samples,
                    std::vector< vil_image_view< feature_t > >& features,
                    vil_image_view< bool >& groundtruth )
{
  const std::size_t positive_count = positive_samples.size();
  const std::size_t negative_count = negative_samples.size();

  const std::size_t entries = positive_count + negative_count;
  const std::size_t feature_count = positive_samples.feature_count();

  features.resize( feature_count, vil_image_view< feature_t >( entries, 1 ) );

  groundtruth = vil_image_view< bool >( entries, 1 );

  for( std::size_t i = 0; i < positive_count; ++i )
  {
    groundtruth( i, 0 ) = true;

    for( std::size_t f = 0; f < feature_count; ++f )
    {
      ( features[ f ] )( i, 0 ) = positive_samples.entry( i )[ f ];
    }
  }

  for( std::size_t i = 0; i < negative_count; ++i )
  {
    const std::size_t adj_i = i + positive_count;

    groundtruth( adj_i, 0 ) = false;

    for( std::size_t f = 0; f < feature_count; ++f )
    {
      ( features[ f ] )( adj_i, 0 ) = negative_samples.entry( i )[ f ];
    }
  }
}


// Adjust model parameters based on groudtruth and scores
void adjust_model_weights( const vil_image_view< weight_t >& scores,
                           const vil_image_view< bool >& groundtruth,
                           model_t& model,
                           const std::size_t hist_width = 50 )
{
  // Calculate histogram binnings
  std::vector< std::size_t > pos_count( hist_width, 0 );
  std::vector< std::size_t > tot_count( hist_width, 0 );

  std::vector< double > prob_distr( hist_width, 0 );

  weight_t min_val, max_val;
  vil_math_value_range( scores, min_val, max_val );

  weight_t range = max_val - min_val;
  weight_t intvl = range / hist_width;

  if( intvl <= 0 )
  {
    LOG_ERROR( "Unable to compute probabilities, not enough data" );
    return;
  }

  for( std::size_t i = 0; i < scores.ni(); ++i )
  {
    const weight_t score = scores( i, 0 );
    std::size_t bin = ( score - min_val ) / intvl;

    if( bin >= hist_width )
    {
      bin = hist_width - 1;
    }

    if( groundtruth( i, 0 ) )
    {
      pos_count[ bin ]++;
    }

    tot_count[ bin ]++;
  }

  for( std::size_t i = 0; i < hist_width; ++i )
  {
    if( tot_count[i] > 0 )
    {
      prob_distr[i] = static_cast<weight_t>( pos_count[i] ) / tot_count[i];
    }
    else if( i > 0 )
    {
      prob_distr[i] = prob_distr[i-1];
    }

    LOG_INFO( "Bin " << i << ", Center: " << min_val + intvl * ( i + 0.5 )
      << ", Value-Ratio: " << prob_distr[i] << " " << pos_count[i] << " " << tot_count[i] );
  }

  // Ask user for input on which bins to use for interpolation
  unsigned bin1, bin2 ;
  double scale_factor;

  std::cout << std::endl << "Enter bin 1 to use for interpolation: ";
  std::cin >> bin1;
  std::cout << std::endl << "Enter bin 2 to use for interpolation: ";
  std::cin >> bin2;
  std::cout << std::endl << "Enter scale factor: ";
  std::cin >> scale_factor;
  std::cout << std::endl;

  // Interpolate new histogram weights
  double bin1_weight = min_val + intvl * ( bin1 + 0.5 );
  double bin2_weight = min_val + intvl * ( bin2 + 0.5 );

  double bin1_prob = prob_distr[ bin1 ];
  double bin2_prob = prob_distr[ bin2 ];

  double scale = scale_factor * ( bin2_prob - bin1_prob ) / ( bin2_weight - bin1_weight );
  double bias = bin1_prob - ( scale * bin1_weight );

  // Modify actual model
  LOG_INFO( "Re-weighting existing model (bias=" << bias << ", scale=" << scale << ")" );

  model.adjust_model( scale, bias );
}


int main( int argc, char** argv )
{
  // Input options and parsing
  vul_arg< string_t > input_file(
    0,
    "Input training data file. Each line should have a label "
    "followed by a feature list.",
    "" );
  vul_arg< string_t > output_file(
    0,
    "Output model file.",
    "" );
  vul_arg< string_t > positive_identifiers_str(
    "--positive-identifiers",
    "Comma seperated list of positive sample identifiers",
    "1" );
  vul_arg< string_t > negative_identifiers_str(
    "--negative-identifiers",
    "Comma seperated list of negative sample identifiers",
    "0" );
  vul_arg< string_t > ignore_identifiers_str(
    "--ignore-identifiers",
    "Comma seperated list of sample identifiers to ignore",
    "2,3,4,5,6,7,8,9,10" );
  vul_arg< string_t > ignore_features_str(
    "--ignore-features",
    "Comma seperated list of features to ignore",
    "" );
  vul_arg< string_t > file_delimiters(
    "--file-delimiters",
    "Input training data file delimiters",
    " ," );
  vul_arg< label_t > max_iter_count(
    "--max-iter-count",
    "Maximum number of algorithm iterations",
    250 );
  vul_arg< string_t > norm_method(
    "--normalization-method",
    "Classifier weight normalization method, can be either "
    "none, prob_approx, stumps_only, full_histogram.",
    "stumps_only" );
  vul_arg< string_t > weak_learner_output(
    "--output-weak-learners",
    "If set, the raw weak learner intermediate models from training "
    "will be output to this file",
    "" );

  if( argc < 3 )
  {
    vul_arg_base::display_usage_and_exit( "Invalid input provided." );
    return EXIT_FAILURE;
  }

  vul_arg_parse( argc, argv );

  label_vec_t pos_labels, neg_labels, skip_labels, features_to_ignore;

  if( !string_to_vector( positive_identifiers_str(), pos_labels ) )
  {
    LOG_ERROR( "Unable to parse positive identifiers string" );
    return EXIT_FAILURE;
  }

  if( !string_to_vector( negative_identifiers_str(), neg_labels ) )
  {
    LOG_ERROR( "Unable to parse negative identifiers string" );
    return EXIT_FAILURE;
  }

  if( !string_to_vector( ignore_identifiers_str(), skip_labels ) )
  {
    LOG_ERROR( "Unable to parse ignore identifiers string" );
    return EXIT_FAILURE;
  }

  if( !string_to_vector( ignore_features_str(), features_to_ignore ) )
  {
    LOG_ERROR( "Unable to parse ignore identifiers string" );
    return EXIT_FAILURE;
  }

  // Initialize and set training settings
  ossr_classifier_settings model_settings;

  model_settings.averaging_factor = 0.0;
  model_settings.use_external_thread = false;
  model_settings.verbose_logging = true;
  model_settings.iterations_per_update = max_iter_count();
  model_settings.weak_learner_file = weak_learner_output();
  model_settings.consolidate_weak_clfr = true;

  // Parse normalization method
  bool map_to_probabilities = false;

  if( norm_method() == "prob_approx" )
  {
    map_to_probabilities = true;
  }
  else if( norm_method() == "stumps_only" )
  {
    model_settings.norm_method = ossr_classifier_settings::STUMPS_ONLY;
  }
  else if( norm_method() == "full_histogram" )
  {
    model_settings.norm_method = ossr_classifier_settings::FULL_HISTOGRAM;
  }
  else if( norm_method() != "none" )
  {
    LOG_ERROR( "Invalid normalization option " << norm_method() );
    return EXIT_FAILURE;
  }

  // Load training data and convert it into the format for the training algorithm
  LOG_INFO( "Loading training data" );

  training_data_t positive_samples;
  training_data_t negative_samples;

  // Scoped to remove deallocate extra memory when we're done with it
  {
    label_vec_t training_labels;
    feature_array_t training_data;

    if( !parse_training_data( input_file(),
                              file_delimiters(),
                              features_to_ignore,
                              training_labels,
                              training_data ) )
    {
      return EXIT_FAILURE;
    }

    moving_training_data_settings mtd_settings;
    mtd_settings.features_per_entry = training_data[0].size();
    mtd_settings.insert_mode = moving_training_data_settings::FIFO;

    std::size_t positive_count = label_count( training_labels, pos_labels );
    std::size_t negative_count = label_count( training_labels, neg_labels );

    if( positive_count == 0 )
    {
      LOG_ERROR( "No positive training entries!" );
      return EXIT_FAILURE;
    }

    if( negative_count == 0 )
    {
      LOG_ERROR( "No negative training entries!" );
      return EXIT_FAILURE;
    }

    mtd_settings.max_entries = positive_count;
    positive_samples.configure( mtd_settings );

    mtd_settings.max_entries = negative_count;
    negative_samples.configure( mtd_settings );

    positive_samples.insert( data_for_label( training_data, training_labels, pos_labels ) );
    negative_samples.insert( data_for_label( training_data, training_labels, neg_labels ) );
  }

  // Train model
  model_t model;

  if( !model.configure( model_settings ) )
  {
    LOG_ERROR( "Unable to configure model trainer" );
    return EXIT_FAILURE;
  }

  model.update_model( positive_samples, negative_samples );

  if( !model.is_valid() )
  {
    LOG_ERROR( "Error training model, learned model is invalid" );
    return EXIT_FAILURE;
  }

  if( !model.is_learned_model() )
  {
    LOG_ERROR( "Error training model, model is not a learned model" );
    return EXIT_FAILURE;
  }

  // Perform probability normalization if enabled
  if( map_to_probabilities )
  {
    LOG_INFO( "Mapping to probabilities" );

    // Use image intermediates to represent data since that is what the
    // model class is currently designed to utilize.
    std::vector< vil_image_view< feature_t > > features;
    vil_image_view< bool > groundtruth;
    vil_image_view< weight_t > scores;

    refactor_data( positive_samples, negative_samples, features, groundtruth );

    // Clear prior buffers, now unused, to conserve memory
    positive_samples = training_data_t();
    negative_samples = training_data_t();

    // Classify data according to learned classifier
    model.classify_images( features, scores );

    // Clear prior buffers, now unused, to conserve memory
    features.clear();

    // Populate histograms and adjust model weights accordingly
    adjust_model_weights( scores, groundtruth, model );
  }

  // Output resultant model
  std::ofstream fout( output_file().c_str() );

  if( !fout.is_open() )
  {
    LOG_ERROR( "Unable to open file: " << output_file() );
    return EXIT_FAILURE;
  }

  fout << model;
  fout.close();

  LOG_INFO( "Training complete" );
  return EXIT_SUCCESS;
}
