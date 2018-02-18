/*ckwg +5
 * Copyright 2015-2017 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_cnn_generator_h_
#define vidtk_cnn_generator_h_

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/cnn_descriptor.h>

#include <utilities/config_block.h>
#include <utilities/external_settings.h>

#include <queue>
#include <vector>

namespace vidtk
{

#define settings_macro( add_param, add_array, add_enumr ) \
  add_param( \
    descriptor_prefix, \
    std::string, \
    "cnn_descriptor_", \
    "Descriptor ID prefix to use in conjunction with the layer string" ); \
  add_param( \
    num_cpu_threads, \
    unsigned int, \
    1, \
    "Number of CPU threads to use" ); \
  add_param( \
    descriptor_length, \
    unsigned int, \
    10, \
    "Each descriptor is computed over this many frames" ); \
  add_param( \
    descriptor_spacing, \
    unsigned int, \
    10, \
    "Have a new descriptor start every n frames" ); \
  add_param( \
    descriptor_skip_count, \
    unsigned int, \
    2, \
    "When running the descriptor, only sample 1 in n frames" ); \
  add_param( \
    bb_scale_factor, \
    double, \
    1.2, \
    "Percent [1,inf] to scale (increase) detected track BB by" ); \
  add_param( \
    bb_pixel_shift, \
    unsigned int, \
    0, \
    "Number of pixels to shift (expand) the detected track BB by" ); \
  add_param( \
    min_frames_for_desc, \
    unsigned int, \
    5, \
    "Minimum required frames to formulate a descriptor for incomplete " \
    "tracks" ); \
  add_param( \
    new_states_w_bbox_only, \
    bool, \
    true, \
    "If this flag is set, we will only compute a new descriptor on states with " \
    "new bounding boxes" ); \
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
    chip_length, \
    int, \
    256, \
    "Image chip boundary length to use for CNN computation." ); \
  add_param( \
    stretch_chip, \
    bool, \
    false, \
    "Whether or not each track bounding box is stretched to occupy the " \
    "entire CNN input, or if the aspect ratio of the original video " \
    "is maintained." ); \
  add_param( \
    normalize_descriptors, \
    bool, \
    false, \
    "Whether or not every output descriptor should be normalized (L2)." ); \
  add_param( \
    chip_extraction_prefix, \
    std::string, \
    "", \
    "If this string is non-empty, we will not output any descriptors and " \
    "instead dump image chips to this directory" ); \
  add_enumr( \
    temporal_pooling_method, \
    (SUM) (MAX), \
    MAX, \
    "Temporal pooling option for merging CNN descriptors produced on single " \
    "frames into track descriptors" ); \
  add_param( \
    output_size_info, \
    bool, \
    false, \
    "Whether or not descriptor size information will be output to console." ); \
  add_array( \
    final_layer_indices, \
    unsigned, 0, \
    "", \
    "Can optionally be set to an arbitrary number of indices corresponding " \
    "to which CNN outputs we want to include in the final descriptor." ); \
  add_param( \
    set_pvo_flag, \
    bool, \
    false, \
    "(Deprecated) Should this descriptor set the is pvo flag?" ); \

init_external_settings3( cnn_settings, settings_macro )

#undef settings_macro


/// A generator for computing cnn descriptors around tracks
template <typename PixType = vxl_byte>
class cnn_generator : public online_descriptor_generator
{

public:

  cnn_generator();
  ~cnn_generator() {}

  bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  // Any data we want stored for each individual track we encounter
  struct cnn_track_data : track_data_storage
  {
    /// Are we currenly computing a descriptor or in skip mode?
    bool compute_mode;

    /// # of frames observed on this track since last descriptor output
    unsigned int frames_observed;

    /// Timestamp of the last from used for descriptor computation
    timestamp last_state_ts;

    /// A pointer to partially complete descriptors we are currently computing
    std::vector< descriptor_sptr > descs;
  };

  // Used to set and initialize any track data we might want to store for the tracks
  // duration, this data will be provided via future step function calls.
  virtual track_data_sptr new_track_routine( track_sptr const& new_track );

  // Standard update function called for all active tracks on the current frame.
  virtual bool track_update_routine( track_sptr const& active_track,
                                     track_data_sptr track_data );

  // Called everytime a track is terminated, in case we want to output any partially
  // complete descriptors.
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data );

  // Called once per frame to determine what optimizations to turn on.
  virtual bool frame_update_routine();

private:

  /// Settings which can be set externally by the caller
  cnn_settings settings_;

  /// Object used for computing single frame descriptors
  cnn_descriptor descriptor_;

  /// Are we in training mode?
  bool is_training_mode_;

  /// Internal mutex for using the descriptor class
  boost::mutex mutex_;

  /// Copy of the current frame
  vil_image_view< PixType > current_frame_;

  /// IDs of all types of descriptors
  std::vector< descriptor_id_t > descriptor_ids_;

  /// Helper function which computes the desired image snippet to compute
  /// cnn features on, given the input image and some initial track bbox
  bool compute_track_snippet( vil_image_view< PixType > const& image,
                              vgl_box_2d< unsigned > const& region,
                              vil_image_view< vxl_byte >& output );

};

} // end namespace vidtk


#endif // vidtk_cnn_generator_
