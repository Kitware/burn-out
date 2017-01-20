/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_groundcam_salient_region_classifier_h_
#define vidtk_groundcam_salient_region_classifier_h_

#include <object_detectors/salient_region_classifier.h>

#include <classifier/hashed_image_classifier.h>
#include <video_properties/eo_ir_detector.h>

namespace vidtk
{

/// Options for the groundcam_salient_region_classifier class.
struct groundcam_classifier_settings
{
  /// Should we use an extra pixel classifier on top of all salient regions?
  bool use_pixel_classifier_;

  /// Filename of the pixel-level classifier if enabled
  std::string pixel_classifier_filename_;

  /// Pixel classifier threshold
  double pixel_classifier_threshold_;

  /// Threshold specifying the required percent (range 0..1) of obstruction
  /// pixels marked in the input fg image for the frame to be considered invalid.
  double bad_frame_obstruction_threshold_;

  /// Threshold specifying the required percent (range 0..1) of salient
  /// pixels marked in the input fg image for the frame to be considered invalid.
  double bad_frame_saliency_threshold_;

  /// Threshold specifying the required percent (range 0..1) of salient + obstruction
  /// pixels marked in the input fg image for the frame to be considered invalid.
  double bad_frame_all_fg_threshold_;

  // Defaults
  groundcam_classifier_settings()
  : use_pixel_classifier_( false ),
    pixel_classifier_filename_( "" ),
    pixel_classifier_threshold_( 0.0 ),
    bad_frame_obstruction_threshold_( 0.40 ),
    bad_frame_saliency_threshold_( 0.40 ),
    bad_frame_all_fg_threshold_( 0.50 )
  {}
};


/// Decides what part of some type of imagery is foreground by combining simple
/// object recognition or other techniques around initial salient regions specified
/// externally.
template< typename PixType = vxl_byte, typename FeatureType = vxl_byte >
class groundcam_salient_region_classifier
  : public salient_region_classifier< PixType, FeatureType >
{

public:

  typedef double weight_t;
  typedef float output_t;
  typedef std::vector< vil_image_view<FeatureType> > feature_array_t;
  typedef vil_image_view< PixType > input_image_t;
  typedef vil_image_view< weight_t > weight_image_t;
  typedef vil_image_view< output_t > output_image_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef image_border image_border_t;

  groundcam_salient_region_classifier( const groundcam_classifier_settings& params );
  virtual ~groundcam_salient_region_classifier() {}

  /// \brief Process a new frame, returning an output mask and weight image
  /// representing (at a pixel level) which regions are "interesting" and
  /// should be submitted for tracking or higher-level object recognizers.
  ///
  /// This function describes how to combine an initial saliency approximation,
  /// a scene obstruction mask, and pixel-level features into a single output
  /// mask indicating which regions likely belong to some category we are
  /// interested in.
  ///
  /// \param image Source RGB image
  /// \param saliency_map A floating point initial saliency weight image
  /// \param saliency_mask An initial binary saliency mask
  /// \param features Vector of pixel level features previously computed
  /// \param output_weights The output saliency weight map
  /// \param output_mask The output salient pixels map
  /// \param border Precomputed border location
  /// \param obstruction_mask An optional mask detailing potential scene obstructions
  virtual void process_frame( const input_image_t& image,
                              const weight_image_t& saliency_map,
                              const mask_image_t& saliency_mask,
                              const feature_array_t& features,
                              output_image_t& output_weights,
                              mask_image_t& output_mask,
                              const image_border_t& border = image_border_t(),
                              const mask_image_t& obstruction_mask = mask_image_t() );

private:

  // Externally set options
  groundcam_classifier_settings settings_;

  // Internal buffers, counters, and classifiers
  hashed_image_classifier< FeatureType > ooi_classifier_;
  vil_image_view< double > ooi_classification_image_;
  unsigned frame_counter_;
  unsigned frames_since_last_break_;

};


} // end namespace vidtk


#endif // vidtk_groundcam_salient_region_classifier_h_
