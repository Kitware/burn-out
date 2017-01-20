/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_ocv_hog_detector_process_h_
#define vidtk_ocv_hog_detector_process_h_

// VXL includes
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

// VIDTK includes
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

// Detector includes
#include <object_detectors/ocv_hog_detector.h>

namespace vidtk
{

class ocv_hog_detector_process
  : public process
{

public:
  typedef ocv_hog_detector_process self_type;

  ocv_hog_detector_process( std::string const& name );
  virtual ~ocv_hog_detector_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( vil_image_view< vxl_byte > const& image );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< vxl_byte > const& );

  void set_source_scale( double scale );
  VIDTK_INPUT_PORT( set_source_scale, double );

  std::vector< image_object_sptr > image_objects() const;
  VIDTK_OUTPUT_PORT( std::vector< image_object_sptr >, image_objects );

private:

  // Is this process disabled?
  bool disabled_;

  // Inputs and outputs
  vil_image_view< vxl_byte > image_;
  double scale_;
  std::vector< image_object_sptr > image_objects_;
  ocv_hog_detector detector_;

  // Internal parameters/settings
  config_block config_;
  unsigned burst_frame_count_;
  unsigned skip_frame_count_;
  bool skip_mode_;
  unsigned frame_counter_;
  double target_scale_;
  double min_scale_factor_;
  double max_scale_factor_;
};

} // end namespace vidtk


#endif // vidtk_ocv_hog_detector_process_h_
