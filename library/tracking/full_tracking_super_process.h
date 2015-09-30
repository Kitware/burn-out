/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_full_tracking_super_process_h_
#define vidtk_full_tracking_super_process_h_

#include <pipeline/super_process.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/video_metadata.h>
#include <utilities/video_modality.h>
#include <utilities/homography.h>
#include <tracking/gui_process.h>
#include <tracking/shot_break_flags.h>
#include <tracking/track.h>
#include <vcl_vector.h>
#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vul/vul_timer.h>

namespace vidtk
{

// partial types
template< class PixType>
class full_tracking_super_process_impl;
class gui_process;

class timestamp;

// ----------------------------------------------------------------
/** Full tracking super process.
 *
 *
 */
template< class PixType>
class full_tracking_super_process
  : public super_process
{
public:
  typedef full_tracking_super_process self_type;

  full_tracking_super_process( vcl_string const& name );
  ~full_tracking_super_process();

  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();
  virtual void post_step();

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

  /** Set configuration values. This method accepts configuration
   * values that the FTSP will need.
   * @param[in] vidname - name of the video to process
   */
  void set_config_data( vcl_string const & vidname );

  // Process I/O ports
  // ---- Inputs ----
  void set_source_image( vil_image_view< PixType > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_image, vil_image_view< PixType > const&);

  void set_source_timestamp( timestamp const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_timestamp, timestamp const&  );

  void set_source_metadata( video_metadata const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_metadata, video_metadata const&  );


  // ---- Outputs ----
  timestamp const& finalized_timestamp() const;
  VIDTK_OUTPUT_PORT( timestamp const&, finalized_timestamp );

  vcl_vector< track_sptr > const& finalized_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, finalized_tracks );

  image_to_image_homography const& finalized_src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography const&, finalized_src_to_ref_homography );

  vidtk::image_to_utm_homography const& finalized_src_to_utm_homography() const;
  VIDTK_OUTPUT_PORT( vidtk::image_to_utm_homography const&, finalized_src_to_utm_homography );

  double get_output_world_units_per_pixel () const;
  VIDTK_OUTPUT_PORT( double, get_output_world_units_per_pixel );

  vidtk::video_modality get_output_video_modality() const;
  VIDTK_OUTPUT_PORT( vidtk::video_modality, get_output_video_modality);

  vidtk::shot_break_flags get_output_shot_break_flags ( ) const;
  VIDTK_OUTPUT_PORT( vidtk::shot_break_flags, get_output_shot_break_flags );

  vcl_vector< track_sptr > const& linked_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, linked_tracks );

  vcl_vector< track_sptr > const& filtered_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, filtered_tracks );


private:
  bool videoname_prefixing_done_;
  full_tracking_super_process_impl< PixType > * impl_;

  vul_timer wall_clock_;
};

} // namespace vidtk

#endif // vidtk_full_tracking_super_process_h_
