/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_shot_detection_process_h_
#define vidtk_shot_detection_process_h_

#include <kwklt/klt_track.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <utilities/homography.h>

namespace vidtk
{

/// \brief A simple class to detect shot end boundary. 
///
/// This is the first implementation of the shot end boundary detection. 
/// Currently this works off of two criteria: 
/// 1. The percentage of KLT points lost on a frame.
/// 2. Whether the last homography is singular or not. 

class shot_detection_impl;

class shot_detection_process
  : public process
{
public:
  typedef shot_detection_process self_type;

  shot_detection_process( vcl_string const& name );

  ~shot_detection_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();

  bool is_end_of_shot(){ return is_end_of_shot_; };
  VIDTK_OUTPUT_PORT( bool, is_end_of_shot );

  void set_src_to_ref_homography( vgl_h_matrix_2d<double> const& );
  VIDTK_INPUT_PORT( set_src_to_ref_homography, vgl_h_matrix_2d<double> const&);
  void set_src_to_ref_vidtk_homography( image_to_image_homography const& );
  VIDTK_INPUT_PORT( set_src_to_ref_vidtk_homography, image_to_image_homography const&);

  void set_active_tracks( vcl_vector< klt_track_ptr > const& trks );
  VIDTK_INPUT_PORT( set_active_tracks, vcl_vector< klt_track_ptr > const& );

  void set_terminated_tracks( vcl_vector< klt_track_ptr > const& trks );
  VIDTK_INPUT_PORT( set_terminated_tracks, vcl_vector< klt_track_ptr > const& );

private:
  shot_detection_impl * impl_; 

  /// \brief Control signal identifying that the end of shot has been detected.
  ///
  /// End of shot is based on break of image stabilization chain. This process
  /// would return failure in the step function after setting this flag to true.
  bool is_end_of_shot_;
};

} // end namespace vidtk

#endif // vidtk_shot_detection_process_h_
