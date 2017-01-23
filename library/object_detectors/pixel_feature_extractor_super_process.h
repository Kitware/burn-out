/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_pixel_feature_extractor_super_process_h_
#define vidtk_pixel_feature_extractor_super_process_h_

#include <pipeline_framework/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <video_properties/border_detection_process.h>

#include <utilities/timestamp.h>
#include <utilities/video_metadata.h>
#include <utilities/homography.h>

#include <vector>

namespace vidtk
{

// Internal implementation of the super process
template< class InputType, class OutputType >
class pixel_feature_extractor_super_process_impl;


/// A process that, when given an input image, generates a lot of features
/// for each individual pixel in the image describing local image structure,
/// temporal variance, and other features. The output is a vector array
/// of images of type OutputType, with each image corresponding to some feature.
template< class InputType, class OutputType >
class pixel_feature_extractor_super_process
  : public super_process
{

public:

  typedef pixel_feature_extractor_super_process self_type;
  typedef std::vector< vil_image_view< OutputType > > feature_array_t;

  pixel_feature_extractor_super_process( std::string const& name );
  virtual ~pixel_feature_extractor_super_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  void set_source_color_image( vil_image_view< InputType > const& );
  VIDTK_INPUT_PORT( set_source_color_image, vil_image_view< InputType > const& );

  void set_source_grey_image( vil_image_view< InputType > const& );
  VIDTK_INPUT_PORT( set_source_grey_image, vil_image_view< InputType > const& );

  void set_source_gsd( double );
  VIDTK_INPUT_PORT( set_source_gsd, double );

  void set_source_timestamp( timestamp const& );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_border( image_border const& );
  VIDTK_INPUT_PORT( set_border, image_border const& );

  void set_auxiliary_features( feature_array_t const& );
  VIDTK_INPUT_PORT( set_auxiliary_features, feature_array_t const& );

  feature_array_t feature_array() const;
  VIDTK_OUTPUT_PORT( feature_array_t, feature_array );

  vil_image_view< double > var_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< double >, var_image );

private:

  pixel_feature_extractor_super_process_impl<InputType, OutputType> * impl_;

};

} // end namespace vidtk

#endif // vidtk_pixel_feature_extractor_super_process_h_
