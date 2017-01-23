/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_obj_specific_salient_region_classifier_h_
#define vidtk_obj_specific_salient_region_classifier_h_

#include "obj_specific_salient_region_detector.h"
#include "moving_training_data_container.h"

#include <utilities/training_thread.h>

#include <classifier/hashed_image_classifier.h>

#include <learning/adaboost.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <vector>
#include <iostream>

namespace vidtk
{

/// \brief Settings for the obj_specific_salient_region_classifier class.
struct ossr_classifier_settings
{
  /// Number of iterations per model update.
  unsigned iterations_per_update;

  /// Averaging factor used to average prior and new models for each update.
  /// If set to 0.0, the new model will always completely override the prior.
  double averaging_factor;

  /// Is threading enabled?
  bool use_external_thread;

  /// Seed classifier filename, if exists.
  std::string seed_model_fn;

  /// Classifier normalization method.
  enum{ NONE, FULL_HISTOGRAM, STUMPS_ONLY } norm_method;

  /// Additional logging option.
  bool verbose_logging;

  /// Debug option to print intermediate classifiers to this file.
  std::string weak_learner_file;

  /// Advanced option to make the classifier more efficient on certain data.
  bool consolidate_weak_clfr;

  // Set defaults
  ossr_classifier_settings()
  : iterations_per_update( 10 ),
    averaging_factor( 0.95 ),
    use_external_thread( false ),
    seed_model_fn(),
    norm_method( NONE ),
    verbose_logging( false ),
    weak_learner_file(),
    consolidate_weak_clfr( false )
  {}
};

/// \brief A pixel-level classifier learned specific to an object.
template< typename FeatureType, typename OutputType >
class obj_specific_salient_region_classifier
  : public hashed_image_classifier<FeatureType,OutputType>
{

public:

  typedef unsigned index_t;
  typedef OutputType weight_t;
  typedef vil_image_view< weight_t > weight_image_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef FeatureType input_t;
  typedef vil_image_view< input_t > input_image_t;
  typedef moving_training_data_container< FeatureType > training_data_t;
  typedef std::vector< input_image_t > feature_vector_t;
  typedef hashed_image_classifier_model< OutputType > model_t;
  typedef boost::shared_ptr< model_t > model_sptr_t;

  obj_specific_salient_region_classifier();
  virtual ~obj_specific_salient_region_classifier();

  /// \brief Set internal settings and load all required models.
  virtual bool configure( const ossr_classifier_settings& settings );

  /// \brief Classify a feature array, in addition to adding offset to
  /// each pixel in the output classification image.
  virtual void classify_images( const feature_vector_t& input_features,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// \brief Classify a feature array, in addition to adding offset
  /// to each pixel in the output classification image.
  virtual void classify_images( const input_image_t* input_features,
                                const unsigned features,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// \brief Only classify certain pixels as given by a mask.
  virtual void classify_images( const feature_vector_t& input_features,
                                const mask_image_t& mask,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// \brief Only classify certain pixels as given by a mask.
  virtual void classify_images( const input_image_t* input_features,
                                const unsigned features,
                                const mask_image_t& mask,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// \brief Update some external model given new training data
  ///
  /// \param positive_samples Positive training samples
  /// \param negative_samples Negative training samples
  /// \param key Optional key, see reset_model function
  ///
  /// This function call will be blocking if threading is not enabled,
  /// and non-blocking if it is.
  virtual void update_model( const training_data_t& positive_samples,
                             const training_data_t& negative_samples,
                             const index_t& key = 0 );

  /// \brief Resets the internal model to the initial seed model.
  ///
  /// \param key Optional key
  ///
  /// This function also takes in an optional key. If this key is specified
  /// then this model will only use the learned target model from the update
  /// model function if that key matches the key specified here. All other
  /// update model calls will be ignored.
  virtual void reset_model( const index_t& key = 0 );

  /// \brief Sets an externally provided seed model.
  virtual void set_model( model_sptr_t external_model );

  /// \brief Get number of features used to generate the internal model.
  virtual unsigned feature_count() const;

  /// \brief Is the current model valid?
  virtual bool is_valid() const;

  /// \brief Does the current class contain a seed model or a learned model?
  virtual bool is_learned_model() const;

  /// \brief Adjust internal model by scaling model weights or adding a bias.
  ///
  /// \param scale All weights are multiplied by this scale factor
  /// \param bias Bias added to internal model after scaling
  virtual void adjust_model( weight_t scale, weight_t bias = 0 );

private:

  typedef obj_specific_salient_region_classifier self_t;
  typedef boost::shared_ptr< self_t > sptr_t;
  typedef hashed_image_classifier< FeatureType, OutputType > base_t;
  typedef boost::shared_ptr< training_thread > thread_sptr_t;
  typedef typename training_data_t::feature_t feature_t;
  typedef boost::shared_mutex mutex_t;
  typedef boost::unique_lock< mutex_t > exclusive_lock_t;
  typedef boost::shared_lock< mutex_t > shared_lock_t;

  // Recorded settings
  ossr_classifier_settings settings_;

  // Thread for training
  thread_sptr_t training_thread_;

  // Original model
  model_sptr_t seed_model_;

  // Mutex used for when updating the internal model
  mutable mutex_t update_mutex_;

  // Last key received
  index_t last_key_;

  // Is our model still the seed model?
  bool is_seed_model_;

  // Update the internal model with an updated one
  void update_model( model_sptr_t new_model, index_t key );

  // A class for threaded tasks utilized when update_model is called
  class threaded_task;
  friend class threaded_task;

};

} // end namespace vidtk

#endif // vidtk_obj_specific_salient_region_classifier_h_
