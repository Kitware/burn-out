/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _vidtk_motion_and_saliency_detector_pipeline_h_
#define _vidtk_motion_and_saliency_detector_pipeline_h_

#include <object_detectors/detector_implementation_base.h>

#include <object_detectors/diff_super_process.h>
#include <object_detectors/conn_comp_super_process.h>
#include <tracking_data/image_object.h>
#include <utilities/merge_n_vectors_process.h>


// Instantiate class definition for vector merge process
INSTANTIATE_MERGE_N_VECTOR( 2, vidtk::image_object_sptr )

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Motion detector base class.
 *
 * This class creates a motion detector pipeline.
 */
template< class PixType >
class motion_and_saliency_detector_pipeline
  : public detector_implementation_base< PixType >
{
public:

  motion_and_saliency_detector_pipeline( std::string const& name );
  virtual ~motion_and_saliency_detector_pipeline();

  virtual config_block params();
  virtual bool set_params( config_block const& blk );

private:
  template< class Pipeline >
  void setup_pipeline( Pipeline * p );

  typedef merge_2_vectors_process< image_object_sptr > merge_image_object_process;

  process_smart_pointer< diff_super_process< PixType > > proc_diff_sp;
  process_smart_pointer< conn_comp_super_process< PixType > > proc_conn_comp1_sp;
  process_smart_pointer< conn_comp_super_process< PixType > > proc_conn_comp2_sp;
  process_smart_pointer< merge_image_object_process > proc_merge;

  process_smart_pointer< set_object_type_process > proc_set_obj_type_1;
  process_smart_pointer< set_object_type_process > proc_set_obj_type_2;
  process_smart_pointer< set_object_type_process > proc_set_proj_obj_type;

  boost::shared_ptr< set_type_functor > m_obj_type_1;
  boost::shared_ptr< set_type_functor > m_obj_type_2;
  boost::shared_ptr< set_type_functor > m_proj_obj_type;

  bool m_run_async;
  bool m_gui_feedback_enabled;
  double m_max_workable_gsd;
};

} // namespace vidtk

#endif // _vidtk_motion_and_saliency_detector_pipeline_h_
