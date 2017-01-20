/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_cnn_descriptor_h_
#define vidtk_cnn_descriptor_h_

#include <vil/vil_image_view.h>

#include <utilities/external_settings.h>

#include <vector>
#include <string>

#include <caffe/net.hpp>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

#define settings_macro( add_param, add_array ) \
  add_param( \
    model_definition, \
    std::string, \
    "", \
    "Filename of the CNN model definition." ); \
  add_param( \
    model_weights, \
    std::string, \
    "", \
    "Filename of the CNN model weights." ); \
  add_param( \
    use_gpu, \
    bool, \
    false, \
    "Whether or not to use a GPU for processing." ); \
  add_param( \
    device_id, \
    int, \
    0, \
    "GPU device ID to use if enabled." ); \
  add_array( \
    layers_to_use, \
    std::string, \
    0, \
    "", \
    "Layers of the CNN to formulate descriptors from." ); \
  add_param( \
    final_layer_string, \
    std::string, \
    "final", \
    "String that, if it appears in the layer list, corresponds " \
    "to the final output from the loaded CNN." ); \
  add_array( \
    final_layer_indices, \
    unsigned, 0, \
    "", \
    "Can optionally be set to an arbitrary number of indices " \
    "corresponding to which CNN outputs we want to include in " \
    "the final descriptor." ); \

init_external_settings2( cnn_descriptor_settings, settings_macro )

#undef settings_macro

/// \brief A class for computing CNN-based descriptors from a single frame.
class cnn_descriptor
{

public:

  typedef float feature_t;
  typedef std::vector< feature_t > feature_vec_t;
  typedef vil_image_view< vxl_byte > input_image_t;
  typedef std::vector< feature_vec_t > output_t;

  cnn_descriptor() {}
  virtual ~cnn_descriptor() {}

  /// Load models.
  bool configure( const cnn_descriptor_settings& settings );

  /// Compute descriptor on a single image.
  void compute( const input_image_t image, output_t& features );

  /// Compute descriptors for multiple images.
  void batch_compute( const std::vector< input_image_t > images,
                      std::vector< output_t >& output );

private:

  // The internally loaded CNN model and weights
  boost::shared_ptr< caffe::Net< feature_t > > cnn_;

  // Internal copy of externally set options
  std::vector< std::string > layers_to_use_;

  // Final layer string
  std::string final_layer_str_;

  // Indices to include in final layer
  std::vector< unsigned > final_layer_inds_;
};

} // end namespace vidtk

#endif
