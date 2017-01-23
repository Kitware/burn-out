/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_cnn_detector_process_h_
#define vidtk_cnn_detector_process_h_

// VXL includes
#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

// VIDTK includes
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

// Detector includes
#include <object_detectors/cnn_detector.h>

// Boost includes
#include <boost/scoped_ptr.hpp>

namespace vidtk
{

template< class PixType >
class cnn_detector_process
  : public process
{

public:
  typedef cnn_detector_process self_type;

  cnn_detector_process( std::string const& name );
  virtual ~cnn_detector_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  void set_source_image( vil_image_view< PixType > const& image );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );

  void set_source_mask( vil_image_view< bool > const& mask );
  VIDTK_INPUT_PORT( set_source_mask, vil_image_view< bool > const& );

  void set_source_scale( double scale );
  VIDTK_INPUT_PORT( set_source_scale, double );

  std::vector< image_object_sptr > detections() const;
  VIDTK_OUTPUT_PORT( std::vector< image_object_sptr >, detections );

  vil_image_view< bool > fg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool >, fg_image );

  vil_image_view< float > heatmap_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< float >, heatmap_image );

private:

  // Is this process disabled?
  bool disabled_;

  // Inputs
  vil_image_view< PixType > image_;
  vil_image_view< bool > mask_;
  double scale_;

  // Outputs
  std::vector< image_object_sptr > detections_;
  vil_image_view< bool > fg_;
  vil_image_view< float > heatmap_;

  // Core detection algorithm
  boost::scoped_ptr< cnn_detector< PixType > > detector_;

  // Internal parameters/settings
  config_block config_;
  unsigned burst_frame_count_;
  unsigned burst_skip_count_;
  bool burst_skip_mode_;
  unsigned burst_counter_;
  unsigned initial_skip_count_;
  unsigned initial_no_output_count_;
  unsigned processed_counter_;
  double target_scale_;
  double min_scale_factor_;
  double max_scale_factor_;
  double max_operate_scale_;
  unsigned border_pixel_count_;
};

} // end namespace vidtk


#endif // vidtk_cnn_detector_process_h_
