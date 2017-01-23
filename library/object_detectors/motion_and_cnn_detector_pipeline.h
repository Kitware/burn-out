/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_motion_and_cnn_detector_pipeline_h_
#define vidtk_motion_and_cnn_detector_pipeline_h_

#include <object_detectors/detector_implementation_base.h>

#include <object_detectors/diff_super_process.h>
#include <object_detectors/conn_comp_super_process.h>
#include <object_detectors/cnn_detector_process.h>
#include <object_detectors/detection_consolidator_process.h>
#include <object_detectors/project_to_world_process.h>
#include <object_detectors/filter_image_objects_process.h>
#include <object_detectors/transform_image_object_process.h>

#include <utilities/extract_matrix_process.h>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Motion and HOG detector base class.
 *
 * This class creates a motion and HOG detector pipeline.
 */
template< class PixType >
class motion_and_cnn_detector_pipeline
  : public detector_implementation_base< PixType >
{
public:

  motion_and_cnn_detector_pipeline( std::string const& name );
  virtual ~motion_and_cnn_detector_pipeline();

  virtual config_block params();
  virtual bool set_params( config_block const& blk );

private:

  template< class Pipeline >
  void setup_pipeline( Pipeline * p );

  process_smart_pointer< diff_super_process< PixType > > proc_diff_sp;
  process_smart_pointer< conn_comp_super_process< PixType > > proc_conn_comp_sp;

  process_smart_pointer< cnn_detector_process< PixType > > proc_cnn1;
  process_smart_pointer< cnn_detector_process< PixType > > proc_cnn2;

  process_smart_pointer< detection_consolidator_process > proc_det_cons;

  process_smart_pointer< extract_matrix_process< image_to_plane_homography > > proc_matrix;
  process_smart_pointer< project_to_world_process > proc_project;
  process_smart_pointer< filter_image_objects_process< PixType > > proc_filter;
  process_smart_pointer< transform_image_object_process< PixType > > proc_add_chip;

  // operating flags extracted from the config.
  bool m_run_async;
  bool m_gui_feedback_enabled;
  double m_max_workable_gsd;
};

} // namespace vidtk

#endif // vidtk_motion_and_cnn_detector_pipeline_h_
