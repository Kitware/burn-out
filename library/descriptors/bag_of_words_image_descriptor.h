/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_descriptor_bag_of_words_image_descriptor
#define vidtk_descriptor_bag_of_words_image_descriptor

#include <vil/vil_image_view.h>

#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>

#include <vector>
#include <string>

#include <descriptors/bag_of_words_model.h>

namespace vidtk
{

/// A helper class for descriptors which map some features to a bag of
/// words model via one of a variety of implemented techniques.
template < typename PixType = float, typename WordType = double, typename MappingType = double >
class bag_of_words_image_descriptor
{

public:

  typedef std::vector< std::vector< WordType > > vocabulary_type;
  typedef std::vector< MappingType > output_type;
  typedef bag_of_words_model< WordType, MappingType > model_type;
  typedef boost::scoped_ptr< model_type > model_sptr_type;

  bag_of_words_image_descriptor() {}
  virtual ~bag_of_words_image_descriptor() {}

  /// Load a bag of words model from some file
  virtual bool load_vocabulary( const std::string& filename );

  /// Set an external vocabulary to use in this model. Useful when we might
  /// want to compute some vocabulary online elsewhere.
  virtual bool set_vocabulary( const vocabulary_type& vocab );

  /// Is the internal bag of words model valid and loaded?
  virtual bool is_vocabulary_valid();

  /// Return a reference to the internal model, throwing if model is invalid.
  virtual model_type const& vocabulary();

  /// Calculate a vector of raw dsift features, image must be single channel.
  virtual void compute_features( const vil_image_view< PixType >& image,
                                 std::vector< WordType >& features ) const = 0;

  /// Map a vector of raw dsift features onto a bow model.
  virtual void compute_bow_descriptor( const std::vector< WordType >& features,
                                       std::vector< MappingType >& output ) const = 0;

  /// Calculate a bow descriptor directory from an input image.
  virtual void compute_bow_descriptor( const vil_image_view< PixType >& image,
                                       std::vector< MappingType >& output ) const = 0;

protected:

  // The internal BoW model storing the bow vocabulary
  model_sptr_type vocabulary_;

};

}

#endif
