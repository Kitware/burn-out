/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_dpm_tot_generator_h_
#define vidtk_dpm_tot_generator_h_

#include <descriptors/online_descriptor_generator.h>

#include <utilities/config_block.h>

#include <opencv2/objdetect/objdetect.hpp>

#include <boost/scoped_ptr.hpp>

#include <string>

namespace vidtk
{

/// \brief Settings for the dpm_tot_generator class
///
/// These settings are declared in macro form to prevent code duplication
/// in multiple places (e.g. initialization, file parsing, etc...).
#define settings_macro( add_param, add_array, add_enumr ) \
  add_param( \
    descriptor_id, \
    std::string, \
    "DPM-TOT-Descriptor", \
    "Descriptor ID to output" ); \
  add_param( \
    person_model_filename, \
    std::string, \
    "", \
    "Filename for person DPM model" ); \
  add_param( \
    vehicle_model_filename, \
    std::string, \
    "", \
    "Filename for vehicle DPM model" ); \
  add_enumr( \
    output_mode, \
    (TOT_VALUES) (AGGREGATE_DESCRIPTOR), \
    TOT_VALUES, \
    "Output descriptor type, see class description" ); \
  add_param( \
    thread_count, \
    unsigned, \
    4, \
    "Default number of threads to use for generation" ); \
  add_param( \
    compute_sampling_rate, \
    unsigned, \
    5, \
    "Frame downsampling factor for the descriptor (ie, run every nth frame)" ); \
  add_param( \
    output_sampling_rate, \
    unsigned, \
    0, \
    "If non-zero, output raw descriptors every on this nth frame, regardless " \
    "of when they are computed, otherwise they will be output on every compute " \
    "frame." ); \
  add_param( \
    time_difference_trigger, \
    double, \
    2.5, \
    "If the time difference (sec) between the last time the detector was run " \
    "is greater than this threshold, re-run the detector on the next frame " \
    "Over-riding any frame downsampling settings" ); \
  add_param( \
    overlap_threshold, \
    float, \
    0.5, \
    "The Overlap threshold passed to the detect function" ); \
  add_param( \
    bb_scale_factor, \
    double, \
    1.50, \
    "Factor [1,inf] to scale (increase) detected track bounding box dims" ); \
  add_param( \
    bb_pixel_shift, \
    unsigned int, \
    1, \
    "Number of pixels to shift (expand) the detected track bounding box dims" ); \
  add_param( \
    process_grayscale, \
    bool, \
    false, \
    "Compute features over color images or just grayscale" ); \
  add_param( \
    enable_adaptive_resize, \
    bool, \
    true, \
    "Should we attempt to normalize the size of each input region " \
    "after scaling the track bounding boxes?" ); \
  add_param( \
    required_min_area, \
    double, \
    10, \
    "Required min area for track bounding boxes to be used for " \
    "descriptor computation." ); \
  add_param( \
    desired_min_area, \
    double, \
    8000, \
    "Desired minimum area for DPM computation in pixels^2" ); \
  add_param( \
    desired_max_area, \
    double, \
    10000, \
    "Desired maximum area for DPM computation in pixels^2" ); \
  add_param( \
    new_states_w_bbox_only, \
    bool, \
    true, \
    "If this flag is set, we will only run the classifier algorithm on " \
    "track states with current bounding boxes" ); \
  add_param( \
    output_detection_folder, \
    std::string, \
    "", \
    "If the string is set, a folder to output detection images into" ); \
  add_param( \
    no_vehicles, \
    bool, \
    false, \
    "If this variable is set, we will assume this environment has no vehicles" ); \
  add_param( \
    max_upscale_factor, \
    double, \
    10.0, \
    "Maximum image upscaling factor for adaptive resizing" ); \
  add_param( \
    person_classifier_threshold, \
    double, \
    -0.6, \
    "We must have a detection of at least this magnitude on a track in order " \
    "for it to be considered a person" ); \
  add_param( \
    vehicle_classifier_threshold, \
    double, \
    -0.7, \
    "We must have a detection of at least this magnitude on a track in order " \
    "for it to be considered a vehicle" ); \
  add_param( \
    lowest_detector_score, \
    double, \
    -1.0, \
    "The lowest detector score that can be output from either of the detectors" ); \
  add_param( \
    raw_threshold_1, \
    double, \
    -0.5, \
    "Threshold used in raw mode for features 15-18 of the raw descriptor." ); \
  add_param( \
    raw_threshold_2, \
    double, \
    0.0, \
    "Threshold used in raw mode for features 19-22 of the raw descriptor." ); \

init_external_settings3( dpm_tot_settings, settings_macro )

#undef settings_macro

/// \brief OpenCV HOG DPM Descriptor
///
/// This descriptor generates one of two possible outputs.
///
/// Either a TOT summary containing the following values:
///
/// 0. Person probability
/// 1. Vehicle probability
/// 2. Other probability
///
/// Or an aggregate descriptor containing the following:
///
/// 0. Number of total times detector was run
/// 1. Number of positive person classifications near target
/// 2. Number of positive vehicle classifications near target
/// 3. Average person classification weight
/// 4. Average vehicle classification weight
/// 5. Maximum person classification weight
/// 6. Maximum vehicle classification weight
/// 7. Percent of frames with people detections
/// 8. Percent of frames with vehicle detections
/// 9. Normalized person/vehicle count ratio, if more person scores
/// 10. Normalized vehicle/person count ratio, if more vehicle scores
/// 11. Person 75th percentile classification weight
/// 12. Vehicle 75th percentile classification weight
/// 13. Person average BBOX match score
/// 14. Vehicle average BBOX match score
/// 15. Person threshold 1 average classification weight
/// 16. Vehicle threshold 1 average classification weight
/// 17. Person threshold 1 BBOX match score
/// 18. Vehicle threshold 1 BBOX match score
/// 19. Person threshold 2 average classification weight
/// 20. Vehicle threshold 2 average classification weight
/// 21. Person threshold 2 BBOX match score
/// 22. Vehicle threshold 2 BBOX match score
///
class dpm_tot_generator : public online_descriptor_generator
{

public:

  typedef cv::LatentSvmDetector dpm_detector_t;
  typedef cv::LatentSvmDetector::ObjectDetection detection_t;
  typedef std::vector< detection_t > detection_list_t;
  typedef cv::Mat ocv_image_t;
  typedef vgl_box_2d< unsigned > bbox_t;
  typedef vil_image_view< vxl_byte > vxl_image_t;

  /// TOT descriptor bin labels
  enum
  {
    PERSON_PROBABILITY_BIN = 0,
    VEHICLE_PROBABILITY_BIN = 1,
    OTHER_PROBABILITY_BIN = 2,
    TOTAL_TOT_BINS = 3
  };

  /// Raw descriptor bin labels
  enum
  {
    TOTAL_RUN_COUNT = 0,
    POS_PERSON_DET = 1,
    POS_VEHICLE_DET = 2,
    AVG_PERSON_SCORE = 3,
    AVG_VEHICLE_SCORE = 4,
    MAX_PERSON_SCORE = 5,
    MAX_VEHICLE_SCORE = 6,
    POS_PERSON_PER = 7,
    POS_VEHICLE_PER = 8,
    PERSON_RATIO_SCORE = 9,
    VEHICLE_RATIO_SCORE = 10,
    PERSON_75TH_SCORE = 11,
    VEHICLE_75TH_SCORE = 12,
    PERSON_BBOX_SCORE = 13,
    VEHICLE_BBOX_SCORE = 14,
    PERSON_THRESH1_SCORE = 15,
    VEHICLE_THRESH1_SCORE = 16,
    PERSON_THRESH1_BBOX = 17,
    VEHICLE_THRESH1_BBOX = 18,
    PERSON_THRESH2_SCORE = 19,
    VEHICLE_THRESH2_SCORE = 20,
    PERSON_THRESH2_BBOX = 21,
    VEHICLE_THRESH2_BBOX = 22,
    TOTAL_FEATURE_COUNT = 23
  };

  /// Constructor.
  dpm_tot_generator();

  /// Destructor.
  virtual ~dpm_tot_generator();

  /// Set any settings for this descriptor generator.
  virtual bool configure( const external_settings& settings );
  virtual external_settings* create_settings();

protected:

  /// Called the first time we receive a new track.
  virtual track_data_sptr new_track_routine( track_sptr const& new_track );

  /// Standard update function.
  virtual bool track_update_routine( track_sptr const& active_track,
                                     track_data_sptr track_data );

  /// Handle terminated tracks.
  virtual bool terminated_track_routine( track_sptr const& finished_track,
                                         track_data_sptr track_data );

private:

  // Settings which can be set externally by the caller
  dpm_tot_settings settings_;

  // OpenCV detector objects
  boost::scoped_ptr< dpm_detector_t > person_detector_;
  boost::scoped_ptr< dpm_detector_t > vehicle_detector_;

  // Helper function which computes the desired image snippet to compute
  // detections over, given the input image and some initial track bbox
  bool compute_track_snippet( const frame_data_sptr input_frame,
                              const bbox_t& region,
                              ocv_image_t& output,
                              double& new_target_area );

  // Helper function which runs a DPM detector on some image
  bool run_detector( const ocv_image_t& image,
                     dpm_detector_t& detector,
                     detection_list_t& output );
};

} // end namespace vidtk

#endif // dpm_tot_generator_h_
