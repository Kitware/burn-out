/*ckwg +5
 * Copyright 2015-2017 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_cnn_detector_h_
#define vidtk_cnn_detector_h_

#include <vil/vil_image_view.h>

#include <utilities/external_settings.h>
#include <utilities/ring_buffer.h>

#include <tracking_data/image_object.h>

#include <vector>
#include <string>

#include <video_transforms/frame_averaging.h>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

#define settings_macro( add_param, add_array, add_enumr ) \
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
  add_enumr( \
    use_gpu, \
    (YES) (NO) (AUTO), \
    AUTO, \
    "Whether or not to use a GPU for processing. Auto will use it only if " \
    "available." ); \
  add_param( \
    device_id, \
    int, \
    -1, \
    "GPU device ID to use if enabled. Set as -1 for default." ); \
  add_param( \
    threshold, \
    float, \
    0.0, \
    "Detector threshold to use." ); \
  add_enumr( \
    detection_mode, \
    (DISABLED) (FIXED_SIZE) (USE_BLOB), \
    DISABLED, \
    "Algorithm to use to formulate detection bounding boxes. Either a fixed " \
    "size bounding box will be generated, or the connected component blobs " \
    "off the detection heatmap will be used to generate boxes." ); \
  add_enumr( \
    nms_option, \
    (NONE) (PIXEL_LEVEL) (BOX_LEVEL) (ALL), \
    NONE, \
    "Non-maximum suppression option. Blob level will suppress non-maximum " \
    "pixel detections, box level with suppress overlapping boxes, all will " \
    "perform both methods." ); \
  add_param( \
    bbox_nms_crit, \
    double, \
    0.50, \
    "Percentage overlap [0.0,1.0] required to suppress boxes when NMS is " \
    "enabled at the box level." ); \
  add_param( \
    memory_usage, \
    double, \
    -1.0, \
    "If known, the average GPU memory used per image pixel for the given model. " \
    "This can be used to subdivide up input images so that we don't exceed GPU " \
    "memory. If specified the chip_ni and chip_nj will be automatically computed " \
    "and their values instead be maximum chip sizes if they are specified." ); \
  add_param( \
    chip_ni, \
    unsigned, \
    0, \
    "Set a manual chip width, if the image width is a greater size than this " \
    "then the image will be processes in multiple subregions seperately." ); \
  add_param( \
    chip_nj, \
    unsigned, \
    0, \
    "Set a manual chip height, if the image width is a greater size than this " \
    "then the image will be processes in multiple subregions seperately." ); \
  add_array( \
    bbox_side_dims, \
    unsigned, 2, \
    "32 32", \
    "Bounding box side length, used if detection mode is set to fixed." ); \
  add_param( \
    swap_rb, \
    bool, \
    false, \
    "Whether or not the red and blue planes of input images should be " \
    "swapped before input to the CNN (applies to 3 channel imagery only)." ); \
  add_param( \
    squash_channels, \
    bool, \
    false, \
    "If the input image contains more than one channels, all channels be " \
    "merged into a single channel (via averaging) before input." ); \
  add_param( \
    transpose_input, \
    bool, \
    false, \
    "Whether or not to transpose the input before running the CNN on it. " \
    "The output of the CNN will then also be automatically transposed." ); \
  add_param( \
    buffer_length, \
    unsigned, \
    1, \
    "Number of frames to internally buffer for joint input into the cnn. " \
    "If greater than 1, an empty output will be generated until we reach " \
    "this many frames." ); \
  add_param( \
    recent_frame_first, \
    bool, \
    false, \
    "If buffer length is greater than 1, frames will be input to the CNN " \
    "with the most recent frame first (earliest to latest) instead of " \
    "latest to earliest." ); \
  add_enumr( \
    norm_option, \
    (NO_NORM) (HIST_EQ) (PER_FRAME_MEAN_STD), \
    NO_NORM, \
    "Image normalization option. Can either be hist_eq for histogram " \
    "equalization, per_frame_mean_std for mean subtraction and divided by " \
    "standard deviation, or no_norm for no normalization." ); \
  add_array( \
    input_scaling_dims, \
    double, 2, \
    "1.0 0.0", \
    "Before input to the CNN, image values will be scaled and shifted by " \
    "these two values. This occurs after the normalization option." ); \
  add_array( \
    output_scaling_dims, \
    unsigned, 3, \
    "1 0 0", \
    "Amount of upsampling required to bring the cnn output resolution to " \
    "the original input size, followed by any padding required on the sides " \
    "of the output (left, top)." ); \
  add_array( \
    target_channels, \
    unsigned, 0, \
    "", \
    "If the output image contains multiple channels, the maximum value from " \
    "each of these are used to generate detections." ); \
  add_enumr( \
    output_option, \
    (RAW_CHANNEL) (PROB_DIFF), \
    RAW_CHANNEL, \
    "Whether or not the output image should contain the raw probability " \
    "channel or simply the highest probability for the OOI minus all " \
    "other probabilities." ); \
  add_param( \
    average_count, \
    int, \
    0, \
    "Number of frames to average detection outputs over, this will result " \
    "in a 2 channel output for the probability map with the first channel " \
    "as the averaged version, and the second the map for the current frame." ); \
  add_enumr( \
    mask_option, \
    (UNAVERAGED) (AVERAGED), \
    UNAVERAGED, \
    "Should the averaged or unaveraged detection map be used to generate " \
    "the output detection mask." ); \
  add_param( \
    detection_image_folder, \
    std::string, \
    "", \
    "If set, images showing detection maps and detections will be dumped to " \
    "this folder for every input frame." ); \
  add_param( \
    detection_image_ext, \
    std::string, \
    "png", \
    "If detection_image_folder is set, the file type to use for images." ); \
  add_array( \
    detection_scaling_dims, \
    double, 2, \
    "1.0 0.0", \
    "Before output to the detection image folder, image values will be scaled " \
    "and shifted by these two values." ); \
  add_param( \
    detection_image_offset, \
    int, \
    -1, \
    "If detection_image_folder and buffering is set, the offset to the image " \
    "to display detections on (the default is the center of the buffer)." ); \
  add_param( \
    reload_key, \
    std::string, \
    "", \
    "If set, the internal CNN caffe model will never be reloaded after " \
    "the initial load. The key value is a unique identifier in case there " \
    "are multiple detectors in the pipeline (which can also be used to " \
    "share models across different detectors if they share the same key)." ); \

init_external_settings3( cnn_detector_settings, settings_macro )

#undef settings_macro

/// \brief A class for computing CNN-based descriptors from a single frame.
template< class PixType >
class cnn_detector
{

public:

  typedef vil_image_view< PixType > input_image_t;

  typedef float cnn_float_t;
  typedef vil_image_view< cnn_float_t > float_image_t;

  typedef vil_image_view< float > detection_map_t;
  typedef vil_image_view< bool > detection_mask_t;

  typedef std::pair< vgl_box_2d< unsigned >, double > detection_t;
  typedef std::vector< detection_t > detection_vec_t;

  cnn_detector();
  virtual ~cnn_detector();

  /// Load models.
  bool configure( const cnn_detector_settings& settings );

  /// Compute detection maps for a single image.
  void detect( const input_image_t& image, detection_map_t& map );

  /// Compute detection mask for a single image.
  void detect( const input_image_t& image, detection_mask_t& mask );

  /// Compute detections from a single image.
  void detect( const input_image_t& image, detection_vec_t& detections );

  /// Compute detections from a single image.
  void detect( const input_image_t& image,
               detection_map_t& map,
               detection_mask_t& mask,
               detection_vec_t& detections );

  /// Compute detection maps for multiple images.
  void batch_detect( const std::vector< input_image_t >& images,
                     std::vector< detection_map_t >& maps );

  /// Compute detection maps for multiple images.
  void batch_detect( const std::vector< input_image_t >& images,
                     std::vector< detection_mask_t >& masks );

  /// Compute detections for multiple images.
  void batch_detect( const std::vector< input_image_t >& images,
                     std::vector< detection_vec_t >& detections );

  /// Compute detections for multiple images.
  void batch_detect( const std::vector< input_image_t >& images,
                     std::vector< detection_map_t >& maps,
                     std::vector< detection_mask_t >& masks,
                     std::vector< detection_vec_t >& detections );

private:

  // Class for encapsulating member variables
  class priv;
  const std::unique_ptr<priv> d;

  // Internal helper functions
  void debug_output( const detection_map_t& map,
                     const std::string& key,
                     const unsigned id = 0 );

  void debug_output( const input_image_t& image,
                     const std::string& key,
                     const unsigned id = 0,
                     const detection_vec_t detections = detection_vec_t() );

  void copy_image_to_blob( const vil_image_view< PixType >& source,
                           vil_image_view< cnn_float_t >& dest );

  void gen_detections_box( const detection_map_t& map,
                           const detection_mask_t& mask,
                           detection_vec_t& detections );

  void gen_detections_blob( const detection_map_t& map,
                            const detection_mask_t& mask,
                            detection_vec_t& detections );

};

} // end namespace vidtk

#endif
