/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_mfeat_bow_descriptor_h_
#define vidtk_mfeat_bow_descriptor_h_

#include <descriptors/bag_of_words_image_descriptor.h>

#include <vector>
#include <string>

#include <vil/vil_image_view.h>

namespace vidtk
{

typedef bag_of_words_model< float, double > mfeat_vocabulary;


/// mfeat descriptor computation settings
struct mfeat_computer_settings
{
  /// Location of the BoW model file
  std::string bow_filename;

  /// Features to BoW mapping settings
  bow_mapping_settings bow_settings;

  /// Compute Hessian features
  bool compute_hessian;

  /// Compute Harris Laplace features
  bool compute_harris_laplace;

  /// Compute DoG features
  bool compute_dog;

  /// Compute Hessian Laplace features
  bool compute_hessian_laplace;

  /// Compute Multiscale Hessian features
  bool compute_multiscale_hessian;

  /// Compute Multiscale Harris features
  bool compute_multiscale_harris;

  /// Compute MSER features
  bool compute_mser;

  /// Drop descriptors this many pixels near the image border
  unsigned boundary_margin;

  /// Should we estimate the affine shape around each keypoint?
  bool estimate_affine_shape;

  /// Should we estimate the affine shape around each keypoint?
  bool estimate_orientation;

  // Default settings
  mfeat_computer_settings()
  : bow_filename(""),
    bow_settings(),
    compute_hessian( false ),
    compute_harris_laplace( true ),
    compute_dog( true ),
    compute_hessian_laplace( false ),
    compute_multiscale_hessian( true ),
    compute_multiscale_harris( false ),
    compute_mser( false ),
    boundary_margin( 0 ),
    estimate_affine_shape( true ),
    estimate_orientation( true )
  {}
};


/// Locked descriptor constants which can be used both internally and externally
///@{
static const int mfeat_orientations_per_cell = 8;
static const int mfeat_cells_per_dim = 4;
static const int mfeat_total_cells = mfeat_cells_per_dim * mfeat_cells_per_dim;
static const int mfeat_descriptor_bins = mfeat_total_cells * mfeat_orientations_per_cell;
///@}


/// A class for computing mfeat and mfeat bow features and descriptors.
class mfeat_bow_descriptor : public bag_of_words_image_descriptor< float, float, double >
{

public:

  mfeat_bow_descriptor() {}
  mfeat_bow_descriptor( const mfeat_computer_settings& options );
  virtual ~mfeat_bow_descriptor() {}

  /// Set internal settings and load vocabulary.
  bool configure( const mfeat_computer_settings& options );

  /// Set internal settings to defaults and load vocabulary.
  bool configure( const std::string& filename );

  /// Calculate a vector of raw mfeat features.
  void compute_features( const vil_image_view<float>& image,
                         std::vector<float>& features ) const;

  /// Map a vector of raw mfeat features onto a bow model.
  void compute_bow_descriptor( const std::vector<float>& features,
                               std::vector<double>& output ) const;

  /// Calculate a bow descriptor directory from an input image.
  void compute_bow_descriptor( const vil_image_view<float>& image,
                               std::vector<double>& output ) const;

private:

  // Internal copy of externally set options
  mfeat_computer_settings settings_;

};

} // end namespace vidtk

#endif
