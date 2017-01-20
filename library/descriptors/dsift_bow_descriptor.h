/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_dsift_bow_descriptor_h_
#define vidtk_dsift_bow_descriptor_h_

#include <descriptors/bag_of_words_image_descriptor.h>

#include <vector>
#include <string>

#include <vil/vil_image_view.h>

namespace vidtk
{

typedef bag_of_words_model< float, double > dsift_vocabulary;

/// DSIFT descriptor computation settings
struct dsift_settings
{
  /// Location of the BoW model file
  std::string bow_filename;

  /// Number of orientation bins to use
  int orientation_bins;

  /// Number of spatial bins in x and y directions
  int spatial_bins;

  /// Patch size between descriptors
  int patch_size;

  /// Step size between descriptors (pixels)
  int step_size;

  /// Features to BoW mapping settings
  bow_mapping_settings bow_settings;

  /// Set default settings
  dsift_settings()
   : bow_filename(""),
     orientation_bins(8),
     spatial_bins(4),
     patch_size(8),
     step_size(4),
     bow_settings()
  {}

  /// Based on the other settings, return the descriptor length.
  int feature_descriptor_size() const;
};


/// A class for computing dsift and dsift bow features and descriptors.
class dsift_bow_descriptor
  : public bag_of_words_image_descriptor< float, float, double >
{

public:

  dsift_bow_descriptor() {}
  dsift_bow_descriptor( const dsift_settings& options );
  virtual ~dsift_bow_descriptor() {}

  /// Set internal settings and load vocabulary.
  bool configure( const dsift_settings& options );

  /// Set internal settings to defaults and load vocabulary.
  bool configure( const std::string& filename );

  /// Calculate a vector of raw dsift features, image must be single channel.
  void compute_features( const vil_image_view<float>& image,
                         std::vector<float>& features ) const;

  /// Map a vector of raw dsift features onto a bow model.
  void compute_bow_descriptor( const std::vector<float>& features,
                               std::vector<double>& output ) const;

  /// Calculate a bow descriptor directory from an input image.
  void compute_bow_descriptor( const vil_image_view<float>& image,
                               std::vector<double>& output ) const;

private:

  // Internal copy of externally set options
  dsift_settings settings_;

};

} // end namespace vidtk

#endif
