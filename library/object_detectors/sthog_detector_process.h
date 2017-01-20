/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_sthog_detector_process_h_
#define vidtk_sthog_detector_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <object_detectors/sthog_detector.h>

#include <tracking_data/image_object.h>

#include <vnl/vnl_int_2.h>
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <vector>

namespace vidtk
{

template <class PixType>
class sthog_detector_process
  : public process
{
public:
  typedef sthog_detector_process self_type;

  sthog_detector_process( std::string const& name );
  virtual ~sthog_detector_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  /// \brief The input image.
  void set_source_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// \brief The detection boxes.
  std::vector< image_object_sptr > detect_object() const;
  VIDTK_OUTPUT_PORT( std::vector< image_object_sptr >, detect_object );

private:

  void clear_outputs();
  void clear_inputs();

  // Parameters

  bool disabled_;

  std::vector< std::string > model_filename_vec_;

  double det_threshold_;
  int group_threshold_;
  int det_levels_;
  float resize_scale_;
  vnl_int_2 det_tile_size_;
  int det_tile_margin_;

  std::string output_file_;
  std::string output_image_pattern_;

  int sthog_frames_;
  vnl_int_2 hog_window_size_;
  vnl_int_2 hog_stride_step_;
  vnl_int_2 hog_cell_size_;
  vnl_int_2 hog_block_size_;
  vnl_int_2 hog_orientations_;

  // Input
  vil_image_view<PixType> input_image_;

  // Output
  std::vector< image_object_sptr > detection_boxes_;

  sthog_detector::write_image_t write_image_mode_;
};


} // end namespace vidtk


#endif // vidtk_sthog_detector_process_h_
