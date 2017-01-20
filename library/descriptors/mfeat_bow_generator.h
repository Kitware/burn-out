/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_mfeat_bow_generator_h_
#define vidtk_mfeat_bow_generator_h_

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/mfeat_bow_descriptor.h>

#include <utilities/config_block.h>

#include <queue>
#include <vector>

namespace vidtk
{


/// Any external settings we want to define for users of this hog generator
#define settings_macro( add_param ) \
  add_param( \
    descriptor_name, \
    std::string, \
    "mfeat_bow", \
    "Descriptor ID to output" ); \
  add_param( \
    bow_filename, \
    std::string, \
    "mfeat_bow_model_filename", \
    "Filename of the BoW vocabulary file" ); \
  add_param( \
    num_threads, \
    unsigned int, \
    4, \
    "Number of threads to use" ); \
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
    1.25, \
    "Percent [1,inf] to scale (increase) detected track BB by" ); \
  add_param( \
    bb_pixel_shift, \
    unsigned int, \
    0, \
    "Number of pixels to shift (expand) the detected track BB by" ); \
  add_param( \
    enable_adaptive_resize, \
    bool, \
    true, \
    "Should we attempt to normalize the size of each input region?" ); \
  add_param( \
    desired_min_area, \
    double, \
    6000, \
    "Desired minimum area for descriptor computation" ); \
  add_param( \
    desired_max_area, \
    double, \
    8000, \
    "Desired maximum area for descriptor computation" ); \
  add_param( \
    min_frames_for_desc, \
    unsigned int, \
    5, \
    "Minimum required frames to formulate a descriptor for incomplete " \
    "tracks" ); \
  add_param( \
    min_chip_length, \
    unsigned, \
    16, \
    "Minimum final image chip dimension required to compute features" ); \
  add_param( \
    compute_hessian, \
    bool, \
    false, \
    "Should we compute hessian features?" ); \
  add_param( \
    compute_harris_laplace, \
    bool, \
    true, \
    "Should we compute harris laplace features?" ); \
  add_param( \
    compute_dog, \
    bool, \
    true, \
    "Should we compute difference of gaussian features?" ); \
  add_param( \
    compute_hessian_laplace, \
    bool, \
    false, \
    "Should we compute hessian laplace features?" ); \
  add_param( \
    compute_multiscale_hessian, \
    bool, \
    true, \
    "Should we compute multiscalar hessian features?" ); \
  add_param( \
    compute_multiscale_harris, \
    bool, \
    false, \
    "Should we compute multiscalar harris features?" ); \
  add_param( \
    compute_mser, \
    bool, \
    false, \
    "Should we compute mser features?" ); \
  add_param( \
    duplicate_name, \
    std::string, \
    "", \
    "(Deprecated) Duplicate descriptor ID, output with the same ID." ); \

init_external_settings( mfeat_bow_settings, settings_macro )

#undef settings_macro

/// A generator for computing mfeat descriptors around tracks
template <typename PixType = vxl_byte>
class mfeat_bow_generator : public online_descriptor_generator
{

public:

  mfeat_bow_generator();
  ~mfeat_bow_generator() {}

  bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  // Any data we want stored for each individual track we encounter
  struct mfeat_track_data : track_data_storage
  {
    /// Are we currenly computing a descriptor or in skip mode?
    bool compute_mode;

    /// # of frames observed on this track
    unsigned int frames_observed;

    /// A pointer to some partially complete descriptor we are currently computing
    descriptor_sptr wip;

    /// Temp buffer for holding computed features (saves time reallocating)
    std::vector< float > features;

    /// Temp buffer for holding computed bow mappins (saves time reallocating)
    std::vector< double > bow_mapping;
  };

  // Used to set and initialize any track data we might want to store for the tracks
  // duration, this data will be provided via future step function calls
  virtual track_data_sptr new_track_routine( track_sptr const& new_track );

  // Standard update function called for all active tracks on the current
  // frame.
  virtual bool track_update_routine( track_sptr const& active_track,
                                     track_data_sptr track_data );

  // Called everytime a track is terminated, in case we want to output
  // any partially complete descriptors
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data );

  // Called once per frame to determine what optimizations to turn on
  virtual bool frame_update_routine();

private:

  /// Settings which can be set externally by the caller
  mfeat_bow_settings settings_;

  /// The loaded BoW vocabulary
  mfeat_bow_descriptor computer_;

  /// Size of loaded BoW vocabulary
  unsigned int vocabulary_size_;

  /// Was a vocabulary model loaded successfully?
  bool is_model_loaded_;

  /// Reference to the current image
  vil_image_view<PixType> current_frame_;

  /// Helper function which computes the desired image snippet to compute
  /// mfeat features on, given the input image and some initial track bbox
  void compute_track_snippet( const vil_image_view<PixType>& img,
                              const vgl_box_2d<unsigned>& region,
                              vil_image_view<float>& output );

};

} // end namespace vidtk


#endif // vidtk_mfeat_bow_generator_
