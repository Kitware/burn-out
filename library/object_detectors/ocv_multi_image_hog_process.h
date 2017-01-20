/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ocv_multi_dpm_detector_process_h_
#define vidtk_ocv_multi_dpm_detector_process_h_

// VXL includes
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>
#include <vnl/vnl_double_3.h>

// VIDTK includes
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

// Descriptor includes
#include <object_detectors/ocv_hog_detector.h>

#include <string>
#include <vector>

namespace vidtk
{

class ocv_multi_image_hog_process
  : public process
{

public:

  typedef ocv_multi_image_hog_process self_type;

  ocv_multi_image_hog_process( std::string const& name );
  virtual ~ocv_multi_image_hog_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_imagery( std::vector<vil_image_view<vxl_byte> > const& );
  VIDTK_INPUT_PORT( set_source_imagery, std::vector<vil_image_view<vxl_byte> > const& );

  void set_offset_scales( std::vector<vnl_double_3> const&);
  VIDTK_INPUT_PORT( set_offset_scales, std::vector<vnl_double_3> const& );

  std::vector<image_object_sptr> image_objects() const;
  VIDTK_OUTPUT_PORT( std::vector<image_object_sptr>, image_objects );

private:

  // Is this process disabled?
  bool disabled_;

  std::vector<vil_image_view<vxl_byte> > images_;
  std::vector<vnl_double_3> offset_scales_;
  std::vector<image_object_sptr> image_objects_;
  ocv_hog_detector detector_;

  // Internal parameters/settings
  config_block config_;

};

} // end namespace vidtk


#endif // vidtk_ocv_multi_dpm_detector_process_h_
