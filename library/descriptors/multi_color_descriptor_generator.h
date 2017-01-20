/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_multi_color_descriptor_generator_h_
#define vidtk_multi_color_descriptor_generator_h_

#include <descriptors/online_descriptor_generator.h>
#include <descriptors/color_descriptor_helpers_opencv.h>

#include <video_transforms/automatic_white_balancing.h>
#include <video_transforms/color_commonality_filter.h>

#include <utilities/config_block.h>

#include <queue>
#include <vector>

#include <boost/circular_buffer.hpp>

namespace vidtk
{


#define settings_macro( add_param ) \
  add_param( \
    color_space, \
    std::string, \
    "RGB", \
    "Color space to formulate histogram over, options: RGB, Lab, HSV" ); \
  add_param( \
    perform_auto_white_balancing, \
    bool, \
    true, \
    "Should we perform automatic white balancing before generating " \
    "descriptors?" ); \
  add_param( \
    use_color_commonality, \
    bool, \
    true, \
    "Should we weight colors less common in the entire image with higher " \
    "weights in the resultant histograms?" ); \
  add_param( \
    use_track_mask, \
    bool, \
    true, \
    "Should we weight colors in the inputted track masks higher (if " \
    "present) in the resultant histograms?" ); \
  add_param( \
    num_threads, \
    unsigned int, \
    1, \
    "Number of threads to use" ); \
  add_param( \
    resolution_per_chan, \
    unsigned int, \
    8, \
    "Resolution per each color channel of the utilized histogram" ); \
  add_param( \
    descriptor_length, \
    unsigned int, \
    25, \
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
    filter_dark_pixels, \
    bool, \
    false, \
    "Should we give darker (less saturated) pixels lower weights?" ); \
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
    10, \
    "Minimum required frames to formulate a descriptor for incomplete " \
    "tracks" ); \
  add_param( \
    histogram_output_id, \
    std::string, \
    "weighted_color_histogram", \
    "Output name for the histogram descriptor variant" ); \
  add_param( \
    single_value_output_id, \
    std::string, \
    "average_color_value", \
    "Output name for the single point value descriptor" ); \
  add_param( \
    person_output_id, \
    std::string, \
    "person_color_summary", \
    "Output name for the person object-specific descriptor" ); \
  add_param( \
    duplicate_histogram_output_id, \
    std::string, \
    "", \
    "Output name for the histogram descriptor variant" ); \
  add_param( \
    duplicate_single_value_output_id, \
    std::string, \
    "", \
    "Output name for the single point value descriptor" ); \
  add_param( \
    duplicate_person_output_id, \
    std::string, \
    "", \
    "Output name for the person object-specific descriptor" ); \


init_external_settings( multi_color_descriptor_settings, settings_macro )

#undef settings_macro

/// \brief A generator for computing multiple different color descriptors.
///
/// The core of which is a weighted 3D histogram in some color space which includes
/// features such as white balancing, basic shadow compensation, and indentifying pixels
/// more likely to be part of our OOI. Should be finished converted off of opencv later.
template <typename PixType = vxl_byte>
class multi_color_descriptor_generator : public online_descriptor_generator
{

public:

  multi_color_descriptor_generator();
  virtual ~multi_color_descriptor_generator() {}

  /// Set any settings for this generator.
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
    MultiFeatureHist hist;
  };

  // Called once per frame to perform operations on the whole frame (white
  // balancing, cci...)
  virtual bool frame_update_routine();

  // Used to set and initialize any track data we might want to store for the
  // tracks duration, this data will be provided via future step function calls
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

  // Settings which can be set externally by the caller
  multi_color_descriptor_settings settings_;

  // Computed total resolution of the output histogram
  unsigned total_resolution_;

  // Color space to use
  enum { MF_RGB, MF_LAB, MF_HSV } color_space_;

  // Different modes for generating weight images (which indicate
  // how much weight to put on each pixel in the object's bbox)
  enum { MF_COMBO, MF_CCI, MF_TRACKMASK, MF_NONE } weight_mode_;

  // The current frame (after any initial filtering)
  auto_white_balancer< PixType > white_balancer_;
  vil_image_view< PixType > filtered_frame_;
  cv::Mat filtered_frame_ocv_;

  // The current color commonality image
  color_commonality_filter_settings cci_settings_;
  std::vector< unsigned > cci_histogram_;
  vil_image_view< float > color_commonality_image_;
  cv::Mat color_commonality_image_ocv_;

  // Helper function to output a finalized descriptor
  void output_descriptors_for_track( track_sptr const& active_track,
                                     hist_track_data& data );
};


} // end namespace vidtk


#endif // vidtk_multi_color_descriptor_generator_h_
