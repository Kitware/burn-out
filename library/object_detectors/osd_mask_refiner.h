/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_osd_mask_refiner_h_
#define vidtk_osd_mask_refiner_h_

#include <vil/vil_image_view.h>

#include <video_properties/border_detection_process.h>
#include <video_transforms/frame_averaging.h>

#include <utilities/ring_buffer.h>
#include <utilities/external_settings.h>

#include <object_detectors/osd_recognizer.h>
#include <object_detectors/pixel_feature_writer.h>

#include <boost/scoped_ptr.hpp>

#include <vector>

namespace vidtk
{

/// \brief Settings for the osd_mask_refiner class.
///
/// These settings are declared in macro form to prevent code duplication in
/// multiple places (e.g. initialization, file parsing, etc...).
#define settings_macro( add_param, add_array, add_enumr ) \
  add_param( \
    dilation_radius, \
    double, \
    0.0, \
    "Extra amount to always dilate the mask by after all other operations." ); \
  add_param( \
    closing_radius, \
    double, \
    0.0, \
    "Radius of closing operation to perform on the mask. 0 to disable." ); \
  add_param( \
    opening_radius, \
    double, \
    0.0, \
    "Radius of opening operation to perform on the mask. 0 to disable." ); \
  add_param( \
    reset_buffer_length, \
    unsigned, \
    0, \
    "After a OSD change is detected, buffer this many frames in order to " \
    "improve the OSD detections for the first few frames after the reset. " \
    "This occurs before any other operations. Buffered frames are still " \
    "processed, only outputted after the buffer fills up." ); \
  add_param( \
    refiner_index, \
    unsigned, \
    1, \
    "If there are multiple refiners in a pipeline, this field can be used " \
    "to differentiate between them if we want to run different refinement " \
    "actions on each node." ); \
  add_param( \
    apply_fills, \
    bool, \
    true, \
    "Whether or not fill operations should be performed by this algorithm." ); \
  add_param( \
    clfr_adjustment, \
    double, \
    0.0, \
    "Classifier adjustment made to all pixel classifiers used internally." ); \
  add_param( \
    training_file, \
    std::string, \
    "", \
    "If non-empty, this will mean we are in training mode and want to dump " \
    "out internal features to this file." ); \
  add_enumr( \
    training_mode, \
    (PIXEL_CLFR) (COMPONENT_CLFR), \
    PIXEL_CLFR, \
    "If training_mode is set, whether or not we are extracting features " \
    "for pixel or component classification." ) \
  add_param( \
    groundtruth_dir, \
    std::string, \
    "", \
    "Directory containing groundtruth image files for training mode." ); \

init_external_settings3( osd_mask_refiner_settings, settings_macro )

#undef settings_macro


/// \brief Given a recognized on-screen display shown on the video, this class
/// refines the output mask based on parameters stored in a configuration file.
///
/// This class is meant to be called in an online fashion, though depending on
/// some internal options it may buffer frames internally after a new type of
/// OSD is detected (therefore it may output no or several masks at once if
/// this option is set).
template <typename PixType, typename FeatureType>
class osd_mask_refiner
{

public:

  typedef vil_image_view< PixType > source_image_t;
  typedef vil_image_view< FeatureType > feature_image_t;
  typedef std::vector< feature_image_t > feature_array_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef vil_image_view< double > classified_image_t;
  typedef hashed_image_classifier< FeatureType > pixel_classifier_t;
  typedef adaboost component_classifier_t;
  typedef osd_recognizer_output< PixType, FeatureType > osd_info_t;
  typedef boost::shared_ptr< osd_info_t > osd_info_sptr_t;
  typedef pixel_feature_writer< FeatureType > feature_writer_t;
  typedef boost::scoped_ptr< feature_writer_t > feature_writer_sptr_t;
  typedef windowed_frame_averager< double > frame_averager_t;

  osd_mask_refiner() : frames_since_reset_( 0 ) {}
  virtual ~osd_mask_refiner() {}

  /// \brief Configure the refinement class with new settings.
  bool configure( const osd_mask_refiner_settings& options );

  /// \brief Modify an existing mask given any information about its OSD.
  ///
  /// This function will return no masks if the class is buffering masks
  /// internally, one mask in normal operations, and more than one mask
  /// when the minimum temporal smoothing criteria have been met for the
  /// first time after a new OSD has appeared on the video. Optionally,
  /// corresponding input OSD frame information can also be returned.
  void refine_mask( const mask_image_t& input_mask,
                    const osd_info_sptr_t input_osd_info,
                    std::vector< mask_image_t >& output_masks,
                    std::vector< osd_info_sptr_t >& output_osd_info );

  /// \brief Flush any internal masks which have yet to be output.
  void flush_masks( std::vector< mask_image_t >& output_masks,
                    std::vector< osd_info_sptr_t >& output_osd_info );

private:

  // Settings set externally
  osd_mask_refiner_settings options_;

  // Pre-computed kernels for morphology
  vil_structuring_element dilation_el_;
  vil_structuring_element closing_el_;
  vil_structuring_element opening_el_;

  // Temporal buffer for masks
  std::vector< mask_image_t > mask_buffer_;
  std::vector< osd_info_sptr_t > osd_info_buffer_;
  unsigned frames_since_reset_;
  std::map< unsigned, frame_averager_t > local_averages_;

  // Internal buffers to prevent frequent re-allocation
  mask_image_t morphology_buffer_;

  // Training mode variables
  bool pixel_training_mode_;
  bool component_training_mode_;
  feature_writer_sptr_t training_data_extractor_;

  // Helper functions used to respond to individual requested OSD actions:

  // Apply all actions stored in osd_info to the output mask
  void apply_actions( const osd_info_sptr_t osd_info,
                      mask_image_t& output_mask );

  // Classify a region in the image according to a classifier and input features
  void classify_region( const feature_array_t& features,
                        const image_region& region,
                        const pixel_classifier_t& clfr,
                        mask_image_t& output );

  // Classify a region in the image according to a classifier and input features
  void classify_region( const feature_array_t& features,
                        const image_region& region,
                        const pixel_classifier_t& clfr,
                        classified_image_t& output,
                        const double additional_offset = 0.0 );

  // Reclassify a region, overwriting any results currently in the output mask
  void reclassify_region( const feature_array_t& features,
                          const image_region& region,
                          const pixel_classifier_t& clfr,
                          mask_image_t& output,
                          const unsigned spec_option = 0,
                          const unsigned command_id = 0,
                          const double parameter1 = 0.0,
                          const double parameter2 = 0.0,
                          const double parameter3 = 0.0 );

  // Union the first mask and the second, placing the output in the second
  void union_region( const mask_image_t& mask1,
                     mask_image_t& mask2 );

  // Classify a region, not overwriting any results currently in the output mask
  void classify_and_union_region( const feature_array_t& features,
                                  const image_region& region,
                                  const pixel_classifier_t& clfr,
                                  mask_image_t& output,
                                  const unsigned spec_option = 0,
                                  const unsigned command_id = 0,
                                  const double parameter1 = 0.0,
                                  const double parameter2 = 0.0,
                                  const double parameter3 = 0.0 );

  // Helper function for the two above functions to prevent code duplication
  void classify_region_helper( const feature_array_t& features,
                               const image_region& region,
                               const pixel_classifier_t& clfr,
                               mask_image_t& output,
                               const unsigned spec_option = 0,
                               const unsigned command_id = 0,
                               const double parameter1 = 0.0,
                               const double parameter2 = 0.0,
                               const double parameter3 = 0.0 );

  // Reclassify a region, and determine which of two classifiers fits it better
  void xhair_selection( const feature_array_t& features,
                        const image_region& region,
                        const pixel_classifier_t& clfr1,
                        const pixel_classifier_t& clfr2,
                        mask_image_t& output );

  // Check if we think some blinking component specified by the template is active
  bool check_blinker_component( const image_region& region,
                                const double per_thresh,
                                const mask_image_t& output );

  // Only apply extra dilation/erosion to a specific part of the output mask
  void apply_morph_to_region( const image_region& region,
                              const double kernel_size,
                              const bool dilate,
                              mask_image_t& output );
};


} // end namespace vidtk


#endif // vidtk_osd_mask_refiner_h_
