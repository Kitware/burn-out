/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_descriptor_bag_of_words_model
#define vidtk_descriptor_bag_of_words_model

#include <vector>
#include <string>
#include <iostream>

namespace vidtk
{

/// Settings describing how features are mapped onto a trained BoW vocabulary.
struct bow_mapping_settings
{
  /// Type of normalization to perform on the output mapping vector.
  enum{ L1, L2, NONE } norm_type_;

  /// Bin voting schema. SINGLE_VOTE will simply cast a vote of weight 1.0
  /// to the nearest BoW word, TOP_3_WEIGHTED will cast weighted votes
  /// into the top 3 matches based on their simularities.
  enum{ SINGLE_VOTE_UNWEIGHTED, TOP_3_WEIGHTED } voting_method_;

  // Default values
  bow_mapping_settings() : norm_type_( L1 ),
                           voting_method_( TOP_3_WEIGHTED ) {}
};


/// Settings describing how BoW vocabularies are learned from features.
struct bow_training_settings
{
  /// Clustering method
  enum{ KMEANS } clustering_method_;

  /// Max clustering iterations
  unsigned max_iter_;

  /// Number of words
  unsigned word_count_;

  /// Number of bins per word
  unsigned bins_per_word_;

  /// Downselect every 1 in x features
  unsigned downsampling_rate_;

  /// After clustering, should we run L2 normalization on each BoW word?
  bool normalize_output_vocab_;

  /// verbosity (passed to vl)
  int verbosity_;

  // Default values
  bow_training_settings() : clustering_method_( KMEANS ),
                            max_iter_( 120 ),
                            word_count_( 300 ),
                            bins_per_word_( 128 ),
                            downsampling_rate_( 2 ),
                            normalize_output_vocab_( false ),
                            verbosity_( 0 ) {}
};


/// A helper class for descriptors which map some features to a bag of
/// words model via one of a variety of implemented techniques.
template < typename WordType = double, typename MappingType = double >
class bag_of_words_model
{

public:

  typedef std::vector< std::vector< WordType > > vocabulary_type;
  typedef std::vector< MappingType > output_type;
  typedef std::vector< WordType > input_type;
  typedef std::vector< input_type > input_vector_type;

  bag_of_words_model() : is_valid_(false) {}
  virtual ~bag_of_words_model() {}

  /// Load a bag of words model from some file
  virtual bool load_model( const std::string& filename );

  /// Set an external bag of words vocabulary to use in this model. Useful when
  /// we might want to compute some vocabulary online elsewhere.
  virtual bool set_model( const vocabulary_type& model );

  /// Map input features to some loaded BoW model
  bool map_to_model( const input_type& features,
                     const bow_mapping_settings& options,
                     output_type& mapping_output ) const;

  /// Map input features to some loaded BoW model
  bool map_to_model( const input_vector_type& features,
                     const bow_mapping_settings& options,
                     output_type& mapping_output ) const;

  /// Train a new BoW model by performing clustering.
  bool train_model( const input_type& features,
                    const bow_training_settings& options );

  /// Train a new BoW model by performing clustering.
  bool train_model( const input_vector_type& features,
                    const bow_training_settings& options );

  /// Is the loaded vocabulary valid?
  bool is_valid() const { return is_valid_; }

  /// Return the number of words in the loaded vocabulary
  unsigned word_count() const { return vocabulary_.size(); }

  /// Returns a reference to the internal BoW model
  vocabulary_type const& vocabulary() const { return vocabulary_; }

  /// Return the number of features per word in the loaded vocabulary
  unsigned features_per_word() const;

  // Stream operator overload
  template< typename W, typename M >
  friend std::ostream& operator<<( std::ostream& out, bag_of_words_model<W,M>& model );

protected:

  // Is the loaded vocabulary model valid?
  bool is_valid_;

  // The loaded vocabulary model
  vocabulary_type vocabulary_;

  // Perform actual mapping between the input features and the BoW model
  virtual bool map_to_model( const WordType** features,
                             const unsigned num_features,
                             const bow_mapping_settings& options,
                             output_type& mapping_output ) const;
};

}

#endif
