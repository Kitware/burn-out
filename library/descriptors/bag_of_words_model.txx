/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "bag_of_words_model.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <limits>

#include <logger/logger.h>

#include <boost/lexical_cast.hpp>

#ifdef USE_VLFEAT
extern "C"
{
  #include <vl/kmeans.h>
}
#endif

namespace vidtk
{

VIDTK_LOGGER( "bag_of_words_model_txx" );

// Load a BoW vocab from a given file
template < typename WordType, typename MappingType >
bool bag_of_words_model< WordType, MappingType >
::load_model( const std::string& filename )
{
  is_valid_ = false;
  try
  {
    // Open file
    std::ifstream input( filename.c_str() );

    if( !input )
    {
      LOG_ERROR( "Unable to open " << filename << "!" );
      return false;
    }

    // Parse file header, if it exists
    unsigned word_entries, numbers_per_word;
    std::string header;
    input >> header;

    if( header == "BOW_MODEL_FILE" || header == "ASCII_BOW_FILE" )
    {
      input >> word_entries >> numbers_per_word;
      LOG_INFO( "Found '" << header << "' with " << word_entries << " words of dimension " << numbers_per_word );
    }
    else
    {
      word_entries = boost::lexical_cast<unsigned>( header );
      input >> numbers_per_word;
      LOG_INFO( "Found unrecognized header '" << header << "' ; assuming dimension " << numbers_per_word );
    }

    if( word_entries == 0 || numbers_per_word == 0 )
    {
      LOG_ERROR( "BoW file " << filename << " contains invalid header information." );
      return false;
    }

    vocabulary_.clear();
    vocabulary_.resize( word_entries, std::vector< WordType >(numbers_per_word) );

    // Read in model
    for( unsigned int i = 0; i < word_entries; i++)
    {
      for( unsigned int j = 0; j < numbers_per_word; j++)
      {
        input >> vocabulary_[i][j];

        if( input.eof() )
        {
          LOG_ERROR( "BoW file " << filename << " contains incomplete data." );
          return false;
        }
      }
    }

    // Close file
    input.close();
  }
  catch(...)
  {
    LOG_ERROR( "Unable to read " << filename << "!" );
    return false;
  }

  // Mark internal model as being valid
  is_valid_ = true;
  return true;
}

// Set an external model.
template < typename WordType, typename MappingType >
bool bag_of_words_model< WordType, MappingType >
::set_model( const vocabulary_type& model )
{
  vocabulary_ = model;
  is_valid_ = true;
  return true;
}

// Map input features unto a BoW model
template < typename WordType, typename MappingType >
bool bag_of_words_model< WordType, MappingType >
::map_to_model( const input_type& features,
                const bow_mapping_settings& options,
                output_type& mapping_output ) const
{
  // Validate inputs
  LOG_ASSERT( features.size() % features_per_word() == 0,
              "Input feature vector and model sizes do not match!" );

  // Run internal algorithm
  const unsigned word_length = this->features_per_word();
  const unsigned input_word_count = features.size() / word_length;

  std::vector< const WordType* > input_word_ptrs( input_word_count );
  for( unsigned i = 0; i < input_word_count; i++ )
  {
    input_word_ptrs[i] = &features[0] + i * word_length;
  }

  return this->map_to_model( &input_word_ptrs[0],
                             input_word_count,
                             options,
                             mapping_output );
}

// Map input features unto a BoW model
template < typename WordType, typename MappingType >
bool bag_of_words_model< WordType, MappingType >
::map_to_model( const input_vector_type& features,
                const bow_mapping_settings& options,
                output_type& mapping_output ) const
{
  // Validate inputs
  LOG_ASSERT( features.size() > 0 && features[0].size() == features_per_word(),
              "Input feature vector and model sizes do not match!" );

  // Run internal algorithm
  const unsigned input_word_count = features.size();

  std::vector< const WordType* > input_word_ptrs( input_word_count );
  for( unsigned i = 0; i < input_word_count; i++ )
  {
    input_word_ptrs[i] = &(features[i][0]);
  }

  return this->map_to_model( &input_word_ptrs[0],
                             input_word_count,
                             options,
                             mapping_output );
  return true;
}

// Map input features unto a BoW model (real definition)
template < typename WordType, typename MappingType >
bool bag_of_words_model< WordType, MappingType >
::map_to_model( const WordType** features,
                const unsigned num_features,
                const bow_mapping_settings& options,
                output_type& mapping_output ) const
{
  // Reset output vector
  const int word_size = static_cast<int>( this->features_per_word() );
  const int vocab_size = static_cast<int>( vocabulary_.size() );
  mapping_output.resize( vocabulary_.size() );
  std::fill( mapping_output.begin(), mapping_output.end(), 0 );

  // Vote into bow descriptor
  if( options.voting_method_ == bow_mapping_settings::TOP_3_WEIGHTED )
  {
    for( unsigned int i = 0; i < num_features; i++ )
    {
      const WordType *input_word = features[i];

      MappingType top_scores[4] = {-2, -2, -2, -2};
      int top_word_ids[4] = {-1, -1, -1, -1};

      for( int j = 0; j < vocab_size; j++ )
      {
        int k;
        MappingType sum = 0;

        for( k = 0; k < word_size; k++ )
        {
          sum += input_word[k] * vocabulary_[j][k];
        }

        for( k = 2; k >= 0; k-- )
        {
          if( sum > top_scores[k] )
          {
            top_scores[k+1] = top_scores[k];
            top_word_ids[k+1] = top_word_ids[k];
            top_scores[k] = sum;
            top_word_ids[k] = j;
          }
          else
          {
            break;
          }
        }
      }
      mapping_output[top_word_ids[0]] += top_scores[0];
      mapping_output[top_word_ids[1]] += top_scores[1] * 0.5f;
      mapping_output[top_word_ids[2]] += top_scores[2] * 0.25f;
    }
  }
  else if( options.voting_method_ == bow_mapping_settings::SINGLE_VOTE_UNWEIGHTED )
  {
    for( unsigned int i = 0; i < num_features; i++ )
    {
      int best_id = -1;
      MappingType min_dist = std::numeric_limits< MappingType >::max();

      for( int j = 0; j < vocab_size; j++ )
      {
        MappingType sum = 0.0;

        for( int k = 0; k < word_size; k++ )
        {
          MappingType t1 = vocabulary_[ j ][ k ] - features[ i ][ k ];
          sum += t1 * t1;
        }

        if( sum < min_dist )
        {
          min_dist = sum;
          best_id = j;
        }
      }
      mapping_output[ best_id ]++;
    }
  }

  // Normalization
  if( options.norm_type_ == bow_mapping_settings::NONE )
  {
    return true;
  }

  MappingType sum = 0;
  if( options.norm_type_ == bow_mapping_settings::L1 )
  {
    for( int i = 0; i < vocab_size; i++)
    {
      sum += std::fabs(mapping_output[i]);
    }
  }
  else if( options.norm_type_ == bow_mapping_settings::L2 )
  {
    for( int i = 0; i < vocab_size; i++)
    {
      sum += mapping_output[i] * mapping_output[i];
    }
    sum = std::sqrt(sum);
  }
  if( sum != 0 )
  {
    for( int i = 0; i < vocab_size; i++ )
    {
      mapping_output[i] = mapping_output[i] / sum;
    }
  }
  return true;
}

// Training functions, TODO: replace VLFeat clustering with vxl version if it's as fast
template < typename WordType, typename MappingType >
bool bag_of_words_model< WordType, MappingType >
::train_model( const input_type& features,
               const bow_training_settings& options )
{
  LOG_ASSERT( options.clustering_method_ == bow_training_settings::KMEANS,
              "Unsupported clustering method inputted." );

  const unsigned input_feature_count = features.size() / options.bins_per_word_;

  LOG_INFO( "Beginning training on " << input_feature_count << " input features." );

#ifdef USE_VLFEAT

  // Downsize and reformat data
  const unsigned downsampled_feature_count = input_feature_count / options.downsampling_rate_;
  std::vector<float> downsampled_data( downsampled_feature_count * options.bins_per_word_ );

  for( unsigned i = 0; i < downsampled_feature_count; i++ )
  {
    unsigned input_feature_start = i * options.bins_per_word_ * options.downsampling_rate_;
    unsigned output_feature_start = i * options.bins_per_word_;

    for( unsigned j = 0; j < options.bins_per_word_; j++ )
    {
      downsampled_data[ output_feature_start + j ] = static_cast<float>( features[ input_feature_start + j ] );
    }
  }

  // Run KMeans
  VlKMeansAlgorithm algorithm = VlKMeansElkan;
  VlVectorComparisonType distance = VlDistanceL2;
  VlKMeansInitialization init = VlKMeansPlusPlus;

  VlKMeans *kmeans = vl_kmeans_new( VL_TYPE_FLOAT, distance );

  vl_kmeans_set_algorithm( kmeans, algorithm );
  vl_kmeans_set_max_num_iterations( kmeans, options.max_iter_ );
  vl_kmeans_set_initialization( kmeans, init );
  vl_kmeans_set_num_repetitions( kmeans, 1 );
  vl_kmeans_set_verbosity( kmeans, options.verbosity_ );

  LOG_INFO( "Beginning clustering" );

  vl_kmeans_cluster( kmeans,
                     &downsampled_data[0],
                     options.bins_per_word_,
                     downsampled_feature_count,
                     options.word_count_ );

  LOG_INFO( "Clustering finished" );

  const float *output_centers = reinterpret_cast<const float*>(vl_kmeans_get_centers( kmeans ));

  // Copy clusters from vlfeat machine to a more standard vector format
  vocabulary_.resize( options.word_count_, std::vector<WordType>( options.bins_per_word_ ) );

  for( unsigned i = 0; i < options.word_count_; i++ )
  {
    for( unsigned j = 0; j < options.bins_per_word_; j++ )
    {
      vocabulary_[i][j] = static_cast<WordType>( output_centers[ i*options.bins_per_word_ + j ] );
    }
  }

  vl_kmeans_delete( kmeans);

  if( options.normalize_output_vocab_ )
  {
    for( unsigned i = 0; i < options.word_count_; i++ )
    {
      WordType sum = 0;

      for( unsigned j = 0; j < options.bins_per_word_; j++ )
      {
        sum += ( vocabulary_[i][j] * vocabulary_[i][j] );
      }
      sum = std::sqrt( sum );
      if( sum != 0 )
      {
        for( unsigned j = 0; j < options.bins_per_word_; j++ )
        {
          vocabulary_[i][j] /= sum;
        }
      }
    }
  }

  if( vocabulary_.size() > 0 && vocabulary_[0].size() == options.bins_per_word_ )
  {
    is_valid_ = true;
  }
  return true;
#else
  LOG_FATAL( "BoW train_model function currently requires a build with VLFeat" );
  return false;
#endif
}

template < typename WordType, typename MappingType >
bool bag_of_words_model< WordType, MappingType >
::train_model( const input_vector_type& features,
               const bow_training_settings& options )
{
  std::vector< WordType > input_vector;
  for( unsigned i = 0; i < features.size(); i++ )
  {
    input_vector.insert( input_vector.end(), features[i].begin(), features[i].end() );
  }
  return this->train_model( input_vector, options );
}

// Accessor definition
template < typename WordType, typename MappingType >
unsigned bag_of_words_model< WordType, MappingType >
::features_per_word() const
{
  return ( vocabulary_.empty() ? 0 : vocabulary_[0].size() );
}

// Stream operator
template < typename W, typename M >
std::ostream& operator<<( std::ostream& out, bag_of_words_model<W,M>& model )
{
  LOG_ASSERT( model.is_valid(), "BoW model not valid, cannot output!" );

  out << "ASCII_BOW_FILE" << std::endl;
  out << model.word_count() << " " << model.features_per_word() << std::endl;

  for( unsigned i = 0; i < model.vocabulary().size(); i++ )
  {
    for( unsigned j = 0; j < model.vocabulary()[i].size(); j++ )
    {
      out << model.vocabulary()[i][j] << " ";
    }
    out << std::endl;
  }

  return out;
}

} // end namespace vidtk
