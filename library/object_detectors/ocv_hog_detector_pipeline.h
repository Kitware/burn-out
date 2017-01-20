/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_ocv_hog_detector_pipeline_h_
#define vidtk_ocv_hog_detector_pipeline_h_

#include <object_detectors/detector_implementation_base.h>

#include <object_detectors/ocv_hog_detector_process.h>
#include <object_detectors/detector_pass_thru_process.h>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Motion detector base class.
 *
 * This class creates a motion detector pipeline.
 */
template< class PixType >
class ocv_hog_detector_pipeline
  : public detector_implementation_base< PixType >
{
public:

  ocv_hog_detector_pipeline( std::string const& name );
  virtual ~ocv_hog_detector_pipeline();

  virtual config_block params();
  virtual bool set_params( config_block const& blk );

private:
  template< class Pipeline >
  void setup_pipeline( Pipeline * p );

  process_smart_pointer< ocv_hog_detector_process > proc_dpm;
  process_smart_pointer< detector_pass_thru_process< PixType > > proc_pass;
  process_smart_pointer< set_object_type_process > proc_set_obj_type;

  boost::shared_ptr< set_type_functor > m_obj_type;

  // operating flags extracted from the config.
  bool m_run_async;
};

} // namespace vidtk

#endif // _vidtk_ocv_hog_detector_pipeline_h_
