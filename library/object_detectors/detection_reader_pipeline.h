/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _vidtk_detection_reader_pipeline_h_
#define _vidtk_detection_reader_pipeline_h_

#include <object_detectors/detector_implementation_base.h>
#include <object_detectors/detector_pass_thru_process.h>

#include <tracking_data/io/image_object_reader_process.h>

namespace vidtk
{

// ----------------------------------------------------------------
/**
 * @brief Read object detections from a stream.
 *
 * This class is a detector_sp implementation which generates
 * detections from data read from a file.
 */
template< class PixType >
class detection_reader_pipeline
  : public detector_implementation_base< PixType >
{
public:
  detection_reader_pipeline( std::string const& name );
  virtual ~detection_reader_pipeline();

  virtual config_block params();
  virtual bool set_params(config_block const& blk);

private:
  template< class Pipeline >
  void setup_pipeline( Pipeline * p );

  process_smart_pointer< image_object_reader_process > proc_obj_reader;
  process_smart_pointer< detector_pass_thru_process< PixType > > proc_pass;

}; // end class detection_reader_pipeline

} // end namespace

#endif // _vidtk_detection_reader_pipeline_h_
