/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "obj_specific_salient_region_classifier.h"

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <fstream>

#include <vnl/vnl_vector.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER( "obj_specific_salient_region_classifier_txx" );

template< typename FeatureType, typename OutputType >
obj_specific_salient_region_classifier< FeatureType, OutputType >
::obj_specific_salient_region_classifier()
{
}

template< typename FeatureType, typename OutputType >
obj_specific_salient_region_classifier< FeatureType, OutputType >
::~obj_specific_salient_region_classifier()
{
  training_thread_.reset();
}

template< typename FeatureType, typename OutputType >
bool
obj_specific_salient_region_classifier< FeatureType, OutputType >
::configure( const ossr_classifier_settings& settings )
{
  exclusive_lock_t lock( update_mutex_ );

  settings_ = settings;

  last_key_ = 0;

  if( settings.use_external_thread )
  {
    training_thread_.reset( new training_thread() );
  }

  if( !settings.seed_model_fn.empty() )
  {
    if( !base_t::load_from_file( settings.seed_model_fn ) )
    {
      return false;
    }

    this->seed_model_ = this->model_;
    this->is_seed_model_ = true;
  }
  else
  {
    this->model_ = model_sptr_t();
    this->seed_model_ = model_sptr_t();
    this->is_seed_model_ = false;
  }

  return true;
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::classify_images( const feature_vector_t& input_features,
                   weight_image_t& output_image,
                   const weight_t offset ) const
{
  base_t::classify_images( input_features, output_image, offset );
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::classify_images( const feature_vector_t& input_features,
                   const mask_image_t& mask,
                   weight_image_t& output_image,
                   const weight_t offset ) const
{
  base_t::classify_images( input_features, mask, output_image, offset );
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::classify_images( const input_image_t* input_features,
                   const unsigned features,
                   weight_image_t& output_image,
                   const weight_t offset ) const
{
  shared_lock_t lock( update_mutex_ );
  base_t::classify_images( input_features, features, output_image, offset );
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::classify_images( const input_image_t* input_features,
                   const unsigned features,
                   const mask_image_t& mask,
                   weight_image_t& output_image,
                   const weight_t offset ) const
{
  shared_lock_t lock( update_mutex_ );
  base_t::classify_images( input_features, features, mask, output_image, offset );
}

template< typename FloatType >
class ossrc_stump
{
public:

  FloatType weight;
  unsigned feature_id;
  unsigned threshold;
  bool greater_than_crit;

  ossrc_stump()
  : weight( 0.0 ),
    feature_id( 0 ),
    threshold( 0 ),
    greater_than_crit( false )
  {}

  void invert()
  {
    greater_than_crit = !greater_than_crit;
  }
};

template< typename FeatureType, typename FloatType >
FloatType
train_stump( const moving_training_data_container< FeatureType >& positive_samples,
             const moving_training_data_container< FeatureType >& negative_samples,
             const vnl_vector< FloatType >& weights,
             const unsigned feature_index,
             ossrc_stump< FloatType >& output )
{
  const unsigned max_feature_value = std::numeric_limits< FeatureType >::max();
  const unsigned max_feature_value_p1 = std::numeric_limits< FeatureType >::max() + 1;

  const unsigned pos_sample_count = positive_samples.size();
  const unsigned neg_sample_count = negative_samples.size();
  const unsigned tot_sample_count = pos_sample_count + neg_sample_count;

  // Weight arrays for all accumulated weights for positive and negative samples
  std::vector< FloatType > pos_w_array( max_feature_value_p1, 0.0 );
  std::vector< FloatType > neg_w_array( max_feature_value_p1, 0.0 );

  // Accumulate pos array points
  const FeatureType *pos_pos = positive_samples.entry( 0 ) + feature_index;
  const FeatureType *neg_pos = negative_samples.entry( 0 ) + feature_index;
  const std::ptrdiff_t step = positive_samples.feature_count();

  for( unsigned i = 0; i < positive_samples.size(); i++, pos_pos+=step )
  {
    pos_w_array[ *pos_pos ] += weights[i];
  }

  for( unsigned i = positive_samples.size(); i < tot_sample_count; i++, neg_pos+=step )
  {
    neg_w_array[ *neg_pos ] += weights[i];
  }

  // Integrate weight array values
  for( unsigned i = 1; i < max_feature_value_p1; i++ )
  {
    pos_w_array[ i ] = pos_w_array[ i ] + pos_w_array[ i-1 ];
    neg_w_array[ i ] = neg_w_array[ i ] + neg_w_array[ i-1 ];
  }

  // Select classifier with least error
  FloatType total_pos = pos_w_array[ max_feature_value ];
  FloatType total_neg = neg_w_array[ max_feature_value ];

  output.feature_id = feature_index;
  output.weight = 0.0;

  FloatType best_error = std::numeric_limits< FloatType >::max();

  for( unsigned i = 0; i < max_feature_value; i++ )
  {
    FloatType reg_error = neg_w_array[i] + total_pos - pos_w_array[i];
    FloatType inv_error = pos_w_array[i] + total_neg - neg_w_array[i];

    if( reg_error < best_error )
    {
      output.threshold = i;
      output.greater_than_crit = false;
      best_error = reg_error;
    }

    if( inv_error < best_error )
    {
      output.threshold = i;
      output.greater_than_crit = true;
      best_error = inv_error;
    }
  }

  return best_error;
}

template< typename FloatType >
void normalize_stumps( std::vector< ossrc_stump< FloatType > >& stumps )
{
  FloatType norm_factor = 0;

  for( unsigned i = 0; i < stumps.size(); ++i )
  {
    norm_factor += std::fabs( stumps[i].weight );
  }

  if( norm_factor != 0 )
  {
    norm_factor = 1.0 / norm_factor;

    for( unsigned i = 0; i < stumps.size(); ++i )
    {
      stumps[i].weight *= norm_factor;
    }
  }
}

template< typename FloatType >
void update_model_weights( const std::vector< ossrc_stump< FloatType > >& new_stumps,
                           const FloatType seed_weight,
                           const FloatType averaging_factor,
                           hashed_image_classifier_model< FloatType >& model )
{
  std::vector< FloatType >& weights = model.weights;
  std::vector< FloatType* >& feature_weights = model.feature_weights;
  std::vector< unsigned >& max_feature_values = model.max_feature_value;

  // First, adjust existing model weights.
  FloatType inv_averaging_factor = 1.0 - averaging_factor;
  FloatType adj_factor = averaging_factor + seed_weight * inv_averaging_factor;

  for( unsigned i = 0; i < weights.size(); i++ )
  {
    weights[i] = weights[i] * adj_factor;
  }

  // Next, adjust values based on new classifiers
  for( unsigned i = 0; i < new_stumps.size(); i++ )
  {
    unsigned feature_index = new_stumps[i].feature_id;
    unsigned pivot = new_stumps[i].threshold + 1;

    adj_factor = new_stumps[i].weight * inv_averaging_factor;
    FloatType* feature_start = feature_weights[ feature_index ];

    unsigned start_ind = ( new_stumps[i].greater_than_crit ? pivot : 0 );
    unsigned end_ind = ( new_stumps[i].greater_than_crit ? max_feature_values[feature_index] : pivot );

    for( unsigned j = start_ind; j < end_ind; j++ )
    {
      feature_start[j] = feature_start[j] + adj_factor;
    }
  }
}

template< typename FeatureType, typename FloatType, typename OutputType >
void classify( const hashed_image_classifier_model< FloatType >& model,
               const moving_training_data_container< FeatureType >& samples,
               vnl_vector< OutputType >& outputs )
{
  const unsigned feature_count = samples.feature_count();
  const std::vector< FloatType* >& feature_weights = model.feature_weights;

  outputs.set_size( samples.size() );
  std::fill( outputs.begin(), outputs.end(), 0.0 );

  for( unsigned f = 0; f < feature_count; f++ )
  {
    const FeatureType* pos = samples.entry( 0 ) + f;
    const std::ptrdiff_t step = feature_count;
    const FloatType* feature_weight_ptr = feature_weights[f];

    for( unsigned i = 0; i < samples.size(); i++, pos += step )
    {
      outputs[i] += feature_weight_ptr[ *pos ];
    }
  }
}

template< typename FeatureType, typename OutputType >
void classify( const ossrc_stump< OutputType >& model,
               const moving_training_data_container< FeatureType >& samples,
               vnl_vector< OutputType >& outputs )
{
  outputs.set_size( samples.size() );
  std::fill( outputs.begin(), outputs.end(), 0.0 );

  const FeatureType* pos = samples.entry( 0 ) + model.feature_id;
  const std::ptrdiff_t step = samples.feature_count();

  if( model.greater_than_crit )
  {
    for( unsigned i = 0; i < samples.size(); i++, pos += step )
    {
      if( *pos > model.threshold )
      {
        outputs[i] = 1.0;
      }
    }
  }
  else
  {
    for( unsigned i = 0; i < samples.size(); i++, pos += step )
    {
      if( *pos <= model.threshold )
      {
        outputs[i] = 1.0;
      }
    }
  }
}

template< typename FloatType, typename OutputType >
void
compute_estimation_sums( const vnl_vector< FloatType >& weights,
                         const vnl_vector< FloatType >& pos_est,
                         const vnl_vector< FloatType >& neg_est,
                         OutputType& sum1, OutputType& sum2 )
{
  LOG_ASSERT( weights.size() == pos_est.size() + neg_est.size(), "Invalid Input" );

  const unsigned pos_size = pos_est.size();

  sum1 = 0;
  sum2 = 0;

  for( unsigned i = 0; i < pos_size; i++ )
  {
    sum1 += ( weights[i] * pos_est[i] );
  }
  for( unsigned i = 0; i < neg_est.size(); i++ )
  {
    sum2 += ( weights[i + pos_size] * neg_est[i] );
  }
}

template< typename FloatType >
void
compute_alpha( const FloatType sum1,
               const FloatType sum2,
               FloatType& alpha )
{
  const FloatType epsilon = std::numeric_limits<FloatType>::epsilon();

  if( sum1 == 0 && sum2 == 0 )
  {
    alpha = 0.0;
  }
  else
  {
    alpha = 0.5 * std::log( (sum1 + epsilon) / (sum2 + epsilon) );
  }
}

template< typename FloatType >
void
update_net_scores( const vnl_vector< FloatType >& pos_est,
                   const vnl_vector< FloatType >& neg_est,
                   const FloatType scale_factor,
                   vnl_vector< FloatType >& net_scores )
{
  const unsigned pos_size = pos_est.size();

  for( unsigned i = 0; i < pos_est.size(); i++ )
  {
    net_scores[i] += ( pos_est[i] * scale_factor );
  }
  for( unsigned i = 0; i < neg_est.size(); i++ )
  {
    net_scores[i + pos_size] += ( neg_est[i] * scale_factor );
  }
}

template< typename FloatType >
void
update_weights( const unsigned pos_samples,
                const vnl_vector< FloatType >& net_scores,
                vnl_vector< FloatType >& weights )
{
  FloatType sum = 0;

  for( unsigned i = 0; i < net_scores.size(); i++ )
  {
    double sign_factor = ( i < pos_samples ? -1.0 : 1.0 );
    weights[i] = std::exp( sign_factor * net_scores[i] );
    sum += weights[i];
  }

  if( sum != 0 )
  {
    for( unsigned i = 0; i < weights.size(); i++ )
    {
      weights[i] = weights[i] / sum;
    }
  }
}

template< typename FeatureType, typename OutputType >
class
obj_specific_salient_region_classifier< FeatureType, OutputType >
::threaded_task : public training_thread_task
{
public:

  explicit threaded_task( self_t* source,
                          const training_data_t& positive_samples,
                          const training_data_t& negative_samples,
                          const index_t& key );

  ~threaded_task() {}

  bool execute();

private:

  self_t* source_;
  training_data_t pos_;
  training_data_t neg_;
  index_t key_;

};

template< typename FeatureType, typename OutputType >
obj_specific_salient_region_classifier< FeatureType, OutputType >::threaded_task
::threaded_task( self_t* source,
                 const training_data_t& positive_samples,
                 const training_data_t& negative_samples,
                 const index_t& key )
{
  source_ = source;
  pos_ = positive_samples;
  neg_ = negative_samples;
  key_ = key;
}

template< typename FeatureType, typename OutputType >
bool
obj_specific_salient_region_classifier< FeatureType, OutputType >::threaded_task
::execute()
{
  if( sizeof( FeatureType ) >= sizeof( unsigned ) )
  {
    LOG_ERROR( "Unable to train a model over the given feature type, not implemented." );
    return false;
  }

  if( pos_.size() == 0 || neg_.size() == 0 )
  {
    LOG_INFO( "Not enough training samples to train ossrc model" );
    return true;
  }

  const weight_t averaging_factor = source_->settings_.averaging_factor;
  const unsigned max_iterations = source_->settings_.iterations_per_update;
  const unsigned verbose = source_->settings_.verbose_logging;

  const unsigned feature_count = pos_.feature_count();
  const unsigned total_entries = pos_.size() + neg_.size();

  model_sptr_t new_model( new model_t() );

  // If a model already exists, it is our seed model to be used for the first iteration
  // of the algorithm. If a seed model doesn't exist, create a new empty model of the
  // required size.
  const bool has_seed_model = ( source_->model_ && source_->model_->is_valid() );

  if( has_seed_model )
  {
    LOG_DEBUG( "Training model using seed classifier" );
    *new_model = *( source_->model_ );
  }
  else
  {
    LOG_DEBUG( "Training model without using a seed classifier" );
    new_model->reset( feature_count, std::numeric_limits< FeatureType >::max() );
  }

  // Initialize training variables to default values for real adaboosting.
  vnl_vector< weight_t > training_weights( total_entries, 1.0 / total_entries );
  vnl_vector< weight_t > net_scores_to_itr( total_entries, 0.0 );
  vnl_vector< weight_t > positive_est( pos_.size() );
  vnl_vector< weight_t > negative_est( neg_.size() );

  weight_t seed_weight = 1.0;
  weight_t sum1, sum2;

  // Update weights using existing classifier - operate as if we have already selected
  // a single weak learner, which is our prior classifier. This will get re-weighted
  // accordingly based on its number of misclassifications, in addition to the merging
  // stage.
  if( has_seed_model )
  {
    classify( *new_model, pos_, positive_est );
    classify( *new_model, neg_, negative_est );

    update_net_scores( positive_est, negative_est, seed_weight, net_scores_to_itr );
    update_weights( pos_.size(), net_scores_to_itr, training_weights );
  }

  std::vector< ossrc_stump< weight_t > > new_stumps;

  // Iterate until completion
  for( unsigned t = 0; t < max_iterations; ++t )
  {
    // Output log statment
    if( verbose )
    {
      LOG_INFO( "Training iteration " << t + 1 );
    }

    // Select best classifier
    ossrc_stump< weight_t > best_stump, current_stump;
    weight_t best_error = std::numeric_limits< weight_t >::max();

    for( unsigned f = 0; f < feature_count; ++f )
    {
      weight_t error = train_stump( pos_, neg_, training_weights, f, current_stump );

      if( error < best_error )
      {
        best_stump = current_stump;
        best_stump.invert();

        best_error = error;
      }
    }

    // Assign weight to latest classifier, both forward and in reverse
    for( unsigned i = 0; i < 2; i++, best_stump.invert() )
    {
      classify( best_stump, pos_, positive_est );
      classify( best_stump, neg_, negative_est );

      compute_estimation_sums( training_weights, positive_est, negative_est, sum1, sum2 );
      compute_alpha( sum1, sum2, best_stump.weight );

      if( best_stump.weight != 0 )
      {
        update_net_scores( positive_est, negative_est, best_stump.weight, net_scores_to_itr );
        update_weights( pos_.size(), net_scores_to_itr, training_weights );
        new_stumps.push_back( best_stump );
      }
    }
  }

  // Handle consolidate option
  if( source_->settings_.consolidate_weak_clfr )
  {
    std::vector< ossrc_stump< weight_t > > filtered_stumps;

    for( unsigned i = 0; i < new_stumps.size(); ++i )
    {
      bool found = false;

      for( unsigned j = 0; j < filtered_stumps.size(); ++j )
      {
        if( new_stumps[i].feature_id == filtered_stumps[j].feature_id &&
            new_stumps[i].threshold == filtered_stumps[j].threshold &&
            new_stumps[i].greater_than_crit == filtered_stumps[j].greater_than_crit )
        {
          found = true;
          filtered_stumps[j].weight = filtered_stumps[j].weight + new_stumps[i].weight;
          break;
        }
      }

      if( !found )
      {
        filtered_stumps.push_back( new_stumps[i] );
      }
    }

    new_stumps = filtered_stumps;
  }

  // Normalize stumps if necessary
  if( source_->settings_.norm_method == ossr_classifier_settings::STUMPS_ONLY )
  {
    normalize_stumps( new_stumps );
  }

  // Output weak learners if enabled
  if( !source_->settings_.weak_learner_file.empty() && !new_stumps.empty() )
  {
    std::ofstream ofs( source_->settings_.weak_learner_file.c_str(), std::ofstream::app );

    if( !ofs )
    {
      LOG_ERROR( "Unable to open output file " << source_->settings_.weak_learner_file );
      return false;
    }

    ofs << new_stumps.size() << std::endl << new_stumps[0].weight;

    for( unsigned i = 1; i < new_stumps.size(); ++i )
    {
      ofs << " " << new_stumps[i].weight;
    }

    ofs << std::endl << new_stumps.size() << std::endl;

    for( unsigned i = 0; i < new_stumps.size(); ++i )
    {
      ofs << "6 1 " << new_stumps[i].feature_id;
      ofs << " " << new_stumps[i].threshold + 0.5;
      ofs << " " << ( new_stumps[i].greater_than_crit ? "1" : "0" );
      ofs << std::endl;
    }

    ofs.close();
  }

  // Morph in new weak learners, average with our prior model
  update_model_weights( new_stumps, seed_weight, averaging_factor, *new_model );

  // Normalize resultant model if necessary
  if( source_->settings_.norm_method == ossr_classifier_settings::FULL_HISTOGRAM )
  {
    new_model->normalize();
  }

  source_->update_model( new_model, key_ );
  return true;
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::update_model( const training_data_t& positive_samples,
                const training_data_t& negative_samples,
                const index_t& key )
{
  if( key != last_key_ )
  {
    return;
  }

  training_thread_task_sptr to_do(
    new threaded_task( this, positive_samples, negative_samples, key )
  );

  if( settings_.use_external_thread )
  {
    training_thread_->execute_if_able( to_do );
  }
  else
  {
    to_do->execute();
  }
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::reset_model( const index_t& key )
{
  last_key_ = key;

  exclusive_lock_t lock( update_mutex_ );

  if( settings_.use_external_thread )
  {
    training_thread_->forced_reset();
  }

  this->model_ = seed_model_;
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::update_model( model_sptr_t new_model, index_t key )
{
  if( key != last_key_ )
  {
    return;
  }

  exclusive_lock_t lock( update_mutex_ );

  if( key != last_key_ )
  {
    return;
  }

  LOG_INFO( "Pixel level model updated!" );

  this->model_ = new_model;
  is_seed_model_ = false;
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::set_model( model_sptr_t external_model )
{
  exclusive_lock_t lock( update_mutex_ );
  base_t::set_model( external_model );
}

template< typename FeatureType, typename OutputType >
unsigned
obj_specific_salient_region_classifier< FeatureType, OutputType >
::feature_count() const
{
  shared_lock_t lock( update_mutex_ );
  return base_t::feature_count();
}

template< typename FeatureType, typename OutputType >
bool
obj_specific_salient_region_classifier< FeatureType, OutputType >
::is_valid() const
{
  shared_lock_t lock( update_mutex_ );
  return base_t::is_valid();
}

template< typename FeatureType, typename OutputType >
bool
obj_specific_salient_region_classifier< FeatureType, OutputType >
::is_learned_model() const
{
  return !is_seed_model_;
}

template< typename FeatureType, typename OutputType >
void
obj_specific_salient_region_classifier< FeatureType, OutputType >
::adjust_model( weight_t scale, weight_t bias )
{
  if( this->is_valid() )
  {
    std::vector< weight_t >& weights = this->model_->weights;

    if( scale != 1 )
    {
      for( unsigned i = 0; i < weights.size(); ++i )
      {
        weights[i] *= scale;
      }
    }

    if( bias != 0 )
    {
      for( unsigned i = 0; i < this->model_->max_feature_value[0]; ++i )
      {
        weights[i] += bias;
      }
    }
  }
  else
  {
    LOG_ERROR( "Model is invalid, cannot scale values" );
  }
}

} // end namespace vidtk
