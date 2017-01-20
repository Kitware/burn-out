/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_salient_region_classifier_process_h_
#define vidtk_salient_region_classifier_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <vector>

#include <tracking_data/image_object.h>

#include <video_properties/border_detection.h>

#include <object_detectors/salient_region_classifier.h>

#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>

namespace vidtk
{


/// Decides what part of some type of imagery is foreground by combining simple
/// object detection techniques around salient regions.
template< typename PixType = vxl_byte, typename FeatureType = vxl_byte >
class salient_region_classifier_process
  : public process
{

public:

  typedef salient_region_classifier_process self_type;
  typedef std::vector< vil_image_view<FeatureType> > feature_array;
  typedef salient_region_classifier< PixType, FeatureType > classifier_type;
  typedef std::vector< image_object_sptr > detection_vector;

  salient_region_classifier_process( std::string const& _name );
  virtual ~salient_region_classifier_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_input_image( vil_image_view< PixType > const& src );
  VIDTK_INPUT_PORT( set_input_image, vil_image_view< PixType > const& );

  void set_saliency_map( vil_image_view<double> const& src );
  VIDTK_INPUT_PORT( set_saliency_map, vil_image_view<double> const& );

  void set_saliency_mask( vil_image_view<bool> const& src );
  VIDTK_INPUT_PORT( set_saliency_mask, vil_image_view<bool> const& );

  void set_obstruction_mask( vil_image_view<bool> const& src );
  VIDTK_INPUT_PORT( set_obstruction_mask, vil_image_view<bool> const& );

  void set_border( image_border const& rect );
  VIDTK_INPUT_PORT( set_border, image_border const& );

  void set_input_features( feature_array const& array );
  VIDTK_INPUT_PORT( set_input_features, feature_array const& );

  vil_image_view<float> pixel_saliency() const;
  VIDTK_OUTPUT_PORT( vil_image_view<float>, pixel_saliency );

  vil_image_view<bool> foreground_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, foreground_image );

private:

  // Possible Inputs
  vil_image_view<PixType> input_image_; // Input image
  vil_image_view<double> saliency_map_; // Initial classification image
  vil_image_view<bool> saliency_mask_; // Reccommended initial saliency mask
  vil_image_view<bool> obstruction_mask_; // Reccommended initial obstruction mask
  image_border border_; // Estimated image border
  feature_array features_; // Pixel-level features

  // Possible Outputs
  vil_image_view<float> output_weights_;
  vil_image_view<bool> output_fg_;

  // The actual algorithm
  bool disabled_;
  config_block config_;
  boost::scoped_ptr< classifier_type > classifier_;
};


} // end namespace vidtk


#endif // vidtk_salient_region_classifier_process_h_
