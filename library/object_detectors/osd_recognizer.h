/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_osd_recognizer_h_
#define vidtk_osd_recognizer_h_

#include <vil/vil_image_view.h>
#include <vil/vil_rgb.h>
#include <vgl/vgl_box_2d.h>

#include <video_properties/border_detection_process.h>

#include <utilities/ring_buffer.h>
#include <utilities/timestamp.h>
#include <utilities/point_view_to_region.h>
#include <utilities/external_settings.h>

#include <classifier/hashed_image_classifier.h>

#include <object_detectors/scene_obstruction_detector.h>
#include <object_detectors/osd_template.h>
#include <object_detectors/text_parser.h>

#ifdef USE_CAFFE
#include <object_detectors/cnn_detector.h>
#endif

#include <learning/adaboost.h>

#include <vector>
#include <limits>
#include <string>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

/// \brief Settings for the osd_recognizer class
///
/// These settings are declared in macro form to prevent code duplication
/// in multiple places (e.g. initialization, file parsing, etc...).
#define settings_macro( add_param, add_array, add_enumr ) \
  add_param( \
    use_templates, \
    bool, \
    false, \
    "Should we use templates in formulating the output mask?" ); \
  add_param( \
    template_filename, \
    std::string, \
    "", \
    "Filepath to the known templates list." ); \
  add_param( \
    template_threshold, \
    double, \
    0.0, \
    "Threshold to apply to the template classifier." ); \
  add_param( \
    use_most_common_template, \
    bool, \
    true, \
    "Whether or not to use the most common detected template, as opposed to " \
    "the detected result for each individual frame." ); \
  add_param( \
    use_resolution, \
    bool, \
    true, \
    "Should we use image resolution during template recognition?" ); \
  add_param( \
    classifier_threshold, \
    double, \
    0.0, \
    "Threshold to apply to the static classification image." ); \
  add_param( \
    descriptor_width_bins, \
    unsigned, \
    10, \
    "Template descriptor spatial bins per image width." ); \
  add_param( \
    descriptor_height_bins, \
    unsigned, \
    8, \
    "Template descriptor spatial bins per image height." ); \
  add_param( \
    descriptor_area_bins, \
    unsigned, \
    18, \
    "Template descriptor area bins per spatial region." ); \
  add_param( \
    descriptor_output_file, \
    std::string, \
    "", \
    "Output filename for writing descriptors to, if required." ); \
  add_param( \
    filter_lone_pixels, \
    bool, \
    true, \
    "Should we prefilter lone pixels with no neighbors in the output mask?" ); \
  add_param( \
    use_initial_approximation, \
    bool, \
    false, \
    "Should we use the initial approximation in addition to the " \
    "averaged result in formulating the output mask?" ); \
  add_param( \
    fill_color_extremes, \
    bool, \
    false, \
    "If set, and the OSD is primarily an extreme color (bright green, blue) " \
    "take extra steps to fill in all extreme values." ); \
  add_param( \
    full_dilation_amount, \
    double, \
    0.0, \
    "Dilation radius (pixels) for all detections." ); \
  add_param( \
    static_dilation_amount, \
    double, \
    0.0, \
    "Extra dilation radius (pixels) for the resultant static detections." ); \
  add_enumr( \
    use_gpu_override, \
    (YES) (NO) (AUTO) (NO_OVERRIDE), \
    NO_OVERRIDE, \
    "Whether or not to use a GPU for processing for all refinement actions " \
    "covered by this class, if set to no_override it will be up to the " \
    "individual action config to determine whether or not to use the GPU." ); \

init_external_settings3( osd_recognizer_settings, settings_macro )

#undef settings_macro


/// \brief Class containing any models specific to a single type of OSD
/// that are used to address all action items for the OSD template.
///
/// The models stored in this class are loaded by the osd_recognizer class
/// in order to keep them all in the same place for easy access. They
/// are used by refinement stages of the pipeline after recognizing the
/// class of OSD which appears on the video.
template <typename PixType, typename FeatureType>
class osd_action_items
{
public:

  typedef vil_image_view< bool > mask_image_t;
  typedef hashed_image_classifier< FeatureType > pixel_classifier_t;
  typedef text_parser_instruction< PixType > text_instruction_t;
  typedef std::vector< text_instruction_t > text_instruction_set_t;
  typedef std::string dynamic_instruction_set_t;

  osd_action_items() : template_matching( false ) {}
  ~osd_action_items() {}

  /// The textual ID of the this OSD.
  std::string osd_type_id;

  /// A list of text parsing instructions for this OSD.
  text_instruction_set_t parse_instructions;

  /// A pointer to a file containing template matching settings.
  dynamic_instruction_set_t dyn_instruction_1;
  dynamic_instruction_set_t dyn_instruction_2;

  /// Any pixel classifiers that are a part of this model.
  std::map< std::string, pixel_classifier_t > clfrs;

  /// Any pixel masks that are a part of this model.
  std::map< std::string, mask_image_t > masks;

  /// Does this obstruction require any secondary template matching?
  bool template_matching;

  /// Any other actions to be taken when this OSD is recognized.
  std::vector< osd_action_item > actions;

#ifdef USE_CAFFE
  typedef cnn_detector< PixType > cnn_detector_t;

  /// Any CNN classifiers required to perform this action
  std::map< std::string, cnn_detector_t > cnn_clfrs;
#endif
};


/// \brief Any information related to OSD detection for the current frame.
///
/// This class is one of the possible outputs produced by the recognizer
/// class in order to keep all models in one place for downstream
/// processes. It also stores a copy of all of the features extracted
/// on the current frame.
template <typename PixType, typename FeatureType>
class osd_recognizer_output
{
public:

  typedef vil_image_view< PixType > source_image_t;
  typedef vil_image_view< FeatureType > feature_image_t;
  typedef std::vector< feature_image_t > feature_array_t;
  typedef vil_image_view< double > classified_image_t;
  typedef scene_obstruction_properties< PixType > properties_t;
  typedef osd_action_items< PixType, FeatureType > osd_action_items_t;
  typedef boost::shared_ptr< const osd_action_items_t > osd_action_items_sptr_t;

  osd_recognizer_output() {}
  ~osd_recognizer_output() {}

  // Initial detection images and their properties properties.
  source_image_t source;
  image_border border;
  feature_array_t features;
  classified_image_t classifications;
  properties_t properties;
  std::string type;

  // A pointer to the detected OSD model for this frame, if known.
  osd_action_items_sptr_t actions;
};


/// \brief Class which recognizes the type of OSD being displayed.
///
/// At the core of this class are external templates, which specify
/// how each type of OSD is recognized, and model groups which contain
/// all models related to each OSD type used to refine the final OSD
/// mask.
template <typename PixType, typename FeatureType>
class osd_recognizer
{

public:

  typedef vil_image_view< PixType > source_image_t;
  typedef vil_image_view< FeatureType > feature_image_t;
  typedef std::vector< feature_image_t > feature_array_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef vil_image_view< double > classified_image_t;
  typedef scene_obstruction_properties< PixType > properties_t;
  typedef adaboost template_classifier_t;

  typedef text_parser_instruction< PixType > text_instruction_t;
  typedef std::vector< text_instruction_t > text_instruction_set_t;
  typedef std::string dynamic_instruction_set_t;

  typedef osd_recognizer_output< PixType, FeatureType > osd_recognizer_output_t;
  typedef boost::shared_ptr< osd_recognizer_output_t > osd_recognizer_output_sptr_t;
  typedef osd_action_items< PixType, FeatureType > osd_action_items_t;
  typedef boost::shared_ptr< osd_action_items_t > osd_action_items_sptr_t;

  osd_recognizer() {}
  virtual ~osd_recognizer() {}

  /// Configure the recognizer with new settings.
  bool configure( const osd_recognizer_settings& options );

  /// \brief Process a new frame outputting information about its OSD.
  ///
  /// The final mask used for computing the OSD type (after internal
  /// morphology has been performed) is also returned.
  osd_recognizer_output_sptr_t process_frame(
    const source_image_t& input_image,
    const feature_array_t& input_features,
    const image_border& input_border,
    const classified_image_t& input_detections,
    const properties_t& input_properties,
    mask_image_t& output_mask );

  /// Get the list of internally loaded templates.
  std::vector< osd_template > const& get_templates() const;

  /// Does any loaded template contain a text parsing instruction?
  bool contains_text_option() const;

private:

  // Settings set externally
  osd_recognizer_settings options_;

  // Individual OSD templates and models
  std::vector< osd_template > templates_;
  std::vector< osd_action_items_sptr_t > actions_;
  std::map< std::string, template_classifier_t > template_classifiers_;

  ring_buffer< int > template_est_history_;
  std::string template_path_;
  std::vector< double > size_intervals_;
  bool contains_text_option_;
  int last_template_;
  std::map< int, int > template_hist_;

  // Simple smoothing options for final mask output
  vil_structuring_element static_dilation_el_;
  vil_structuring_element approximation_dilation_el_;

  // Internal buffer images to prevent continuous reallocation
  mask_image_t thresholded_static_image_;
  mask_image_t thresholded_full_image_;
  mask_image_t morphed_full_image_;
  vil_image_view< unsigned > labeled_blobs_;

  // Helper functions:

  // Selects which template is shown on the video
  int select_template( const mask_image_t& components,
                       const properties_t& props );

  // Filter out individual high pixels with no neighbors
  void filter_lone_pixels( mask_image_t& image );

  // Generate an obstruction template type descriptor
  void generate_descriptor( const unsigned ni, const unsigned nj,
                            const std::vector< osd_symbol >& symbols,
                            std::vector< double >& descriptor );

  // Detect color extremes which are likely to be OSD components
  void fill_color_extremes( const source_image_t& input_image,
                            mask_image_t& output_mask,
                            const properties_t& input_properties );

};


} // end namespace vidtk


#endif // vidtk_osd_recognizer_h_
