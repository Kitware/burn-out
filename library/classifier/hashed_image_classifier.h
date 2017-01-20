/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_hashed_image_classifier_h_
#define vidtk_hashed_image_classifier_h_

#include <vil/vil_image_view.h>
#include <vil/vil_rgb.h>
#include <vgl/vgl_box_2d.h>

#include <iostream>
#include <vector>
#include <limits>
#include <fstream>

#include <boost/shared_ptr.hpp>

namespace vidtk
{


template <typename FeatureType, typename OutputType >
class hashed_image_classifier;


/// \brief Internal data required for the hashed_image_classifier class.
/**
 * The data members of this class are exposed just in case any external functions
 * or processes want to train their own models, usually in an online fashion.
 */
template <typename FloatType = double>
class hashed_image_classifier_model
{
public:

  typedef hashed_image_classifier_model< FloatType > self_t;
  typedef FloatType weight_t;

  hashed_image_classifier_model() : num_features( 0 ) {}
  hashed_image_classifier_model( const self_t& other );
  virtual ~hashed_image_classifier_model() {}

  // Number of features
  unsigned int num_features;

  // Maximum value for each feature
  std::vector< unsigned > max_feature_value;

  // Root pointer to start of weight array for all features
  std::vector< weight_t > weights;

  // Pointers to the start of each individual feature weight vector
  std::vector< weight_t* > feature_weights;

  // Is the current classifier configured correctly?
  bool is_valid() const;

  // Copy operator
  self_t& operator=( const self_t& other );

  // Seed the model with empty values of the given size
  void reset( unsigned feature_count, unsigned entries_per_feature );

  // Normalize internal histogram weights to a given absolute sum
  void normalize( weight_t total_weight = 1.0 );
};


/// \brief Stream operator declaration for the hashed_image_classifier model class.
template <typename FloatType >
std::ostream& operator<<( std::ostream& os,
  const hashed_image_classifier_model< FloatType >& obj );


/// \brief Stream operator declaration for the hashed_image_classifier class.
template <typename FeatureType, typename OutputType >
std::ostream& operator<<( std::ostream& os,
  const hashed_image_classifier< FeatureType, OutputType >& obj );


/// \brief A classifier designed to efficiently classifying every pixel in an image.
/**
 * Every feature (for every pixel) is required to be mapped onto unsigned integral
 * type HashType before input. For every possible value of this hashed mapping,
 * and for every feature, a single weight must be given (in a model file) which
 * contributes to some binary decision about said pixel.
 *
 * More formally, for every feature at location i,j, ie, f(i,j,0), f(i,j,1) f(i,j,...)
 * the output classification is given as:
 *
 *                      c(i,j) = sum( w_{k}[ f(i,j,k) ] )
 *
 *        for every feature k, and some corresponding weight vector w_{k}
 *
 * Typically these models may be specified manually, created with the help of constructs
 * such as parzen window estimates, or learned via supervised learning techniques
 * such as using boosted linear discriminants. Compared against other modern
 * classifiers, it places significant constraints on how features can be used
 * together in favor of efficiency.
 */
template <typename FeatureType, typename OutputType = double>
class hashed_image_classifier
{
public:

  typedef OutputType weight_t;
  typedef vil_image_view< weight_t > weight_image_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef FeatureType input_t;
  typedef vil_image_view< input_t > input_image_t;
  typedef std::vector< input_image_t > feature_vector_t;
  typedef hashed_image_classifier_model< OutputType > model_t;
  typedef boost::shared_ptr< model_t > model_sptr_t;
  typedef hashed_image_classifier< FeatureType, OutputType > self_t;

  /// Default constructor, a model must be loaded via load_from_file before use
  hashed_image_classifier() : model_( new model_t() ) {}

  /// Descructor
  virtual ~hashed_image_classifier() {}

  /// \brief Load a model from a file.
  /**
   * The model file should be specified in the following format, where bracketed
   * items should be replaced by values:
   *
   * [Number of features]
   * [Number of values feature 0 can take on] [weights for value 0, 1, etc. ]
   * [Number of values feature 1 can take on] [weights for value 0, 1, etc. ]
   * etc...
   */
  virtual bool load_from_file( const std::string& file );

  /// Classify a feature array, in addition to adding offset to each pixel.
  virtual void classify_images( const feature_vector_t& input_features,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// Classify a feature array, in addition to adding offset to each pixel.
  virtual void classify_images( const input_image_t* input_features,
                                const unsigned features,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// Only classify certain pixels as given by a mask.
  virtual void classify_images( const feature_vector_t& input_features,
                                const mask_image_t& mask,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// Only classify certain pixels as given by a mask.
  virtual void classify_images( const input_image_t* input_features,
                                const unsigned features,
                                const mask_image_t& mask,
                                weight_image_t& output_image,
                                const weight_t offset = 0.0 ) const;

  /// Generate a weight image for some feature.
  virtual void generate_weight_image( const input_image_t& src,
                                      weight_image_t& dst,
                                      const unsigned& feature_id ) const;

  /// Returns the number of features the loaded model contains info for.
  virtual unsigned feature_count() const { return model_->num_features; }

  /// Was a valid model loaded by this classifier?
  virtual bool is_valid() const { return model_ && model_->is_valid(); }

  /// Set the internal model from some external source.
  virtual void set_model( model_sptr_t external_model );

  /// The stream operator function for writing out models.
  friend std::ostream& operator<< <>( std::ostream& os, const self_t& obj );

protected:

  // A pointer to our internal data
  model_sptr_t model_;

};

}

#endif // vidtk_hashed_image_classifier_h_
