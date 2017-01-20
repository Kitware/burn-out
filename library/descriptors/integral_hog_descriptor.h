/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_integral_hog_descriptor_h_
#define vidtk_integral_hog_descriptor_h_

#include <descriptors/online_descriptor_generator.h>

#include <vgl/vgl_box_2d.h>

#include <vector>
#include <string>

#include <boost/function.hpp>

namespace vidtk
{

/// A model describing a custom integral HoG model.
class custom_ii_hog_model
{
public:

  /// Default constructor, creates an empty model.
  custom_ii_hog_model() {}
  virtual ~custom_ii_hog_model() {}

  /// Normalized locations of each HoG cell relative to some input region.
  /// The range of all values should be in the 0 to 1 range.
  std::vector< vgl_box_2d< double > > cells_;

  /// Ids of the cells comprising each block.
  std::vector< std::vector< unsigned > > blocks_;

  /// Clear anything stored in this model.
  void clear() { cells_.clear(), blocks_.clear(); }

  /// Number of output features a HoG of this type will have
  unsigned feature_count() const;

  /// Is the current model valid?
  bool is_valid() const;

  /// Load a model from a file. An example model contains the below info:
  ///
  ///   ASCII_HOG_MODEL
  ///   [# cells] [# blocks]
  ///   CELL 1: [ RECT ]
  ///   CELL 2: [ RECT ]
  ///   CELL 3: [ RECT ]
  ///   BLOCK 1: [ List of CELL IDs comprising block ]
  ///       etc...
  bool load_model( const std::string& filename );

  /// The number of orientation bins per HoG cell, this is currently
  /// a fixed quantity that should not be changed due to optimizations.
  static const unsigned bins_per_cell = 8;

};


/// Generates approximate HoG descriptors quickly using integral images (ii).
/// In order to use the descriptor, you must first set a reference image. From
/// this reference image, multiple integral images will be calculated. You can
/// then generate custom HoG descriptors anywhere in the reference image plane.
template< typename PixType >
class integral_hog_descriptor
{

public:

  typedef std::vector< double > descriptor_t;
  typedef vil_image_view< PixType > input_image_t;
  typedef vil_image_view< double > gradient_image_t;
  typedef vil_image_view< double > integral_image_t;
  typedef vgl_box_2d< unsigned > image_region_t;

  integral_hog_descriptor() : is_valid_(false) {}

  /// Generates ii from grayscale image input, by first computing its gradients.
  integral_hog_descriptor( const input_image_t& input );

  /// Generates ii from pre-computed gradient images grad_i and grad_j.
  integral_hog_descriptor( const gradient_image_t& grad_i,
                           const gradient_image_t& grad_j );

  virtual ~integral_hog_descriptor() {}

  /// Generates ii from grayscale image input, by first computing its gradients.
  void set_grayscale_ref_image( const input_image_t& input );

  /// Generates ii from pre-computed gradient images grad_i and grad_j.
  void set_gradients_ref_image( const gradient_image_t& grad_i,
                                const gradient_image_t& grad_j );

  /// Generate a custom HoG descriptor for a particular region.
  void generate_hog_descriptor( const image_region_t& region,
                                const custom_ii_hog_model& model,
                                descriptor_t& output ) const;

  /// Generate a custom HoG descriptor for a particular region.
  void generate_hog_descriptor( const image_region_t& region,
                                const custom_ii_hog_model& model,
                                double* output ) const;

  /// Compute HoG descriptors for many regions.
  void generate_hog_descriptors( const std::vector< image_region_t >& regions,
                                 const custom_ii_hog_model& model,
                                 std::vector< descriptor_t >& descriptors ) const;

  /// Compute a grid of HoG descriptors, where every cell must have the same size and
  /// normalization is only performed once for all cells, per descriptors.
  void generate_hog_descriptors( const unsigned cell_pixel_size,
                                 const unsigned cells_per_hog_dim,
                                 std::vector< image_region_t >& regions,
                                 std::vector< descriptor_t >& descriptors,
                                 bool normalize_descriptors = true ) const;

  /// Generate an unnormalized 8-bin descriptor for a single HoG cell.
  void cell_descriptor( const image_region_t& region,
                        descriptor_t& output ) const;

  /// Generate an unnormalized 8-bin descriptor for a single HoG cell.
  void cell_descriptor( const image_region_t& region,
                        double* output ) const;

  /// Generate an unnormalized 8-bin descriptor for a single HoG cell.
  void cell_descriptor( const unsigned i,
                        const unsigned j,
                        const unsigned width,
                        const unsigned height,
                        double* output ) const;

  /// Generate an 8-bin descriptor for a single HoG cell, normalized by area.
  void normalized_cell_descriptor( const image_region_t& region,
                                   descriptor_t& output ) const;

  /// Generate an 8-bin descriptor for a single HoG cell, normalized by area.
  void normalized_cell_descriptor( const image_region_t& region,
                                   double* output ) const;

  /// Generate an 8-bin descriptor for a single HoG cell, normalized by area.
  void normalized_cell_descriptor( const unsigned i,
                                   const unsigned j,
                                   const unsigned width,
                                   const unsigned height,
                                   double* output ) const;

private:

  // Was a valid input image set?
  bool is_valid_;

  // Integral image chain used for feature generation
  integral_image_t integral_gradients_;

  // Image chain to store gradient magnitudes voted unto all 8 bins
  gradient_image_t directional_gradients_;

};

} // end namespace vidtk

#endif
