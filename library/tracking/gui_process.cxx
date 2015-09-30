/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "gui_process.h"

#include <tracking/gui_process_impl.h>

namespace vidtk
{

gui_process
::gui_process( vcl_string const& name, vcl_string const& cls )
: process( name, cls ),
  impl_(0)
{
  impl_ = new gui_process_impl;

  impl_->config.add( "disabled", "true" );
  impl_->config.add( "show_conn_comp", "false" );
  impl_->config.add( "show_world_conn_comp", "false" );
  impl_->config.add( "show_filtered_conn_comp", "false" );
  impl_->config.add( "show_world_filtered_conn_comp", "false" );
  impl_->config.add( "show_mod", "false" );
  impl_->config.add( "show_world_mod", "false" );
  impl_->config.add( "show_fg_image_str", "none" );
  impl_->config.add( "show_tracks", "false" );
  impl_->config.add( "show_image", "false" );
  impl_->config.add( "show_filtered_tracks", "false" );
  impl_->config.add( "show_world_tracks", "false" );
  impl_->config.add( "show_world_image", "false" );
  impl_->config.add( "show_diff_image", "false" );
  impl_->config.add( "persist_gui_at_end", "false" );
  impl_->config.add( "pause_at", "0" );
  impl_->config.add( "show_amhi_image_pan", "false" );
  impl_->config.add( "show_amhi_tracks", "false" );
  impl_->config.add( "suppress_number", "false" );
  impl_->config.add( "vgui_args", "" );
  impl_->config.add( "max_tabs", "5" );
  impl_->config.add( "grid_size", "5" );
  impl_->config.add( "tab_names", "Source,Foreground,Difference,World,AMHI" );
  impl_->config.add_parameter( "synchronize_tabs",
    "true",
    "Enabled synchronization of the multiple tabs displayed in the GUI." );

  impl_->b_pause = false;
  impl_->b_step = false;
}


gui_process
::~gui_process()
{
  delete impl_;
}


config_block
gui_process
::params() const
{
  return impl_->config;
}


bool
gui_process
::set_params( config_block const& blk )
{
  blk.get( "disabled", impl_->disabled );
  blk.get( "show_conn_comp", impl_->show_conn_comp );
  blk.get( "show_world_conn_comp", impl_->show_world_conn_comp );
  blk.get( "show_filtered_conn_comp", impl_->show_filtered_conn_comp );
  blk.get( "show_world_filtered_conn_comp",
           impl_->show_world_filtered_conn_comp );
  blk.get( "show_mod", impl_->show_mod );
  blk.get( "show_world_mod", impl_->show_world_mod );
  blk.get( "show_fg_image_str", impl_->show_fg_image_str );
  blk.get( "show_tracks", impl_->show_tracks );
  blk.get( "show_image", impl_->show_image );
  blk.get( "show_filtered_tracks", impl_->show_filtered_tracks );
  blk.get( "show_world_tracks", impl_->show_world_tracks );
  blk.get( "show_world_image", impl_->show_world_image );
  blk.get( "show_diff_image", impl_->show_diff_image );
  blk.get( "persist_gui_at_end", impl_->persist_gui_at_end );
  blk.get( "pause_at", impl_->pause_at );
  blk.get( "show_amhi_image_pan", impl_->show_amhi_image_pan );
  blk.get( "show_amhi_tracks", impl_->show_amhi_tracks );
  blk.get( "suppress_number", impl_->suppress_number );
  blk.get( "max_tabs", impl_->max_tabs );
  blk.get( "grid_size", impl_->grid_size );
  blk.get( "vgui_args", impl_->gui_args );
  blk.get( "tab_names", impl_->tab_names );
  blk.get( "synchronize_tabs", impl_->synched_tabs );

  impl_->config.update( blk );
  return true;
}


bool
gui_process
::initialize()
{
  return true;
}

bool
gui_process
::step()
{
  return true;
}

bool
gui_process
::persist()
{
  return true;
}

void
gui_process
::set_source_timestamp( vidtk::timestamp const& ts )
{
  impl_->ts = ts;
}

void
gui_process
:: set_image( vil_image_view<vxl_byte> const& image )
{
  impl_->img = image;
}

void
gui_process
:: set_objects( vcl_vector< image_object_sptr > const& objects )
{
  impl_->objs = objects;
}

void
gui_process
:: set_filter1_objects( vcl_vector< image_object_sptr > const& objects )
{
  impl_->filter1_objs = objects;
}

void
gui_process
:: set_init_tracks( vcl_vector<track_sptr> const& tracks )
{
  impl_->init_tracks = tracks;
}

void
gui_process
:: set_filter2_tracks( vcl_vector<track_sptr> const& tracks )
{
  impl_->filter2_tracks = tracks;
}

void
gui_process
:: set_active_tracks( vcl_vector<track_sptr> const& tracks )
{
  impl_->active_tracks = tracks;
}

void
gui_process
:: set_fg_image( vil_image_view<bool> const& img )
{
  impl_->fg_image = img;
}

void
gui_process
:: set_morph_image( vil_image_view<bool> const& img )
{
  impl_->morph_image = img;
}

void
gui_process
:: set_diff_image( vil_image_view<float> const& img )
{
  impl_->diff_image = img;
}

void
gui_process
:: set_world_homog( vgl_h_matrix_2d<double> const& homog )
{
  impl_->homog = homog;
}

void
gui_process
:: set_active_trkers( vcl_vector<da_so_tracker_sptr> const& trackers )
{
  impl_->active_trackers = trackers;
}

void
gui_process
:: set_amhi_image( vil_image_view<vxl_byte> const& img )
{
  impl_->amhi_image = img;
}

vil_image_view<vxl_byte> const&
gui_process
:: gui_image_out() const
{
  return impl_->output_image;
}

bool const&
gui_process
::amhi_image_enabled() const
{
  return impl_->amhi_image_enabled;
}

void
gui_process
::set_tracker_gate_sigma( double gate_sigma )
{
  impl_->tracker_gate_sigma = gate_sigma;
}


void
gui_process
::set_world_units_per_pixel( double gsd )
{
  impl_->gsd = gsd;
}


void
gui_process
::step_once()
{
  impl_->b_step = true;
}

void
gui_process
::pause_cb()
{
  impl_->b_pause = ! impl_->b_pause;
}

} // namespace

