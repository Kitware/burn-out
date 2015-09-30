/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_gui_process_impl_h_
#define vidtk_gui_process_impl_h_

namespace vidtk
{

struct gui_process_impl
{
  gui_process_impl()
  : disabled( true ),
    tracker_gate_sigma( 3.0 ),
    gsd( -1 ),
    synched_tabs( true )
  {}

  // PORT VARIABLES //
  bool disabled;
  bool amhi_image_enabled;
  vidtk::timestamp ts;
  vil_image_view<vxl_byte> img;
  vcl_vector< image_object_sptr > objs;
  vcl_vector< image_object_sptr > filter1_objs;
  vcl_vector<track_sptr> init_tracks;
  vcl_vector<track_sptr> filter2_tracks;
  vcl_vector<track_sptr> active_tracks;
  vil_image_view<bool> fg_image;
  vil_image_view<bool> gmm_image;
  vil_image_view<bool> morph_image;
  vil_image_view<float> diff_image;
  vgl_h_matrix_2d<double> homog;
  vcl_vector<da_so_tracker_sptr> active_trackers;
  vil_image_view<vxl_byte> amhi_image;
  vil_image_view<vxl_byte> output_image;
  double tracker_gate_sigma;
  double gsd;

  // CONFIG PARAMETERS //
  config_block config;
  bool show_conn_comp;
  bool show_world_conn_comp;
  bool show_filtered_conn_comp;
  bool show_world_filtered_conn_comp;
  bool show_mod;
  bool show_world_mod;
  vcl_string show_fg_image_str;
  bool show_tracks;
  bool show_image;
  bool show_filtered_tracks;
  bool show_world_tracks;
  bool show_world_image;
  bool show_diff_image;
  bool persist_gui_at_end;
  unsigned int pause_at;
  bool show_amhi_image_pan;
  bool show_amhi_tracks;
  bool suppress_number;
  vcl_string gui_args;
  unsigned grid_size;
  unsigned max_tabs;
  vcl_vector<vcl_string> img_names;
  vcl_string tab_names;
  bool synched_tabs;

  //^^ CONFIG PARAMs ^^//

  bool b_pause;
  bool b_step;

  bool show_source_image;
  bool show_world;
  bool show_amhi_image;
  int show_fg_image;

  int num_imgs;
};

} // end namespace vidtk

#endif// vidtk_gui_process_impl_h_


