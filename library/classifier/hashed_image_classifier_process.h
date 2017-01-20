/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_hashed_image_feature_classifier_process_h_
#define vidtk_hashed_image_feature_classifier_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/config_block.h>
#include <utilities/video_modality.h>

#include <classifier/hashed_image_classifier.h>

#include <vil/vil_image_view.h>

#include <vector>

namespace vidtk
{

/// Classify every pixel in an image according to a hashed_image_feature
/// classifier. Can also optionally select different classifiers based on
/// modality and/or GSD range.
template< typename HashType = vxl_byte, typename OutputType = double >
class hashed_image_classifier_process
  : public process
{
public:

  typedef hashed_image_classifier_process self_type;
  typedef std::vector< vil_image_view< HashType > > input_features;
  typedef hashed_image_classifier< HashType, OutputType > classifier;

  hashed_image_classifier_process( std::string const& _name );

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// Set input vector of per-pixel features
  void set_pixel_features( input_features const& features );
  VIDTK_INPUT_PORT( set_pixel_features, input_features const& );

  /// Set input gsd, if used
  void set_gsd( double gsd );
  VIDTK_INPUT_PORT( set_gsd, double );

  /// Set input modality, if used
  void set_modality( vidtk::video_modality vm );
  VIDTK_INPUT_PORT( set_modality, vidtk::video_modality );

  /// Classified output image
  vil_image_view<OutputType> classified_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<OutputType>, classified_image );

private:

  // Are we using different classifiers for GSD/modalities?
  bool use_variable_models_;

  // Internal parameter block
  config_block config_;

  // Other parameters
  double lower_gsd_threshold_;
  double upper_gsd_threshold_;

  // Default model
  classifier default_clfr_;

  // Specialized models, if used (narrow, medium, high)
  classifier eo_n_clfr_;
  classifier eo_m_clfr_;
  classifier eo_w_clfr_;
  classifier ir_n_clfr_;
  classifier ir_m_clfr_;
  classifier ir_w_clfr_;

  // Inputs
  const input_features* input_features_;
  double input_gsd_;
  vidtk::video_modality input_modality_;

  // Algorithm outputs
  vil_image_view<OutputType> output_img_;

  // Helper functions
  void reset_inputs();
};


} // end namespace vidtk


#endif // vidtk_hashed_image_classifier_process_h_
