/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef _SHOT_BREAK_MONITOR_H_
#define _SHOT_BREAK_MONITOR_H_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>

#include <tracking/shot_break_flags.h>

#include <video/eo_ir_detector.h>

#include <vil/vil_image_view.h>


namespace vidtk
{


// ----------------------------------------------------------------
/** Video modality monitor.
 *
 * This class monitors the input video modality value. If the new
 * modality value is different from the previous value, this process
 * fails causing a pipeline reset. Those applications that need this
 * can disable this man with a gun.
 *
 * If this process is not enabled, then all inputs just pass through.
 */
template< class PixType >
class shot_break_monitor
  : public process
{
public:
  typedef shot_break_monitor self_type;

  shot_break_monitor( vcl_string const& name );
  virtual ~shot_break_monitor();

  // -- process interface --
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();
  virtual bool reset();

  // status accessors
  bool is_end_of_shot();
  bool is_modality_change();
  vidtk::video_modality current_modality() const;


#define VMM_PROCESS_PORTS                                               \
  CONNECTION_DEF( vidtk::timestamp,                 timestamp )         \
  CONNECTION_DEF( vidtk::timestamp::vector_t,       timestamp_vector )  \
  CONNECTION_DEF( vil_image_view< PixType >,        image )             \
  CONNECTION_DEF( vidtk::image_to_image_homography, src_to_ref_homography ) \
  CONNECTION_DEF( vidtk::plane_to_image_homography, wld_to_src_homography ) \
  CONNECTION_DEF( vidtk::plane_to_utm_homography,   wld_to_utm_homography ) \
  CONNECTION_DEF( vidtk::image_to_plane_homography, src_to_wld_homography ) \
  CONNECTION_DEF( vidtk::image_to_plane_homography, ref_to_wld_homography ) \
  CONNECTION_DEF( vil_image_view< bool >,           mask )                  \
  CONNECTION_DEF( double,                           world_units_per_pixel ) \
  CONNECTION_DEF( vidtk::shot_break_flags,          shot_break_flags )


// Define macro to generate input method, input port, output port,
// output method, and data area.  The ports defined in this way
// transfer data into and out of this process. The
// synchronization must be preserved.

// Note: it is possible for the process to inspect these values.

#define CONNECTION_DEF(TYPE, BASENAME)                                  \
  private:                                                              \
  TYPE BASENAME ## _data;                                               \
  TYPE BASENAME ## _data_save;                                          \
public:                                                                 \
void set_input_ ## BASENAME( TYPE const& v)                             \
  {                                                                     \
    BASENAME ## _data = v;                                              \
  }                                                                     \
VIDTK_INPUT_PORT( set_input_ ## BASENAME, TYPE const& );                \
TYPE get_output_ ## BASENAME() const                                    \
  {                                                                     \
    return BASENAME ## _data;                                           \
  }                                                                     \
VIDTK_OUTPUT_PORT( TYPE, get_output_ ## BASENAME );                     \
VIDTK_ALLOW_SEMICOLON_AT_END( BASENAME );

  VMM_PROCESS_PORTS  // Expand for all process ports

#undef CONNECTION_DEF // cleanup namespace

  video_modality get_output_video_modality() const { return out_video_modality_; }
  VIDTK_OUTPUT_PORT( video_modality, get_output_video_modality );


private:
  void save_inputs();
  void swap_inputs();
  void reset_save_area();
  video_modality compute_modality( double new_gsd );

  vidtk::video_modality last_modality_;

  // Configuration parameters
  config_block config_;

  bool disabled_;  // set if processing is disabled.
  double fov_change_threshold_;

  // These are our findings for shot break status
  bool end_of_shot_;
  bool modality_change_;
  vidtk::video_modality out_video_modality_;

  bool first_frame_only_;

  eo_ir_detector < PixType > eo_ir_oracle; // the algorithm


}; // end class shot_break_monitor

} // end namespace

#endif /* _SHOT_BREAK_MONITOR_H_ */

