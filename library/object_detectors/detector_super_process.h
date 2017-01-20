/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#if !defined (vidtk_detector_super_process_h_)
#define vidtk_detector_super_process_h_

#include <object_detectors/detector_super_process_port_def.h>
#include <object_detectors/detector_implementation_base.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>

#include <tracking_data/image_object.h>
#include <tracking_data/shot_break_flags.h>
#include <tracking_data/gui_frame_info.h>

#include <process_framework/pipeline_aid.h>
#include <pipeline_framework/super_process.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>

#include <boost/scoped_ptr.hpp>


namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Interface for detector super process.
 *
 * Detector processes looks at images and generate a list of
 * image_objects that represent detections. The actual implementation
 * determines what constitutes a detection. It may be a motion
 * signature, or it may be something else.
 *
 * This process supports reconfigure and reset as done by DTSP on
 * video mode breaks.
 *
\msc
--- [ label = "video mode change" ];
  DTSP=>pipeline [ label = "set_params_downstream()" ];
  pipeline=>detect_sp [ label = "set_params()" ];

  DTSP=>pipeline [ label = "reset_downstream()" ];
  pipeline=>detect_sp [ label = "reset()" ];

\endmsc
 */
template < class PixType >
class detector_super_process
  : public vidtk::super_process
{
public:
  typedef detector_super_process < PixType > self_type;

  detector_super_process( std::string const& name,
          typename vidtk::detector_implementation_base< PixType >::sptr impl_p);
  virtual  ~detector_super_process();

  // process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual process::step_status step2();


  // Generate input ports
#define DEFINE_INPUTS(T,N)                      \
  void input_ ## N ( T const& val );            \
  VIDTK_INPUT_PORT( input_ ## N, T const& );

  DETECTOR_SUPER_PROCESS_INPUTS( DEFINE_INPUTS )

#undef DEFINE_INPUTS


  // Generate output ports
#define DEFINE_OUTPUTS(T,N)                     \
  T output_ ## N () const;                      \
  VIDTK_OUTPUT_PORT( T, output_ ## N );

  DETECTOR_SUPER_PROCESS_OUTPUTS( DEFINE_OUTPUTS )

#undef DEFINE_OUTPUTS

protected:

  /// Pointer to pipeline implementation
  typename detector_implementation_base< PixType >::sptr m_impl_ptr;

};

// Undefine these macros so we don't pollute the name space.
// If they are needed anywhere else, just use include to get definition.
#undef DETECTOR_SUPER_PROCESS_INPUTS
#undef DETECTOR_SUPER_PROCESS_OUTPUTS

} // namespace vidtk

#endif // vidtk_detector_super_process_h_
