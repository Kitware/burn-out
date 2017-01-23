/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_salient_region_classifier_h_
#define vidtk_salient_region_classifier_h_

#include <vector>

#include <vil/vil_image_view.h>

#include <video_properties/border_detection.h>

#include <tracking_data/image_object.h>

namespace vidtk
{

/// Abstract base class for deciding what part of some type of imagery is
/// foreground by combining simple object recognition (or other techniques)
/// around salient regions in any number of ways.
template< typename PixType = vxl_byte, typename FeatureType = vxl_byte >
class salient_region_classifier
{

public:

  typedef std::vector< vil_image_view<FeatureType> > feature_array_t;
  typedef vil_image_view< PixType > input_image_t;
  typedef vil_image_view< double > weight_image_t;
  typedef vil_image_view< float > output_image_t;
  typedef vil_image_view< bool > mask_image_t;
  typedef image_border image_border_t;

  salient_region_classifier() {}
  virtual ~salient_region_classifier() {}

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
  /// \param saliency_mask A binary initial saliency mask
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
                              const mask_image_t& obstruction_mask = mask_image_t() ) = 0;

};


} // end namespace vidtk


#endif // vidtk_salient_region_classifier_h_
