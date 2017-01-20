/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_scene_obstruction_detector_process_h_
#define vidtk_scene_obstruction_detector_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <vector>

#include <object_detectors/scene_obstruction_detector.h>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

/// The scene_obstruction_detector_process takes in a series of input features,
/// as produced by the pixel_feature_extractor_super_process, in addition
/// to the input (source image). From these features, it uses a hashed_image
/// classifier to come up with an initial estimate of locations of potential
/// scene obstructions. This initial approximation is then averaged between
/// successive frames. If a signifant change in the initial approximation is
/// detected, a "mask shot break" is triggered which resets the internal average.
///
/// As output, it produces an output image containing 2 planes, 1 detailing
/// the classification values for the running average, and one for the intial
/// approximation. This process can be configured to try and detect a few
/// effects, such as HUDs, dust, or other obstructors.
///
/// The process also has several optional sub-systems including adaptive
/// weighting, performing a 2 level classifier, and outputting training
/// data for the creation of different classifiers.
template <typename PixType>
class scene_obstruction_detector_process
  : public process
{

public:

  typedef scene_obstruction_detector_process self_type;
  typedef vxl_byte feature_type;
  typedef std::vector< vil_image_view<feature_type> > feature_array;
  typedef vil_image_view<bool> mask_type;
  typedef scene_obstruction_properties<PixType> mask_properties_type;
  typedef scene_obstruction_detector<PixType,vxl_byte> detector_type;

  scene_obstruction_detector_process( std::string const& _name );
  virtual ~scene_obstruction_detector_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  void set_color_image( vil_image_view<PixType> const& src );
  VIDTK_INPUT_PORT( set_color_image, vil_image_view<PixType> const& );

  void set_variance_image( vil_image_view<double> const& src );
  VIDTK_INPUT_PORT( set_variance_image, vil_image_view<double> const& );

  void set_border( vgl_box_2d<int> const& rect );
  VIDTK_INPUT_PORT( set_border, vgl_box_2d<int> const& );

  void set_input_features( feature_array const& array );
  VIDTK_INPUT_PORT( set_input_features, feature_array const& );

  vil_image_view<double> classified_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<double>, classified_image );

  mask_properties_type mask_properties() const;
  VIDTK_OUTPUT_PORT( mask_properties_type, mask_properties );

private:

  // Possible Inputs
  vil_image_view<double> var_; // Short term per pixel variance
  vil_image_view<PixType> rgb_; // RGB input image
  vgl_box_2d<int> border_; // Detected image border
  feature_array features_; // Calculated per-pixel features

  // Possible Outputs
  vil_image_view<double> output_image_; // Classified image
  mask_properties_type output_properties_;

  // Core execution parameters
  config_block config_;
  scene_obstruction_detector_settings options_;
  detector_type detector_;

  // Internal buffers for adding a lag if enabled
  std::vector< vil_image_view<double> > image_buffer_;
  std::vector< mask_properties_type > property_buffer_;
  unsigned reset_buffer_length_;
  unsigned frames_since_reset_;

  // Helper functions
  bool validate_inputs();
  void reset_inputs();
  void flush_buffer();
};


} // end namespace vidtk


#endif // vidtk_scene_obstruction_detector_process_h_
