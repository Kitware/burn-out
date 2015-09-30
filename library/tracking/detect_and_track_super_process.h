/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_detech_and_track_super_process_h_
#define vidtk_detech_and_track_super_process_h_

#include <pipeline/super_process.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/timestamp.h>
#include <utilities/video_modality.h>
#include <utilities/homography.h>
#include <tracking/gui_process.h>
#include <tracking/track.h>
#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <vul/vul_timer.h>

namespace vidtk
{

// Partial types
template< class PixType> class detect_and_track_super_process_impl;
class shot_break_flags;



// ----------------------------------------------------------------
/** detect and track super process.
 *
 * This superprocess encapsulates the main tracking processes. The
 * image is assumed to be already stabilized, and all homographies are
 * calcualted.
 *
 * The data writer process can be used to capture the output of this
 * super process to a file. The writer is disabled by default, but can
 * be included in the DTSP pipeline by adding the following line to
 * the config file. See the writer process config for more file
 * related options.
 *
 * tagged_data_writer:enabled = true
 *
 * This is useful for enabling or disabling the whole writer. Each
 * output must be enabled individually to cause that datum to be
 * include in the file produced. The following is a list of all
 * available data items that can be enabled for writing. These items
 * are disabled by default so they must be enabled to get results. The
 * sample config line shown below will enable the datum.
 *
 * tagged_data_writer:time_stamp = true
 * tagged_data_writer:src_to_ref_homography = true
 * tagged_data_writer:src_to_utm_homography = true
 * tagged_data_writer:ref_to_wld_homography = true
 * tagged_data_writer:world_units_per_pixel = true
 * tagged_data_writer:shot_break_flags = true
 *
 * Currently tracks are not handled.
 */
template< class PixType >
class detect_and_track_super_process
  : public super_process
{
public:
  typedef detect_and_track_super_process self_type;

  detect_and_track_super_process( vcl_string const& name );
  virtual ~detect_and_track_super_process();

  // --- Process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  /// This function is called when the pipeline detects failure in the sp.
  /// It gives the sp a last chance to recover from the failure before
  /// propigating the failure out the the external pipeline.
  virtual bool fail_recover();

  void set_multi_features_dir( vcl_string const & dir );

  /// \brief Provide vidtk::gui_process object(s) to be used.
  ///
  /// Supply full_tracking and back_tracking gui's. Only one will
  /// work at a time. You can specify in the config file which gui
  /// to use for the particular instance of the program. If both
  /// are turned on through the config by mistake, then full_tracking
  /// gui will take precedence and back_tracking gui is forced to
  /// be disabled.
  void set_guis( process_smart_pointer< gui_process > ft_gui,
                 process_smart_pointer< gui_process > bt_gui
                  = process_smart_pointer< gui_process >(
                      new gui_process( "bt_gui" ) ) );


  //    ( <type>, <name> )
#define DTSP_PROCESS_INPUTS                                             \
  CONNECTION_DEF( vidtk::timestamp,                 timestamp )         \
  CONNECTION_DEF( vidtk::timestamp::vector_t,       timestamp_vector )  \
  CONNECTION_DEF( vil_image_view< PixType >,        image )             \
  CONNECTION_DEF( vidtk::image_to_image_homography, src_to_ref_homography ) \
  CONNECTION_DEF( vidtk::plane_to_image_homography, wld_to_src_homography ) \
  CONNECTION_DEF( vidtk::plane_to_utm_homography,   wld_to_utm_homography ) \
  CONNECTION_DEF( vidtk::image_to_plane_homography, src_to_wld_homography ) \
  CONNECTION_DEF( vidtk::image_to_plane_homography, ref_to_wld_homography ) \
  CONNECTION_DEF( double,                           world_units_per_pixel ) \
  CONNECTION_DEF( vil_image_view< bool >,           mask ) \
  CONNECTION_DEF( vidtk::shot_break_flags,          shot_break_flags )

  //    ( <type>, <name> )
#define DTSP_PROCESS_OUTPUTS                                            \
  CONNECTION_DEF( vidtk::timestamp,                 timestamp )         \
  CONNECTION_DEF( vidtk::timestamp::vector_t,       timestamp_vector )  \
  CONNECTION_DEF( vcl_vector< track_sptr >,         active_tracks )     \
  CONNECTION_DEF( vidtk::image_to_image_homography, src_to_ref_homography ) \
  CONNECTION_DEF( vidtk::image_to_utm_homography,   src_to_utm_homography ) \
  CONNECTION_DEF( vidtk::image_to_plane_homography, ref_to_wld_homography ) \
  CONNECTION_DEF( double,                           world_units_per_pixel ) \
  CONNECTION_DEF( vidtk::video_modality,            video_modality )    \
  CONNECTION_DEF( vidtk::shot_break_flags,          shot_break_flags )   \
  CONNECTION_DEF( vcl_vector< track_sptr >,         linked_tracks )     \
  CONNECTION_DEF( vcl_vector< track_sptr >,         filtered_tracks )

  // Process I/O ports
  // ---- Inputs ----
#define CONNECTION_DEF(T,N)                             \
  void set_input_ ## N( T const& value );               \
  VIDTK_INPUT_PORT( set_input_ ## N, T const& );

  DTSP_PROCESS_INPUTS // expand over all input ports

#undef CONNECTION_DEF

  // ---- Outputs ----
#define CONNECTION_DEF(T,N)                             \
  T const& get_output_ ## N() const;                    \
  VIDTK_OUTPUT_PORT( T const&, get_output_ ## N );

  DTSP_PROCESS_OUTPUTS // expand over all output ports

#undef CONNECTION_DEF

private:
  detect_and_track_super_process_impl< PixType > * m_impl;

  bool  videoname_prefixing_done_; 

};

} // namespace vidtk

#endif // vidtk_detech_and_track_super_process_h_
