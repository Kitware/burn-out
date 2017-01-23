/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_simple_color_histogram_generator_h_
#define vidtk_simple_color_histogram_generator_h_

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/color_descriptor_helpers_opencv.h>

#include <utilities/image_histogram.h>
#include <utilities/config_block.h>

#include <queue>
#include <vector>
#include <string>

#include <boost/circular_buffer.hpp>

namespace vidtk
{


#define settings_macro( add_param ) \
  add_param( \
    descriptor_name, \
    std::string, \
    "simple_color_histogram", \
    "Descriptor ID to output" ); \
  add_param( \
    num_threads, \
    unsigned int, \
    1, \
    "Number of threads to use" ); \
  add_param( \
    resolution_per_chan, \
    unsigned int, \
    8, \
    "Number of threads to use" ); \
  add_param( \
    descriptor_length, \
    unsigned int, \
    20, \
    "Each descriptor is computed over this many frames" ); \
  add_param( \
    descriptor_spacing, \
    unsigned int, \
    0, \
    "Have a new descriptor start every n frames" ); \
  add_param( \
    descriptor_skip_count, \
    unsigned int, \
    1, \
    "When running the descriptor, only sample 1 in n frames" ); \
  add_param( \
    smooth_histogram, \
    bool, \
    true, \
    "Should we perform histogram smoothing in histogram space?" ); \
  add_param( \
    smooth_image, \
    bool, \
    true, \
    "Should we smooth the image before extracting histograms?" ); \
  add_param( \
    min_frames_for_incomplete, \
    unsigned int, \
    5, \
    "Minimum required frames to formulate a descriptor for incomplete tracks" ); \
  add_param( \
    duplicate_name, \
    std::string, \
    "", \
    "(Deprecated) Duplicate descriptor ID, output with the same ID." ); \

init_external_settings( simple_color_histogram_settings, settings_macro )

#undef settings_macro


/// A generator for computing test hog descriptors in an online manner
template <typename PixType = vxl_byte>
class simple_color_histogram_generator : public online_descriptor_generator
{

public:

  simple_color_histogram_generator();
  ~simple_color_histogram_generator() {}

  bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  // Any data we want stored for each individual track we encounter
  struct hist_track_data : track_data_storage
  {
    /// Are we currenly computing a descriptor or in skip mode?
    bool compute_mode;

    /// # of frames observed on this track
    unsigned int frames_observed;

    /// A pointer to some partially complete descriptor we are currently computing
    descriptor_sptr wip;

    /// Color histogram used before we dump its information into the descriptor
    Histogram3D hist;
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

private:

  /// Settings which can be set externally by the caller
  simple_color_histogram_settings settings_;

  /// Computed total resolution of the output histogram
  unsigned total_resolution_;
};

} // end namespace vidtk

#endif // vidtk_simple_color_hist_generator_h_
