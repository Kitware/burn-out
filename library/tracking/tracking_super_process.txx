/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tracking_super_process.h"

#include <pipeline/sync_pipeline.h>

#include <utilities/config_block.h>
#include <utilities/ring_buffer_process.h>
#include <utilities/paired_buffer_process.h>
#include <utilities/homography_reader_process.h>
#include <utilities/unchecked_return_value.h>

#include <tracking/compute_object_features_process.h>
#include <tracking/filter_tracks_process.h>
#include <tracking/filter_tracks_process2.h>
#include <tracking/track_linking_process.h>
#include <tracking/transform_tracks_process.h>
#include <tracking/amhi_create_image_process.h>
#include <tracking/da_tracker_process.h>
#include <tracking/multiple_features_process.h>
#include <tracking/back_tracking_super_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/track_initializer_process.h>
#include <tracking/amhi_initializer_process.h>
#include <tracking/da_so_tracker_generator_process.h>
#include <tracking/transform_tracks_process.h>
#include <tracking/gui_process.h>
#include <tracking/frame_objects_buffer_process.h>

#include <video/deep_copy_image_process.h>
#include <video/image_list_writer_process.h>
#include <vcl_algorithm.h>

#include <boost/foreach.hpp>

namespace vidtk
{

template< class PixType >
class tracking_super_process_impl
{
public:
  typedef vidtk::super_process_pad_impl< vcl_vector< track_sptr > > tracks_pad;

  // Processes
  process_smart_pointer< amhi_create_image_process< PixType > > proc_amhi_image;
  process_smart_pointer< amhi_initializer_process<PixType> > proc_amhi_init;
  process_smart_pointer< back_tracking_super_process< PixType > > proc_back_tracking_sp;
  process_smart_pointer< compute_object_features_process<  PixType  > > proc_histogram;
  process_smart_pointer< da_tracker_process > proc_tracker;
  process_smart_pointer< da_so_tracker_generator_process > proc_tracker_initializer;
  process_smart_pointer< filter_tracks_process > proc_trk_filter1;
  process_smart_pointer< filter_tracks_process2 > proc_trk_filter2;
  process_smart_pointer< multiple_features_process > proc_multi_features;
  process_smart_pointer< ring_buffer_process< timestamp > > proc_timestamp_buffer;
  process_smart_pointer< ring_buffer_process< vgl_h_matrix_2d<double> > > proc_wld2img_homog_buffer;
  process_smart_pointer< ring_buffer_process< vgl_h_matrix_2d<double> > > proc_img2wld_homog_buffer;
  process_smart_pointer< ring_buffer_process< vil_image_view< PixType  > > > proc_image_buffer;
  process_smart_pointer< paired_buffer_process< timestamp, vil_image_view< PixType  > > > proc_ts_image_buffer;
  process_smart_pointer< paired_buffer_process< timestamp, double > > proc_gsd_buffer;
  process_smart_pointer< ring_buffer_process< vcl_vector<image_object_sptr> > > proc_unassgn_mods_buffer;
  process_smart_pointer< ring_buffer_process< vcl_vector<image_object_sptr> > > proc_mods_buffer;
  process_smart_pointer< track_initializer_process > proc_track_init;
  process_smart_pointer< track_linking_process > proc_linker;
  process_smart_pointer< track_writer_process > proc_track_writer_unfiltered;
  process_smart_pointer< track_writer_process > proc_track_writer_filtered;
  process_smart_pointer< track_writer_process > proc_track_writer_unfiltered_linked;
  process_smart_pointer< transform_tracks_process< PixType > > proc_add_img_info_to_start;
  process_smart_pointer< transform_tracks_process< PixType > > proc_add_img_info_to_end;
  process_smart_pointer< transform_tracks_process< PixType > > proc_add_latlon;
  process_smart_pointer< transform_tracks_process< PixType > > proc_fg_init;
  process_smart_pointer< transform_tracks_process< PixType > > proc_fg_update;
  process_smart_pointer< transform_tracks_process< PixType > > proc_state_to_image;
  process_smart_pointer< gui_process > proc_gui;
  process_smart_pointer< image_list_writer_process< PixType > > proc_gui_writer;
  process_smart_pointer< deep_copy_image_process<vxl_byte> > proc_image_copy;
  process_smart_pointer< transform_tracks_process< PixType > > proc_set_init_data;
  process_smart_pointer< frame_objects_buffer_process > proc_fg_objs_buffer;

  process_smart_pointer< tracks_pad > pad_filtered_tracks;
  process_smart_pointer< tracks_pad > pad_linked_tracks;

  config_block config;
  config_block default_config;
  config_block forced_config;
  config_block dependency_config;

  bool gui_disabled;
  // Temporary variable holding the sptr to the vgui_process passed
  // in from the app.
  process_smart_pointer< gui_process > gui;

  // I/O data
  timestamp current_timestamp;
  double world_units_per_pixel;
  vil_image_view< PixType > current_image;
  vil_image_view< bool > fg_image;
  vil_image_view< float > diff_image;
  vil_image_view< bool > morph_image;
  vcl_vector< image_object_sptr > unfiltered_objs;
  // input homographies
  image_to_plane_homography in_src2wld_homog;
  plane_to_image_homography in_wld2src_homog;
  image_to_image_homography in_src2ref_homog;
  plane_to_utm_homography in_wld2utm_homog;
  // output homography
  image_to_utm_homography out_src2utm_homog;
  track_vector_t out_active_tracks;

  // -- CTOR -- //
  tracking_super_process_impl()
    : proc_amhi_image( NULL ),
      proc_amhi_init( NULL ),
      proc_back_tracking_sp( NULL ),
      proc_histogram( NULL ),
      proc_tracker( NULL ),
      proc_tracker_initializer( NULL ),
      proc_trk_filter1( NULL ),
      proc_trk_filter2( NULL ),
      proc_multi_features( NULL ),
      proc_timestamp_buffer( NULL ),
      proc_wld2img_homog_buffer( NULL ),
      proc_img2wld_homog_buffer( NULL ),
      proc_image_buffer( NULL ),
      proc_ts_image_buffer( NULL ),
      proc_gsd_buffer( NULL ),
      proc_unassgn_mods_buffer( NULL ),
      proc_mods_buffer( NULL ),
      proc_track_init( NULL ),
      proc_linker( NULL ),
      proc_track_writer_unfiltered( NULL ),
      proc_track_writer_filtered( NULL ),
      proc_track_writer_unfiltered_linked( NULL ),
      proc_add_img_info_to_start( NULL ),
      proc_add_img_info_to_end( NULL ),
      proc_add_latlon( NULL ),
      proc_fg_init( NULL ),
      proc_fg_update( NULL ),
      proc_state_to_image ( NULL ),
      proc_gui( NULL ),
      proc_gui_writer( NULL ),
      proc_image_copy( NULL ),
      proc_set_init_data( NULL ),
      proc_fg_objs_buffer( NULL ),
      pad_filtered_tracks( NULL ),
      pad_linked_tracks( NULL ),
      config(),
      default_config(),
      forced_config(),
      dependency_config(),
      gui_disabled( true ),
      gui( new gui_process( "vgui" ) )
  {
    // zero out the world-to-src homography; check before computing src2utm in step
    double zeros[9] = {0};
    in_wld2src_homog.set_transform( homography::transform_t(zeros) );
  }

  virtual void create_process_configs()
  {
    proc_histogram = new compute_object_features_process< PixType >( "histogram" );
    config.add_subblock( proc_histogram->params(), proc_histogram->name() );

    proc_trk_filter1 = new filter_tracks_process( "trk_filter1" );
    config.add_subblock( proc_trk_filter1->params(), proc_trk_filter1->name() );

    proc_amhi_image = new amhi_create_image_process< PixType >( "amhi_debug_image" );
    config.add_subblock( proc_amhi_image->params(), proc_amhi_image->name() );

    proc_tracker = new da_tracker_process( "tracker" );
    config.add_subblock( proc_tracker->params(), proc_tracker->name() );

    proc_linker = new track_linking_process( "linker" );
    config.add_subblock( proc_linker->params(), proc_linker->name() );

    proc_multi_features = new multiple_features_process( "multi_features" );
    config.add_subblock( proc_multi_features->params(), proc_multi_features->name() );

    proc_trk_filter2 = new filter_tracks_process2( "trk_filter2" );
    config.add_subblock( proc_trk_filter2->params(), proc_trk_filter2->name() );

    proc_image_copy = new deep_copy_image_process< PixType >( "source_image_deep_copy" );
    config.add_subblock( proc_image_copy->params(), proc_image_copy->name() );

    proc_image_buffer =
      new ring_buffer_process< vil_image_view<PixType > >("source_image_buffer" );
    config.add_subblock( proc_image_buffer->params(), proc_image_buffer->name() );

    proc_ts_image_buffer =
      new paired_buffer_process< timestamp, vil_image_view< PixType > >("ts_source_image_buffer");
    config.add_subblock( proc_ts_image_buffer->params(), proc_ts_image_buffer->name() );

    proc_gsd_buffer = new paired_buffer_process< timestamp, double >("gsd_buffer");
    config.add_subblock( proc_gsd_buffer->params(), proc_gsd_buffer->name() );

    proc_timestamp_buffer = new ring_buffer_process< timestamp >( "timestamp_buffer" );
    config.add_subblock( proc_timestamp_buffer->params(),
                         proc_timestamp_buffer->name() );

    proc_add_img_info_to_start =
      new transform_tracks_process< PixType >( "added_to_begin" );
    config.add_subblock( proc_add_img_info_to_start->params(),
                         proc_add_img_info_to_start->name() );

    proc_add_img_info_to_end =
      new transform_tracks_process< PixType >( "added_to_end" );
    config.add_subblock( proc_add_img_info_to_end->params(),
                         proc_add_img_info_to_end->name() );

    proc_add_latlon =
      new transform_tracks_process< PixType >( "add_latlon" );
    config.add_subblock( proc_add_latlon->params(),
                         proc_add_latlon->name() );

    proc_state_to_image =
      new transform_tracks_process< PixType >( "state_to_image" );
    config.add_subblock( proc_state_to_image->params(),
                         proc_state_to_image->name() );

    proc_back_tracking_sp =
      new back_tracking_super_process< PixType >( "back_tracking_sp" );
    config.add_subblock( proc_back_tracking_sp->params(),
                         proc_back_tracking_sp->name() );

    proc_img2wld_homog_buffer =
      new ring_buffer_process< vgl_h_matrix_2d<double> >( "img2wld_homog_buffer" );
    config.add_subblock( proc_img2wld_homog_buffer->params(),
                         proc_img2wld_homog_buffer->name() );

    proc_wld2img_homog_buffer =
      new ring_buffer_process< vgl_h_matrix_2d<double> >( "wld2img_homog_buffer" );
    config.add_subblock( proc_wld2img_homog_buffer->params(),
                         proc_wld2img_homog_buffer->name() );

    proc_fg_init = new transform_tracks_process< PixType >( "fg_init" );
    config.add_subblock( proc_fg_init->params(),
                         proc_fg_init->name() );

    proc_fg_update = new transform_tracks_process< PixType >( "fg_update" );
    config.add_subblock( proc_fg_update->params(),
                         proc_fg_update->name() );

    proc_unassgn_mods_buffer =
      new ring_buffer_process< vcl_vector<image_object_sptr> >("unassigned_mod_buffer" );
    config.add_subblock( proc_unassgn_mods_buffer->params(),
                         proc_unassgn_mods_buffer->name() );

    proc_mods_buffer =
      new ring_buffer_process< vcl_vector<image_object_sptr> >("mods_buffer" );
    config.add_subblock( proc_mods_buffer->params(),
                         proc_mods_buffer->name() );

    proc_tracker_initializer =
      new da_so_tracker_generator_process( "tracker_initializer" );
    config.add_subblock( proc_tracker_initializer->params(),
                         proc_tracker_initializer->name() );

    proc_amhi_init = new amhi_initializer_process<PixType>( "amhi_init" );
    config.add_subblock( proc_amhi_init->params(), proc_amhi_init->name() );

    proc_track_init = new track_initializer_process( "track_init" );
    config.add_subblock( proc_track_init->params(), proc_track_init->name() );

    proc_track_writer_unfiltered =
      new track_writer_process( "output_tracks_unfiltered" );
    config.add_subblock( proc_track_writer_unfiltered->params(),
                         proc_track_writer_unfiltered->name() );

    proc_track_writer_filtered =
      new track_writer_process( "output_tracks_filtered" );
    config.add_subblock( proc_track_writer_filtered->params(),
                         proc_track_writer_filtered->name() );

    proc_track_writer_unfiltered_linked =
      new track_writer_process( "output_tracks_unfiltered_linked" );
    config.add_subblock( proc_track_writer_unfiltered_linked->params(),
                         proc_track_writer_unfiltered_linked->name() );

    proc_gui = new gui_process( "vgui" );
    config.add_subblock( proc_gui->params(), proc_gui->name() );

    proc_gui_writer = new image_list_writer_process< PixType >( "gui_writer" );
    config.add_subblock( proc_gui_writer->params(), proc_gui_writer->name() );

    proc_set_init_data =
      new transform_tracks_process< PixType >( "set_init_data" );
    config.add_subblock( proc_set_init_data->params(),
                         proc_set_init_data->name() );

    proc_fg_objs_buffer = new frame_objects_buffer_process( "fg_objs_buffer" );
    config.add_subblock( proc_fg_objs_buffer->params(),
                         proc_fg_objs_buffer->name() );

    // Output pads
    pad_filtered_tracks = new tracks_pad( "filtered_tracks_out_pad" );
    pad_linked_tracks = new tracks_pad( "linked_tracks_out_pad" );

    // Over-riding the process default with this super-process defaults.
    default_config.add( proc_track_writer_unfiltered->name()
                        + ":disabled", "true" );
    default_config.add( proc_track_writer_filtered->name()
                        + ":disabled", "true" );
    default_config.add( proc_track_writer_unfiltered_linked->name()
                        + ":disabled", "true" );
    default_config.add( proc_gui_writer->name() + ":disabled", "true" );

    // Permanently over-riding the process defaults
    forced_config.add( proc_add_img_info_to_end->name()
                       + ":add_image_info:location", "end" );
    forced_config.add( proc_add_img_info_to_start->name()
                       + ":add_image_info:location", "start" );
    forced_config.add( proc_set_init_data->name()
                       + ":reassign_track_ids:disabled", "false" );
    forced_config.add( proc_tracker->name()
                       + ":reassign_track_ids", "false" );
    forced_config.add( proc_set_init_data->name()
                       + ":mark_used_mods:disabled", "false" );


    try
    {
      config.update( default_config );
      config.update( forced_config );
    }
    catch( unchecked_return_value& e)
    {
      log_error( "tracking_super_process: Failed to set (default_/forced_) "
        "config, because: " << e.what() << "\n" );
      return;
    }

    // Temporary values added here. The *real* values will be added in set_params()
    dependency_config.add( proc_tracker->name()
                           + ":terminate:missed_count", "0" );
    dependency_config.add( proc_track_init->name() + ":delta", "0" );
    dependency_config.add( proc_timestamp_buffer->name() + ":length", "0" );
    dependency_config.add( proc_wld2img_homog_buffer->name() + ":length", "0" );
    dependency_config.add( proc_img2wld_homog_buffer->name() + ":length", "0" );
    dependency_config.add( proc_image_buffer->name() + ":length", "0" );
    dependency_config.add( proc_ts_image_buffer->name() + ":length", "0" );
    dependency_config.add( proc_gsd_buffer->name() + ":length", "0" );
    dependency_config.add( proc_unassgn_mods_buffer->name() + ":length", "0" );
    dependency_config.add( proc_mods_buffer->name() + ":length", "0" );
    dependency_config.add( proc_fg_objs_buffer->name() + ":length", "0" );
    dependency_config.add( proc_track_init->name() + ":miss_penalty", "1000000" );
    dependency_config.add( proc_histogram->name()
                           + ":compute_histogram", "false" );

    // Foreground tracking dependencies
    dependency_config.add( proc_tracker->name()
      + ":fg_tracking:enabled", "false" );
    dependency_config.add( proc_fg_update->name()
      + ":update_fg_model:disabled", "true" );

    // Back-tracking dependencies
    vcl_string btsp = proc_back_tracking_sp->name();
    dependency_config.add( btsp + ":disabled", "true" );
    dependency_config.add( btsp + ":duration_frames", "0" );
    dependency_config.add( btsp + ":track_init_duration_frames", "0" );
    dependency_config.add_subblock(
      proc_back_tracking_sp->params().subblock( "back_tracker" ),
      btsp + ":back_tracker" );
    dependency_config.add_subblock(
      proc_back_tracking_sp->params().subblock( "fg_init" ),
      btsp + ":fg_init" );
    dependency_config.add_subblock(
      proc_back_tracking_sp->params().subblock( "fg_update" ),
      btsp + ":fg_update" );
    dependency_config.add_subblock(
      proc_back_tracking_sp->params().subblock( "tracker_initializer" ),
      btsp + ":tracker_initializer" );

    // AMHI dependencies
    dependency_config.add( proc_amhi_init->name() + ":disabled", "true" );
    dependency_config.add( proc_fg_init->name() + ":create_fg_model:ssd:amhi_mode", "false" );
    dependency_config.add( proc_amhi_image->name() + ":disabled", "true" );

    // No reason to update config with dependency_config
  }

  virtual void setup_pipeline( pipeline_sptr & pp )
  {
    sync_pipeline* p = new sync_pipeline;
    pp = p;

    // TODO: Replace proc_image_buffer. Being replaced by proc_ts_image_buffer.
    //       Also replace all other ring_buffer_process \w paired_buffer_process.
    p->add( proc_image_buffer );
    p->add( proc_ts_image_buffer );
    p->add( proc_histogram );
    p->add( proc_multi_features );
    p->add( proc_tracker );
    p->add( proc_state_to_image );
    p->add( proc_timestamp_buffer );
    p->add( proc_trk_filter1 );
    p->add( proc_add_img_info_to_end );
    p->add( proc_add_latlon );
    p->add( proc_trk_filter2 );
    p->add( proc_amhi_image );
    p->add( proc_linker );
    p->add( proc_add_img_info_to_start );
    p->add( proc_back_tracking_sp );
    p->add( proc_fg_init );
    p->add( proc_fg_update );
    p->add( proc_amhi_init );
    p->add( proc_track_init );
    p->add( proc_unassgn_mods_buffer );
    p->add( proc_tracker_initializer );
    p->add( proc_track_writer_unfiltered );
    p->add( proc_track_writer_filtered );
    p->add( proc_track_writer_unfiltered_linked );
    p->add( proc_image_copy );
    p->add( proc_set_init_data );
    p->add( proc_wld2img_homog_buffer );

    // Output pads
    p->add( pad_filtered_tracks );
    p->add( pad_linked_tracks );

    // incoming connections of proc_image_buffer
    p->connect( proc_image_copy->image_port(),
                proc_image_buffer->set_next_datum_port() );

    // incoming connections of proc_ts_image_buffer
    p->connect( proc_image_copy->image_port(),
                proc_ts_image_buffer->set_next_datum_port() );

    // incomfing connections of proc_histogram
    p->connect( proc_image_copy->image_port(),
                proc_histogram->set_source_image_port() );

    // incoming connections of proc_track_init
    p->connect( proc_unassgn_mods_buffer->buffer_port(),
                proc_track_init->set_mod_buffer_port() );
    p->connect( proc_timestamp_buffer->buffer_port(),
                proc_track_init->set_timestamp_buffer_port() );

    proc_track_init->set_mf_params( proc_multi_features->mf_init_params() ) ;
    p->add_execution_dependency( proc_multi_features.ptr(),
                                 proc_track_init.ptr() );

    // incoming connections of proc_state_to_image
    p->connect( proc_fg_update->out_tracks_port(),
                proc_state_to_image->in_tracks_port());

    // incoming connections of proc_trk_filter1
    p->connect( proc_track_init->new_tracks_port(),
                proc_trk_filter1->set_source_tracks_port() );

    // incoming connections of proc_set_init_data
    p->connect( proc_trk_filter1->tracks_port(),
                proc_set_init_data->in_tracks_port() );

    // incoming connections of proc_amhi_init
    p->connect( proc_set_init_data->out_tracks_port(),
                proc_amhi_init->set_source_tracks_port() );
    p->connect( proc_image_buffer->buffer_port(),
                proc_amhi_init->set_source_image_buffer_port() );

    // incoming connections of proc_tracker_
    p->connect( proc_histogram->objects_port(),
                proc_tracker->set_objects_port() );
    proc_tracker->set_mf_params( proc_multi_features->mf_online_params() );
    p->add_execution_dependency( proc_multi_features.ptr(),
                                 proc_tracker.ptr() );
    p->connect_without_dependency( proc_tracker_initializer->new_trackers_port(),
                                   proc_tracker->set_new_trackers_port());
    p->connect_without_dependency( proc_tracker_initializer->new_wh_trackers_port(),
                                   proc_tracker->set_new_wh_trackers_port());
    p->connect_without_dependency( proc_state_to_image->out_tracks_port(),
                                   proc_tracker->set_active_tracks_port());
    p->connect( proc_image_copy->image_port(),
                proc_tracker->set_source_image_port() );

    p->connect( proc_state_to_image->out_tracks_port(),
                proc_amhi_image->set_source_tracks_port() );
    p->connect( proc_image_copy->image_port(),
                proc_amhi_image->set_source_image_port() );

    // incoming and outgoing connections of proc_fg_objs_buffer
    if( !config.get<bool>( "back_tracking_disabled" ) )
    {
      proc_back_tracking_sp->set_mf_params( proc_multi_features->mf_online_params() );
      p->add_execution_dependency( proc_multi_features.ptr(),
                                   proc_back_tracking_sp.ptr() );
      p->connect( proc_image_buffer->buffer_port(),
                  proc_back_tracking_sp->set_image_buffer_port() );
      p->connect( proc_ts_image_buffer->buffer_port(),
                  proc_back_tracking_sp->set_ts_image_buffer_port() );
      p->connect( proc_timestamp_buffer->buffer_port(),
                  proc_back_tracking_sp->set_timestamp_buffer_port() );

      p->add( proc_mods_buffer );
      p->connect( proc_histogram->objects_port(),
                  proc_mods_buffer->set_next_datum_port() );
      p->connect( proc_mods_buffer->buffer_port(),
                  proc_back_tracking_sp->set_mod_buffer_port() );

      p->add( proc_img2wld_homog_buffer );
      p->connect( proc_img2wld_homog_buffer->buffer_port(),
                  proc_back_tracking_sp->set_img2wld_homog_buffer_port() );

      p->connect( proc_wld2img_homog_buffer->buffer_port(),
                  proc_back_tracking_sp->set_wld2img_homog_buffer_port() );

      p->add( proc_fg_objs_buffer );
      p->connect( proc_tracker->updated_fg_objects_port(),
                  proc_fg_objs_buffer->set_current_objects_port() );
      p->connect( proc_fg_objs_buffer->frame_objects_port(),
                  proc_back_tracking_sp->set_fg_objs_buffer_port() );
      p->connect_without_dependency( proc_back_tracking_sp->updated_fg_objs_buffer_port(),
                                     proc_fg_objs_buffer->set_updated_frame_objects_port() );

      p->add( proc_gsd_buffer );
      p->connect( proc_gsd_buffer->buffer_port(),
                  proc_back_tracking_sp->set_gsd_buffer_port() );

    } // !config.get<bool>( "back_tracking_disabled" )

    // incoming connections of proc_back_tracking_sp
    p->connect( proc_amhi_init->out_tracks_port(),
                proc_back_tracking_sp->in_tracks_port() );

    // incoming connections of proc_add_img_info_to_start_
    p->connect( proc_image_buffer->buffer_port(),
                proc_add_img_info_to_start->set_source_image_buffer_port() );
    p->connect( proc_back_tracking_sp->updated_tracks_port(),
                proc_add_img_info_to_start->in_tracks_port() );

    //incoming connections of proc_fg_init_
    p->connect( proc_add_img_info_to_start->out_tracks_port(),
                proc_fg_init->in_tracks_port() );
    p->connect( proc_ts_image_buffer->buffer_port(),
                proc_fg_init->set_ts_source_image_buffer_port() );

    //incoming connections of proc_fg_update_
    p->connect( proc_tracker->active_tracks_port(),
                proc_fg_update->in_tracks_port() );
    p->connect( proc_ts_image_buffer->buffer_port(),
                proc_fg_update->set_ts_source_image_buffer_port() );

    // incoming connections of proc_tracker_initializer
    p->connect( proc_fg_init->out_tracks_port(),
                proc_tracker_initializer->set_new_tracks_port() );
    p->connect( proc_wld2img_homog_buffer->buffer_port(),
                proc_track_init->set_wld2img_homog_buffer_port() );

    // incoming connections of proc_add_img_info_to_end_
    p->connect_optional( proc_image_buffer->buffer_port(),
                         proc_add_img_info_to_end->set_source_image_buffer_port() );
    p->connect( proc_add_latlon->out_tracks_port(),
                proc_add_img_info_to_end->in_tracks_port() );

    // incoming connections of proc_add_latlon
    p->connect( proc_tracker->terminated_tracks_port(),
                proc_add_latlon->in_tracks_port() );

    // incoming connections of proc_linker_
    p->connect( proc_state_to_image->out_tracks_port(),
                proc_linker->set_input_active_tracks_port());
    p->connect( proc_add_img_info_to_end->out_tracks_port(),
                proc_linker->set_input_terminated_tracks_port());

    // incoming connections of proc_trk_filter2
    p->connect( proc_add_img_info_to_end->out_tracks_port(),
                proc_trk_filter2->set_source_tracks_port() );

    // incoming connections of proc_unassgn_mods_buffer
    p->connect( proc_tracker->unassigned_objects_port(),
                proc_unassgn_mods_buffer->set_next_datum_port() );

    // incoming connections of proc_track_writer_unfiltered
    p->connect( proc_add_img_info_to_end->out_tracks_port(),
                proc_track_writer_unfiltered->set_tracks_port() );

    // incoming connections of proc_track_writer_filtered
    p->connect( proc_trk_filter2->tracks_port(),
                proc_track_writer_filtered->set_tracks_port() );

    // incoming connections of proc_track_writer_unfiltered_linked
    p->connect( proc_linker->get_output_tracks_port(),
                proc_track_writer_unfiltered_linked->set_tracks_port() );

    // Output pads
    p->connect( proc_trk_filter2->tracks_port(),
                pad_filtered_tracks->set_value_port() );
    p->connect( proc_linker->get_output_tracks_port(),
                pad_linked_tracks->set_value_port() );

    // incoming connections of proc_gui
    if( gui_disabled && gui->class_name() != "gui_process" )
    {
      proc_gui = new gui_process( "vgui" );
    }
    else
    {
      proc_gui = gui;

      p->add( proc_gui );

      p->connect( proc_trk_filter1->tracks_port(),
                  proc_gui->set_init_tracks_port() );
      p->connect( proc_trk_filter2->tracks_port(),
                  proc_gui->set_filter2_tracks_port() );
      p->connect( proc_tracker->active_tracks_port(),
                  proc_gui->set_active_tracks_port() );
      p->connect( proc_tracker->active_trackers_port(),
                  proc_gui->set_active_trkers_port() );
      p->connect( proc_amhi_image->amhi_image_port(),
                  proc_gui->set_amhi_image_port() );

      p->add( proc_gui_writer );
      p->connect( proc_gui->gui_image_out_port(),
                  proc_gui_writer->set_image_port() );
    }
  }
}; // class tracking_super_process_impl

template< class PixType >
tracking_super_process< PixType >
::tracking_super_process( vcl_string const& name )
  : super_process( name, "tracking_super_process" ),
    impl_( new tracking_super_process_impl<PixType> )
{
  impl_->create_process_configs();

  impl_->config.add_parameter( "back_tracking_disabled",
    "true",
    "true of false. Wheter back tracking should be performed. "
    " This parameter will over-ride back_tracking_super_process::disabled and "
    "will control other underlying dependencies." );

  impl_->config.add_parameter( "track_init_duration_frames",
    "0",
    "Number of frames representing the duration of the temporal window "
    "over which new tracks will be initialized. This parameter over-rides "
    "track_initializer_process::delta and controls other underlying "
    "dependencies" );

  impl_->config.add_parameter( "track_termination_duration_frames",
    "0",
    "Number of frames representing the duration of the temporal window "
    "over which existing tracks will be terminated. This parameter over-rides "
    "da_tracker_process::terminate:missed_count and controls other underlying "
    "dependencies" );

  impl_->config.add_parameter( "back_tracking_duration_frames",
    "0",
    "Number of frames representing the duration of the temporal window "
    "over which back tracking will be performed. This parameter over-rides "
    "back_tracking_super_process::duration_frames and controls other underlying "
    "dependencies" );

}

template< class PixType >
tracking_super_process< PixType >
::~tracking_super_process()
{
  delete impl_;
}

template< class PixType >
config_block
tracking_super_process< PixType >
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  tmp_config.remove( impl_->dependency_config );
  return tmp_config;
}


template< class PixType >
bool
tracking_super_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    // Update parameters for this process
    bool back_tracking_off = blk.get<bool>( "back_tracking_disabled" );
    unsigned init_len = blk.get<unsigned>( "track_init_duration_frames" );
    unsigned terminate_len = blk.get<unsigned>( "track_termination_duration_frames" );
    unsigned back_track_len = blk.get<unsigned>( "back_tracking_duration_frames" );
    impl_->gui_disabled = blk.get<bool>( impl_->proc_gui->name() + ":disabled" );


    impl_->config.update( blk );

    impl_->setup_pipeline( this->pipeline_ );

    // To over-write the values in file
    impl_->config.update( impl_->forced_config );

    // tracker dependencies
    impl_->dependency_config.set( impl_->proc_tracker->name()
                                  + ":terminate:missed_count", terminate_len );

    // Track initialization temporal duration dependencies
    unsigned buffer_len = 1 + vcl_max( init_len, terminate_len );

    if( ! back_tracking_off )
    {
      buffer_len += back_track_len;

      impl_->dependency_config.set( impl_->proc_img2wld_homog_buffer->name()
        + ":length", buffer_len );
      impl_->dependency_config.set( impl_->proc_mods_buffer->name()
        + ":length", buffer_len );
      impl_->dependency_config.set( impl_->proc_fg_objs_buffer->name()
        + ":length", buffer_len );
      impl_->dependency_config.set( impl_->proc_gsd_buffer->name()
        + ":length", buffer_len );
    }
    impl_->dependency_config.set( impl_->proc_track_init->name() + ":delta",
                                  init_len );
    impl_->dependency_config.set( impl_->proc_timestamp_buffer->name()
                                  + ":length", buffer_len );
    impl_->dependency_config.set( impl_->proc_wld2img_homog_buffer->name()
                                  + ":length", buffer_len );
    impl_->dependency_config.set( impl_->proc_image_buffer->name()
                                  + ":length", buffer_len );
    impl_->dependency_config.set( impl_->proc_ts_image_buffer->name()
                                  + ":length", buffer_len );
    impl_->dependency_config.set( impl_->proc_unassgn_mods_buffer->name()
                                  + ":length", buffer_len );

    // multi_feature dependencies
    {
      vcl_string tmp;
      bool multi_features_will_be_used;
      vcl_string multi_feats_name = impl_->proc_multi_features->name();

      // track_init dependencies
      try{
        impl_->config.get( multi_feats_name + ":test_init_filename", tmp );
        multi_features_will_be_used = true;
      }
      catch( unchecked_return_value & ){
        multi_features_will_be_used = false;
      }

      if( multi_features_will_be_used )
      {
        impl_->dependency_config.set( impl_->proc_track_init->name()
          + ":miss_penalty", 100 );
      }

      // histogram dependencies
      try
      {
        impl_->config.get( multi_feats_name + ":test_online_filename" , tmp );
        multi_features_will_be_used = true;
      }
      catch( unchecked_return_value & ){
        multi_features_will_be_used = false;
      }

      double w_color;
      impl_->config.get(  multi_feats_name + ":w_color", w_color );

      if( multi_features_will_be_used && w_color > 0.0 )
      {
        // Turn histogram computation on.
        impl_->dependency_config.set( impl_->proc_histogram->name()
          + ":compute_histogram", true );
      }
      else
      {
        // Turn histogram computation off.
        // Not using multiple_features at all, therefore there is
        // no need to compute image histogram on objects.
        impl_->dependency_config.set( impl_->proc_histogram->name()
          + ":compute_histogram", false );
      }
    }    // multi_feature dependencies

    // Foreground tracking dependencies
    bool fgt_off = true;
    impl_->config.get( impl_->proc_fg_init->name()
      + ":create_fg_model:disabled", fgt_off );

    impl_->dependency_config.set( impl_->proc_tracker->name()
      + ":fg_tracking:enabled", !fgt_off );
    impl_->dependency_config.set( impl_->proc_fg_update->name()
      + ":update_fg_model:disabled", fgt_off );

    // Back-tracking dependencies
    if( !back_tracking_off )
    {
      vcl_string btsp = impl_->proc_back_tracking_sp->name();

      impl_->dependency_config.set( btsp + ":disabled", back_tracking_off );
      impl_->dependency_config.set( btsp + ":track_init_duration_frames", init_len );
      impl_->dependency_config.set( btsp + ":duration_frames", back_track_len );

      // Important: Make sure that impl_->config is updated \w dependency_config
      // at this point before copying following subblocks.
      impl_->config.update( impl_->dependency_config );

      config_block bt_subset;
      bt_subset.add_subblock( impl_->config.subblock( impl_->proc_tracker->name() ),
        btsp + ":back_tracker" );
      bt_subset.add_subblock( impl_->config.subblock( impl_->proc_fg_init->name() ),
        btsp + ":fg_init" );
      bt_subset.add_subblock( impl_->config.subblock( impl_->proc_fg_update->name() ),
        btsp + ":fg_update" );
      bt_subset.add_subblock( impl_->config.subblock( impl_->proc_tracker_initializer->name() ),
        btsp + ":tracker_initializer" );
      impl_->dependency_config.update( bt_subset );
    } // Back-tracking dependencies

    // AMHI dependencies
    {
      bool amhi_on;
      impl_->config.get( impl_->proc_tracker->name() + ":amhi:enabled", amhi_on );
      impl_->dependency_config.set( impl_->proc_amhi_init->name() + ":disabled", !amhi_on );
      impl_->dependency_config.set( impl_->proc_fg_init->name() + ":create_fg_model:ssd:amhi_mode", amhi_on );
      impl_->dependency_config.set( impl_->proc_amhi_image->name() + ":disabled",
        impl_->gui_disabled || !amhi_on );
    }

    // Update (the pipeline) config with all the dependencies of this
    // super_process.
    impl_->config.update( impl_->dependency_config );

    // GUI is a special case, different from dependency and forced
    bool bt_gui_disabled;
    impl_->config.get( impl_->proc_back_tracking_sp->name()
                       + ":bt_gui:disabled", bt_gui_disabled );

    if( ! impl_->gui_disabled && ! bt_gui_disabled )
    {
      impl_->config.set( impl_->proc_back_tracking_sp->name()
                         + ":bt_gui:disabled", true );

      log_warning( this->name() << ": Forcing "<<  impl_->proc_back_tracking_sp->name()
                   << ":bt_gui:disabled = true" << "\n" );
    }

    // Finally use the impl_->config to set the parameters in the pipeline.
    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw unchecked_return_value( " unable to set pipeline parameters." );
    }
  }
  catch( unchecked_return_value& e)
  {
    log_error( this->name() << ": couldn't set parameters: "
                            << e.what() <<"\n" );

    return false;
  }

  return true;
}


template< class PixType >
bool
tracking_super_process< PixType >
::initialize()
{
  impl_->proc_gui->set_tracker_gate_sigma( impl_->proc_tracker->gate_sigma() );
  plane_to_image_homography H;
  H.set_identity(false);
  this->set_wld_to_src_homography( H );
  return this->pipeline_->initialize();
}


template< class PixType >
process::step_status
tracking_super_process< PixType >
::step2()
{
  log_info (name() << ": input timestamp " << impl_->current_timestamp << "\n");

  // FIXME: Locating the code here is a hack; albeit a minor one...
  process::step_status s = pipeline_->execute();
  if ( s != FAILURE )
  {
    if ( (! impl_->in_wld2utm_homog.is_valid() ) ||
         impl_->in_wld2src_homog.get_transform().get_matrix().is_zero() )
    {
      // no inputs, invalidate output
      impl_->out_src2utm_homog = image_to_utm_homography();
    }
    else
    {
      // compute src->utm
      impl_->out_src2utm_homog =
        impl_->in_wld2utm_homog * impl_->in_wld2src_homog.get_inverse();
    }
  }

  return s;
}


// ================================================================
//                      inputs
// ================================================================

template< class PixType >
void
tracking_super_process< PixType >
::set_source_objects( vcl_vector< image_object_sptr > const& objs )
{
  impl_->proc_histogram->set_source_objects( objs );
  impl_->proc_gui->set_filter1_objects( objs );
}


template< class PixType >
void
tracking_super_process< PixType >
::set_source_image( vil_image_view<PixType> const& image )
{
  impl_->current_image = image;

  impl_->proc_image_copy->set_source_image( impl_->current_image );
  impl_->proc_gui->set_image( impl_->current_image );
  impl_->proc_state_to_image->set_source_image( impl_->current_image );
}


template< class PixType >
void
tracking_super_process< PixType >
::set_timestamp( timestamp const& ts )
{
  impl_->current_timestamp = ts;

  impl_->proc_amhi_image->set_timestamp( impl_->current_timestamp );
  impl_->proc_tracker->set_timestamp( impl_->current_timestamp );
  impl_->proc_add_img_info_to_start->set_timestamp( impl_->current_timestamp );
  impl_->proc_linker->set_timestamp( impl_->current_timestamp );
  impl_->proc_add_img_info_to_end->set_timestamp( impl_->current_timestamp );
  impl_->proc_timestamp_buffer->set_next_datum( impl_->current_timestamp );
  impl_->proc_gui->set_source_timestamp( impl_->current_timestamp );
  impl_->proc_gui_writer->set_timestamp( impl_->current_timestamp );
  impl_->proc_fg_objs_buffer->set_timestamp( impl_->current_timestamp );
  impl_->proc_ts_image_buffer->set_next_key( impl_->current_timestamp );
  impl_->proc_gsd_buffer->set_next_key( impl_->current_timestamp );
  impl_->proc_fg_init->set_timestamp( impl_->current_timestamp );
  impl_->proc_fg_update->set_timestamp( impl_->current_timestamp );
  impl_->proc_state_to_image->set_timestamp( impl_->current_timestamp );
}


template< class PixType >
void
tracking_super_process< PixType >
::set_src_to_wld_homography( image_to_plane_homography const& H )
{
  impl_->in_src2wld_homog = H;

  impl_->proc_tracker->set_image2world_homography( impl_->in_src2wld_homog.get_transform() );
  impl_->proc_img2wld_homog_buffer->set_next_datum( impl_->in_src2wld_homog.get_transform() );
  impl_->proc_state_to_image->set_src2wld_homography( impl_->in_src2wld_homog.get_transform() );
}


template< class PixType >
void
tracking_super_process< PixType >
::set_wld_to_src_homography( plane_to_image_homography const& H )
{
  impl_->in_wld2src_homog = H;

  impl_->proc_tracker->set_world2image_homography( impl_->in_wld2src_homog.get_transform() );
  impl_->proc_wld2img_homog_buffer->set_next_datum( impl_->in_wld2src_homog.get_transform() );
  impl_->proc_gui->set_world_homog( impl_->in_wld2src_homog.get_transform() );
  impl_->proc_state_to_image->set_wld2src_homography( impl_->in_wld2src_homog.get_transform() );
}


template< class PixType >
void
tracking_super_process< PixType >
::set_src_to_ref_homography( image_to_image_homography const& H )
{
  //+ should attempt to set new-reference attribute.
  impl_->in_src2ref_homog = H;
}


template< class PixType >
void
tracking_super_process< PixType >
::set_multi_features_dir( vcl_string const & dir )
{
  impl_->proc_multi_features->set_configuration_directory( dir );
}


template< class PixType >
void
tracking_super_process< PixType >
::set_wld_to_utm_homography( plane_to_utm_homography const& H )
{
  impl_->in_wld2utm_homog = H;
  impl_->proc_add_latlon->set_wld_to_utm_homography( impl_->in_wld2utm_homog );
}


template< class PixType >
void
tracking_super_process< PixType >
::set_guis( process_smart_pointer< gui_process > ft_gui,
            process_smart_pointer< gui_process > bt_gui )
{
  impl_->gui = ft_gui;
  impl_->proc_back_tracking_sp->set_gui( bt_gui );
}

template< class PixType >
void
tracking_super_process< PixType >
::set_fg_image( vil_image_view< bool > const& img )
{
  impl_->fg_image = img;
  impl_->proc_gui->set_fg_image( impl_->fg_image );
}

template< class PixType >
void
tracking_super_process< PixType >
::set_diff_image( vil_image_view< float > const& img )
{
  impl_->diff_image = img;
  impl_->proc_gui->set_diff_image( impl_->diff_image );
}

template< class PixType >
void
tracking_super_process< PixType >
::set_morph_image( vil_image_view< bool > const& img )
{
  impl_->morph_image = img;
  impl_->proc_gui->set_morph_image( impl_->morph_image );
}

template< class PixType >
void
tracking_super_process< PixType >
::set_unfiltered_objects( vcl_vector< image_object_sptr > const& objs )
{
  impl_->unfiltered_objs = objs;
  impl_->proc_gui->set_objects( impl_->unfiltered_objs );
}

template< class PixType >
void
tracking_super_process< PixType >
::set_world_units_per_pixel( double gsd )
{
  impl_->world_units_per_pixel = gsd;
  impl_->proc_gui->set_world_units_per_pixel( gsd );
  impl_->proc_tracker->set_world_units_per_pixel( gsd );
  impl_->proc_gsd_buffer->set_next_datum( impl_->world_units_per_pixel );
}

// ================================================================
//    outputs
// ================================================================

template< class PixType >
vcl_vector< track_sptr > const &
tracking_super_process< PixType >
::linked_tracks() const
{
  return impl_->pad_linked_tracks->value();
}


template< class PixType >
vcl_vector< track_sptr > const&
tracking_super_process< PixType >
::init_tracks() const
{
  return impl_->proc_trk_filter1->tracks();
}


template< class PixType >
vcl_vector< track_sptr > const&
tracking_super_process< PixType >
::filtered_tracks() const
{
  return impl_->pad_filtered_tracks->value();
}


template< class PixType >
vcl_vector< track_sptr > const&
tracking_super_process< PixType >
::active_tracks() const
{
  impl_->out_active_tracks.clear();

  // make a medium deep copy of output
  // We are providing each reader (consumer) with an individual copy
  // because we do not want the consumers to share the same vector
  // of tracks.
  BOOST_FOREACH (track_sptr ptr, impl_->proc_tracker->active_tracks())
  {
    impl_->out_active_tracks.push_back(ptr->shallow_clone());
  }
  return impl_->out_active_tracks;
}


template< class PixType >
vcl_vector< da_so_tracker_sptr > const&
tracking_super_process< PixType >
::active_trackers() const
{
  return impl_->proc_tracker->active_trackers();
}


template< class PixType >
vil_image_view<PixType> const&
tracking_super_process< PixType >
::amhi_image() const
{
  return impl_->proc_amhi_image->amhi_image();
}


template< class PixType >
double
tracking_super_process< PixType >
::tracker_gate_sigma() const
{
  return impl_->proc_tracker->gate_sigma();
}


template< class PixType >
vidtk::timestamp const &
tracking_super_process< PixType >
::out_timestamp() const
{
  // retrieve last timestamp
  return impl_->current_timestamp;
}


template< class PixType >
image_to_image_homography const&
tracking_super_process< PixType >
::src_to_ref_homography() const
{
  return impl_->in_src2ref_homog;
}

template< typename PixType >
image_to_utm_homography const&
tracking_super_process< PixType >
::src_to_utm_homography() const
{
  return impl_->out_src2utm_homog;
}


template< typename PixType >
double
tracking_super_process< PixType >
::world_units_per_pixel() const
{
  return impl_->world_units_per_pixel;
}

} // end namespace vidtk
