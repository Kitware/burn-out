/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_osd_mask_refinement_process_h_
#define vidtk_osd_mask_refinement_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <video_properties/border_detection_process.h>

#include <object_detectors/osd_template.h>
#include <object_detectors/osd_recognizer.h>
#include <object_detectors/osd_mask_refiner.h>

#include <classifier/hashed_image_classifier.h>

#include <vector>

namespace vidtk
{

/// \brief A process for formulating the final output mask for burnin detection
///  after the type of the OSD has been identified.
template <typename PixType>
class osd_mask_refinement_process
  : public process
{

public:

  typedef osd_mask_refinement_process self_type;
  typedef vxl_byte feature_type;
  typedef vil_image_view< bool > mask_image_type;
  typedef vil_image_view< feature_type > feature_image_type;
  typedef osd_recognizer_output< PixType, feature_type > osd_info_type;
  typedef boost::shared_ptr< osd_info_type > osd_info_sptr;
  typedef osd_mask_refiner< PixType, feature_type > refinement_algo_type;

  osd_mask_refinement_process( std::string const& _name );
  virtual ~osd_mask_refinement_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  void set_mask_image( mask_image_type const& mask );
  VIDTK_OPTIONAL_INPUT_PORT( set_mask_image, mask_image_type const& );

  void set_new_feature( feature_image_type const& mask );
  VIDTK_OPTIONAL_INPUT_PORT( set_new_feature, feature_image_type const& );

  void set_osd_info( osd_info_sptr input );
  VIDTK_OPTIONAL_INPUT_PORT( set_osd_info, osd_info_sptr );

  mask_image_type mask_image() const;
  VIDTK_OUTPUT_PORT( mask_image_type, mask_image );

  osd_info_sptr osd_info() const;
  VIDTK_OUTPUT_PORT( osd_info_sptr, osd_info );

private:

  // Primary Inputs
  mask_image_type input_mask_;
  osd_info_sptr input_osd_info_;

  // Secondary Inputs
  feature_image_type new_feature_;

  // Outputs
  mask_image_type output_mask_;
  osd_info_sptr output_osd_info_;

  // Internal parameters/settings
  config_block config_;
  bool disabled_;

  // Recognizer algorithm
  refinement_algo_type refiner_;
};


} // end namespace vidtk


#endif // vidtk_osd_mask_refinement_process_h_
