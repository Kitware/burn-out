/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_dummy_shot_detection_process_h_
#define vidtk_dummy_shot_detection_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

/// \brief A sample class to detection shot end boundary. 
///
/// This processes is generated for testing of the pipeline around it. 
/// It will be replaced an actual implementation that will do something 
/// meaningful to detection end shot boundary within a video. The definition
/// of the shot boundary is up to the process, but typically it implies 
/// break in stabilization. The assuming that detection and tracking algorithms
/// downstream won't be able to recover from this boundary unless, they were
/// notified by the processes like this one. 

template< class PixType >
class dummy_shot_detection_process
  : public process
{
public:
  typedef dummy_shot_detection_process self_type;

  dummy_shot_detection_process( vcl_string const& name );

  ~dummy_shot_detection_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();
  
  virtual bool reset();

  virtual bool step();

  bool is_end_of_shot(){ return is_end_of_shot_; };

  void set_src_to_ref_homography( vgl_h_matrix_2d<double> const& );
  VIDTK_INPUT_PORT( set_src_to_ref_homography, vgl_h_matrix_2d<double> const&);

private:
  // Passing the data along
  vgl_h_matrix_2d<double> const * H_src2ref_;

  /// \brief Control signal identifying that the end of shot has been detected.
  ///
  /// End of shot is based on break of image stabilization chain. This process
  /// would return failure in the step function after setting this flag to true.
  bool is_end_of_shot_;
};

} // end namespace vidtk

#endif // vidtk_dummy_shot_detection_process_h_
