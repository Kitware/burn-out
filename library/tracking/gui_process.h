/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gui_process_h_
#define vidtk_gui_process_h_

#include <utilities/timestamp.h>

#include <tracking/da_so_tracker.h>
#include <tracking/image_object.h>
#include <tracking/track.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

namespace vidtk
{

struct gui_process_impl; 

class gui_process : public process
{
public:
  typedef gui_process self_type;
  gui_process( vcl_string const& name, vcl_string const& cls = "gui_process" );
  ~gui_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  virtual bool persist();

  // PORTS //

  // Input ports
  void set_source_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_image( vil_image_view<vxl_byte> const& image );
  VIDTK_INPUT_PORT( set_image, vil_image_view<vxl_byte> const& );

  void set_objects( vcl_vector< image_object_sptr > const& objects );
  VIDTK_INPUT_PORT( set_objects, vcl_vector< image_object_sptr > const& );

  void set_filter1_objects( vcl_vector< image_object_sptr > const& objects );
  VIDTK_INPUT_PORT( set_filter1_objects, vcl_vector< image_object_sptr > const& );

  void set_init_tracks( vcl_vector<track_sptr> const& tracks );
  VIDTK_INPUT_PORT( set_init_tracks, vcl_vector<track_sptr> const& );

  void set_filter2_tracks( vcl_vector<track_sptr> const& tracks );
  VIDTK_INPUT_PORT( set_filter2_tracks, vcl_vector<track_sptr> const& );

  void set_active_tracks( vcl_vector<track_sptr> const& tracks );
  VIDTK_INPUT_PORT( set_active_tracks, vcl_vector<track_sptr> const& );

  void set_fg_image( vil_image_view<bool> const& img );
  VIDTK_INPUT_PORT( set_fg_image, vil_image_view<bool> const& );

  void set_morph_image( vil_image_view<bool> const& img );
  VIDTK_INPUT_PORT( set_morph_image, vil_image_view<bool> const& );

  void set_diff_image( vil_image_view<float> const& img );
  VIDTK_INPUT_PORT( set_diff_image, vil_image_view<float> const& );

  void set_world_homog( vgl_h_matrix_2d<double> const& homog );
  VIDTK_INPUT_PORT( set_world_homog, vgl_h_matrix_2d<double> const& );

  void set_active_trkers( vcl_vector<da_so_tracker_sptr> const& trackers );
  VIDTK_INPUT_PORT( set_active_trkers, vcl_vector<da_so_tracker_sptr> const& );

  void set_amhi_image( vil_image_view<vxl_byte> const& img );
  VIDTK_INPUT_PORT( set_amhi_image, vil_image_view<vxl_byte> const& );

  void set_tracker_gate_sigma( double );
  VIDTK_INPUT_PORT( set_tracker_gate_sigma, double);

  void set_world_units_per_pixel( double );
  VIDTK_INPUT_PORT( set_world_units_per_pixel, double );

  // Output ports
  vil_image_view<vxl_byte> const& gui_image_out() const;
  VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte> const&, gui_image_out );

  bool const& amhi_image_enabled() const;
  VIDTK_OUTPUT_PORT( bool const&, amhi_image_enabled );

  // END PORTS //

  void step_once();
  void pause_cb();

protected:
  gui_process_impl * impl_;
};

} // end namespace vidtk

#endif// vidtk_gui_process_h_

