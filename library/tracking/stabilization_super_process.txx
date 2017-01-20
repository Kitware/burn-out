/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "stabilization_super_process.h"

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <kwklt/klt_pyramid_process.h>

#include <tracking/homography_super_process.h>
#include <tracking_data/shot_break_flags.h>
#include <tracking/shot_stitching_process.h>
#include <tracking/stab_pass_thru_process.h>
#include <tracking/stab_homography_source.h>

#ifdef WITH_MAPTK_ENABLED
#include <tracking/maptk_tracking_process.h>
#endif

#include <object_detectors/metadata_mask_super_process.h>

#include <utilities/config_block.h>
#include <utilities/videoname_prefix.h>
#include <utilities/homography_reader_process.h>
#include <utilities/project_vidtk_homography_process.h>
#include <utilities/transform_timestamp_process.h>
#include <utilities/tagged_data_writer_process.h>
#include <utilities/homography_writer_process.h>
#include <utilities/metadata_f2f_homography_process.h>

#include <video_io/image_list_writer_process.h>
#include <video_transforms/greyscale_process.h>
#include <video_transforms/rescale_image_process.h>

#include <logger/logger.h>

#include <boost/lexical_cast.hpp>

namespace vidtk
{

VIDTK_LOGGER ("stabilization_super_process");

// incomplete type
class maptk_tracking_process;

template< class PixType>
class stabilization_super_process_impl
{
public:
  typedef vidtk::super_process_pad_impl<vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl<timestamp> timestamp_pad;
  typedef vidtk::super_process_pad_impl< video_metadata > metadata_pad;
  typedef vidtk::super_process_pad_impl< std::vector< video_metadata > > metadata_vec_pad;
  typedef vidtk::super_process_pad_impl< image_to_image_homography > homog_pad;
  typedef vidtk::super_process_pad_impl< std::vector< klt_track_ptr > > klt_track_pad;
  typedef vidtk::super_process_pad_impl< shot_break_flags > shot_break_flags_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > mask_pad;

  // Pipeline processes
  process_smart_pointer< transform_timestamp_process<PixType> > proc_timestamper;
  process_smart_pointer< homography_reader_process > proc_homog_reader;
  process_smart_pointer< rescale_image_process<PixType> > proc_rescale;
  process_smart_pointer< rescale_image_process<bool> > proc_mask_rescale;
  process_smart_pointer< metadata_f2f_homography_process > proc_homog_from_metadata;
  process_smart_pointer< greyscale_process<PixType> > proc_convert_to_grey;
  process_smart_pointer< klt_pyramid_process<PixType> > proc_klt_pyramid;
  process_smart_pointer< homography_super_process > proc_homography_sp;
  process_smart_pointer< metadata_mask_super_process< PixType > > proc_metadata_mask;
  process_smart_pointer< shot_stitching_process< PixType > > proc_shot_stitcher;
  process_smart_pointer< project_vidtk_homography_process<timestamp, timestamp> > proc_project_homography;
  process_smart_pointer< tagged_data_writer_process > proc_data_write;
  process_smart_pointer< homography_writer_process > proc_homog_writer;
  process_smart_pointer< stab_pass_thru_process<PixType> > proc_pass;
  process_smart_pointer< stab_homography_source > proc_homog_src;
  process_smart_pointer< image_list_writer_process<PixType> > proc_prestab_image_writer;
#ifdef WITH_MAPTK_ENABLED
  process_smart_pointer< maptk_tracking_process > proc_maptk_tracking;
#endif

  // Input Pads (dummy processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< mask_pad > pad_source_mask;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< metadata_pad > pad_source_metadata;

  // Output Pads (dummy processes)
  process_smart_pointer< timestamp_pad > pad_output_timestamp;
  process_smart_pointer< image_pad > pad_output_image;
  process_smart_pointer< metadata_vec_pad > pad_output_metadata;
  process_smart_pointer< homog_pad > pad_output_src2ref_homog;
  process_smart_pointer< klt_track_pad > pad_output_active_tracks;
  process_smart_pointer< shot_break_flags_pad > pad_output_shot_break_flags;
  process_smart_pointer< mask_pad > pad_output_mask;

  // Configuration parameters
  config_block config;
  config_block default_config;
  enum sp_mode{
    VIDTK_LOAD,
    VIDTK_COMPUTE,
    VIDTK_COMPUTE_MAPTK,
    VIDTK_DISABLED
  };
  sp_mode mode;

  enum tk_mode { VIDTK_STAB_TRACK_KLT, VIDTK_STAB_TRACK_GPU };
  tk_mode tracker_type;

  bool tdw_enable;

  // CTOR
  stabilization_super_process_impl()
  :
    proc_timestamper( NULL ),
    proc_homog_reader( NULL ),
    proc_rescale( NULL ),
    proc_homog_from_metadata( NULL ),
    proc_convert_to_grey( NULL ),
    proc_klt_pyramid( NULL ),
    proc_homography_sp( NULL ),
    proc_metadata_mask( NULL ),
    proc_shot_stitcher( NULL ),
    proc_project_homography( NULL ),
    proc_data_write( NULL ),
    proc_homog_writer( NULL ),
    proc_pass( NULL ),
    proc_homog_src( NULL ),
    proc_prestab_image_writer( NULL ),
#ifdef WITH_MAPTK_ENABLED
    proc_maptk_tracking( NULL ),
#endif

    // input pads
    pad_source_image( NULL ),
    pad_source_mask( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_metadata( NULL ),

    // output pads
    pad_output_timestamp( NULL ),
    pad_output_image( NULL ),
    pad_output_metadata( NULL ),
    pad_output_src2ref_homog( NULL ),
    pad_output_active_tracks( NULL ),
    pad_output_shot_break_flags( NULL ),
    pad_output_mask( NULL ),
    config(),
    default_config(),
    mode( VIDTK_DISABLED ),
    tdw_enable( false )
  {
  }


// ----------------------------------------------------------------
/** Create process objects and create config block for the pipeline.
 *
 *
 */
void create_process_configs()
  {
    pad_source_image = new image_pad("source_image");
    pad_source_mask = new mask_pad("source_mask");
    pad_source_timestamp = new timestamp_pad("source_timestamp");
    pad_source_metadata = new metadata_pad("source_metadata");

    pad_output_timestamp = new timestamp_pad("output_timestamp");
    pad_output_image = new image_pad("output_image");
    pad_output_metadata = new metadata_vec_pad("output_metadata");
    pad_output_src2ref_homog = new homog_pad("output_src2ref_homog");
    pad_output_active_tracks = new klt_track_pad("output_active_tracks");
    pad_output_shot_break_flags = new shot_break_flags_pad("output_shot_break_flags");
    pad_output_mask = new mask_pad("output_mask");

    proc_timestamper = new vidtk::transform_timestamp_process<PixType>( "timestamper" );
    config.add_subblock( proc_timestamper->params(), proc_timestamper->name() );

    proc_homog_reader = new homography_reader_process( "homog_reader" );
    config.add_subblock( proc_homog_reader->params(),
                         proc_homog_reader->name() );

    proc_rescale = new rescale_image_process<PixType>( "rescale" );
    config.add_subblock( proc_rescale->params(),
                         proc_rescale->name() );

    proc_mask_rescale = new rescale_image_process<bool>( "mask_rescale" );
    config.add_subblock( proc_mask_rescale->params(),
                         proc_mask_rescale->name() );

    proc_homog_from_metadata = new metadata_f2f_homography_process( "homog_georeg" );
    config.add_subblock( proc_homog_from_metadata->params(),
                         proc_homog_from_metadata->name() );

    proc_convert_to_grey = new greyscale_process<PixType>( "rgb_to_grey" );
    config.add_subblock( proc_convert_to_grey->params(),
                         proc_convert_to_grey->name() );

    proc_klt_pyramid = new klt_pyramid_process<PixType>( "klt_pyramid" );
    config.add_subblock( proc_klt_pyramid->params(),
                         proc_klt_pyramid->name() );

    proc_homography_sp = new homography_super_process( "homog_sp" );
    config.add_subblock( proc_homography_sp->params(),
                         proc_homography_sp->name() );

    proc_metadata_mask = new metadata_mask_super_process< PixType >( "md_mask_sp" );
    config.add_subblock( proc_metadata_mask->params(),
                         proc_metadata_mask->name() );

    proc_shot_stitcher = new shot_stitching_process< PixType >( "shot_stitcher" );
    config.add_subblock( proc_shot_stitcher->params(),
                         proc_shot_stitcher->name() );

    proc_project_homography = new project_vidtk_homography_process<timestamp, timestamp>( "rescale_homog" );
    config.add_subblock( proc_project_homography->params(),
                         proc_project_homography->name() );

    proc_data_write = new tagged_data_writer_process( "tagged_data_writer" );
    config.add_subblock( proc_data_write->params(),
                         proc_data_write->name() );

    proc_homog_writer = new homography_writer_process( "homog_writer" );
    config.add_subblock( proc_homog_writer->params(),
                         proc_homog_writer->name() );

    proc_pass = new stab_pass_thru_process<PixType>( "stab_pass_thru" );
    config.add_subblock( proc_pass->params(),
                         proc_pass->name() );

    proc_homog_src = new stab_homography_source( "stab_homog_source" );
    config.add_subblock( proc_homog_src->params(),
                         proc_homog_src->name() );

    // Creating prestab_image_writer that is disabled by default
    proc_prestab_image_writer = new image_list_writer_process<PixType>( "prestab_image_writer" );
    config.add_subblock( proc_prestab_image_writer->params(), proc_prestab_image_writer->name() );

#ifdef WITH_MAPTK_ENABLED
    // Creating MAP-Tk feature tracking process
    proc_maptk_tracking = new maptk_tracking_process( "maptk_feature_tracker" );
    config.add_subblock( proc_maptk_tracking->params(), proc_maptk_tracking->name() );
    LOG_DEBUG( "stab_sp: Constructed proc_maptk_tracking" );
#endif

    // Override process defaults with super-process defaults
    default_config.add_parameter( proc_prestab_image_writer->name() + ":disabled", "true",
                                  "Disabled pre-stabilization image output or not." );

    config.update( default_config );
  }


// ----------------------------------------------------------------
/** Create pipeline for disbled mode.
 *
 * The pipeline created just passes the inputs to the outputs.
 */
  template <class PIPELINE>
  void setup_pipeline_disabled_mode( PIPELINE * p )
  {
    // Add source pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_metadata );

    p->add( proc_timestamper );
    p->connect( pad_source_image->value_port(),       proc_timestamper->set_source_image_port() );
    p->connect( pad_source_timestamp->value_port(),   proc_timestamper->set_source_timestamp_port() );
    p->connect( pad_source_metadata->value_port(),    proc_timestamper->set_source_metadata_port() );

    // create pass-thru process - not strictly needed in this case,
    // but simplifies connecting the data writer.
    p->add( proc_pass );
    p->connect( proc_timestamper->image_port(),            proc_pass->set_input_image_port() );
    p->connect( proc_timestamper->timestamp_port(),        proc_pass->set_input_timestamp_port() );
    p->connect( proc_timestamper->metadata_vec_port(),     proc_pass->set_input_metadata_vector_port() );

    // create process and connections to supply a reasonable s2r homog
    p->add( proc_homog_src );
    p->connect( proc_timestamper->timestamp_port(),              proc_homog_src->input_timestamp_port() );
    p->connect( proc_homog_src->output_src_to_ref_homography_port(),  proc_pass->set_input_homography_port() );

    p->add( pad_output_image );
    p->add( pad_output_timestamp );
    p->add( pad_output_metadata );
    p->add( pad_output_src2ref_homog );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),           pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),       pad_output_timestamp->set_value_port() );
    p->connect( proc_pass->get_output_metadata_vector_port(), pad_output_metadata->set_value_port() );
    p->connect( proc_pass->get_output_homography_port(),      pad_output_src2ref_homog->set_value_port() );
  }


// ----------------------------------------------------------------
/** Create pipeline for load mode.
 *
 *
 */
  template <class PIPELINE>
  void setup_pipeline_load_mode( PIPELINE * p )
  {
    // Add source pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_metadata );

    p->add( proc_timestamper );
    p->connect( pad_source_image->value_port(),       proc_timestamper->set_source_image_port() );
    p->connect( pad_source_timestamp->value_port(),   proc_timestamper->set_source_timestamp_port() );
    p->connect( pad_source_metadata->value_port(),    proc_timestamper->set_source_metadata_port() );

    p->add( proc_prestab_image_writer );
    p->connect( proc_timestamper->image_port(), proc_prestab_image_writer->set_image_port() );
    p->connect( proc_timestamper->timestamp_port(), proc_prestab_image_writer->set_timestamp_port() );

    // Add homog reader to create the src2ref homog
    p->add( proc_homog_reader );
    p->connect( proc_timestamper->timestamp_port(),   proc_homog_reader->set_timestamp_port() );

    p->add( proc_pass );
    p->connect( proc_timestamper->image_port(),            proc_pass->set_input_image_port() );
    p->connect( proc_timestamper->timestamp_port(),        proc_pass->set_input_timestamp_port() );
    p->connect( proc_timestamper->metadata_vec_port(),     proc_pass->set_input_metadata_vector_port() );
    p->connect( proc_homog_reader->image_to_world_vidtk_homography_image_port(), proc_pass->set_input_homography_port() );

    p->add( pad_output_image );
    p->add( pad_output_timestamp );
    p->add( pad_output_metadata );
    p->add( pad_output_src2ref_homog );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),           pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),       pad_output_timestamp->set_value_port() );
    p->connect( proc_pass->get_output_metadata_vector_port(), pad_output_metadata->set_value_port() );
    p->connect( proc_pass->get_output_homography_port(),      pad_output_src2ref_homog->set_value_port() );
  }


// ----------------------------------------------------------------
/** Create pipeline for compute mode.
 *
 *
 */
  template <class PIPELINE>
  void setup_pipeline_compute_mode( PIPELINE * p )
  {
    // Add source pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_metadata );

    p->add( proc_timestamper );
    p->connect( pad_source_image->value_port(),       proc_timestamper->set_source_image_port() );
    p->connect( pad_source_timestamp->value_port(),   proc_timestamper->set_source_timestamp_port() );
    p->connect( pad_source_metadata->value_port(),    proc_timestamper->set_source_metadata_port() );

    p->add( proc_prestab_image_writer );
    p->connect( proc_timestamper->image_port(), proc_prestab_image_writer->set_image_port() );
    p->connect( proc_timestamper->timestamp_port(), proc_prestab_image_writer->set_timestamp_port() );

    p->add( proc_rescale );
    p->connect( proc_timestamper->image_port(),       proc_rescale->set_image_port() );

    p->add( proc_convert_to_grey );
    p->connect( proc_rescale->image_port(),           proc_convert_to_grey->set_image_port() );

    p->add( proc_homography_sp );
    p->connect( proc_timestamper->timestamp_port(),   proc_homography_sp->set_timestamp_port() );

    p->add( proc_homog_from_metadata );
    p->connect( proc_convert_to_grey->copied_image_port(), proc_homog_from_metadata->set_image_port() );
    p->connect( proc_timestamper->metadata_vec_port(),     proc_homog_from_metadata->set_metadata_v_port() );

    if( tracker_type == VIDTK_STAB_TRACK_KLT )
    {
      p->add( proc_klt_pyramid );
      p->connect( proc_convert_to_grey->copied_image_port(),    proc_klt_pyramid->set_image_port() );

      p->connect( proc_homog_from_metadata->h_prev_2_cur_port(), proc_homography_sp->set_homog_predict_port() );

      p->connect( proc_klt_pyramid->image_pyramid_port(),       proc_homography_sp->set_image_pyramid_port() );
      p->connect( proc_klt_pyramid->image_pyramid_gradx_port(), proc_homography_sp->set_image_pyramid_gradx_port() );
      p->connect( proc_klt_pyramid->image_pyramid_grady_port(), proc_homography_sp->set_image_pyramid_grady_port() );
    }
    else if( tracker_type == VIDTK_STAB_TRACK_GPU )
    {
      p->connect( proc_convert_to_grey->copied_image_port(),    proc_homography_sp->set_image_port()  );
    }

    p->add( proc_shot_stitcher );
    p->connect( proc_homography_sp->active_tracks_port(),                proc_shot_stitcher->set_input_klt_tracks_port() );
    p->connect( proc_timestamper->image_port(),                          proc_shot_stitcher->set_input_source_image_port() );
    p->connect( proc_convert_to_grey->copied_image_port(),               proc_shot_stitcher->set_input_rescaled_image_port() );
    p->connect( proc_timestamper->timestamp_port(),                      proc_shot_stitcher->set_input_timestamp_port() );
    p->connect( proc_homography_sp->src_to_ref_homography_port(),        proc_shot_stitcher->set_input_src2ref_h_port() );
    p->connect( proc_homography_sp->get_output_shot_break_flags_port(),  proc_shot_stitcher->set_input_shot_break_flags_port() );

    p->add( proc_pass );

    // Test for masking support enabled
    bool masking_enabled = config.get<bool>( "masking_enabled" );

    if( masking_enabled )
    {
      std::string masking_source = config.get< std::string >( "masking_source" );

      p->add( proc_mask_rescale );

      if( masking_source == "internal" )
      {
        p->add( proc_metadata_mask );

        p->connect( pad_source_image->value_port(),               proc_metadata_mask->set_image_port() );
        p->connect( proc_timestamper->timestamp_port(),           proc_metadata_mask->set_timestamp_port() );
        p->connect( proc_timestamper->metadata_vec_port(),        proc_metadata_mask->set_metadata_vector_port() );

        p->connect( proc_metadata_mask->mask_port(),              proc_mask_rescale->set_image_port() );

        p->connect( proc_mask_rescale->image_port(),              proc_homography_sp->set_mask_port() );
        p->connect( proc_mask_rescale->image_port(),              proc_shot_stitcher->set_input_metadata_mask_port() );
        p->connect( proc_metadata_mask->metadata_vector_port(),   proc_shot_stitcher->set_input_metadata_vector_port() );
      }
      else
      {
        p->add( pad_source_mask );

        p->connect( pad_source_mask->value_port(),                proc_mask_rescale->set_image_port() );

        p->connect( proc_mask_rescale->image_port(),              proc_homography_sp->set_mask_port() );
        p->connect( proc_mask_rescale->image_port(),              proc_shot_stitcher->set_input_metadata_mask_port() );
        p->connect( proc_timestamper->metadata_vec_port(),        proc_shot_stitcher->set_input_metadata_vector_port() );
      }

      p->add( pad_output_mask );

      p->connect( proc_shot_stitcher->output_metadata_mask_port(),  proc_pass->set_input_mask_port() );
      p->connect( proc_pass->get_output_mask_port(),                pad_output_mask->set_value_port() );
    }
    else
    {
      p->connect( proc_timestamper->metadata_vec_port(),           proc_shot_stitcher->set_input_metadata_vector_port() );
    }

    p->add( proc_project_homography );
    p->connect( proc_shot_stitcher->output_src2ref_h_port(),        proc_project_homography->set_source_homography_port() );

    bool writer_disabled = config.get<bool>( "homog_writer:disabled" );

    if( !writer_disabled )
    {
      p->add( proc_homog_writer );
      p->connect( proc_project_homography->homography_port(),  proc_homog_writer->set_source_vidtk_homography_port() );
      p->connect( proc_shot_stitcher->output_timestamp_port(), proc_homog_writer->set_source_timestamp_port() );
    }

    // Connect to pass-thru process
    p->connect( proc_shot_stitcher->output_source_image_port(),     proc_pass->set_input_image_port() );
    p->connect( proc_shot_stitcher->output_klt_tracks_port(),       proc_pass->set_input_klt_tracks_port() );
    p->connect( proc_shot_stitcher->output_metadata_vector_port(),  proc_pass->set_input_metadata_vector_port() );
    p->connect( proc_shot_stitcher->output_shot_break_flags_port(), proc_pass->set_input_shot_break_flags_port() );
    p->connect( proc_project_homography->homography_port(),         proc_pass->set_input_homography_port() );
    p->connect( proc_shot_stitcher->output_timestamp_port(),        proc_pass->set_input_timestamp_port() );

    // Connect to output pads
    p->add( pad_output_image );
    p->add( pad_output_active_tracks );
    p->add( pad_output_metadata );
    p->add( pad_output_shot_break_flags );
    p->add( pad_output_src2ref_homog );
    p->add( pad_output_timestamp );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),            pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_klt_tracks_port(),       pad_output_active_tracks->set_value_port() );
    p->connect( proc_pass->get_output_metadata_vector_port(),  pad_output_metadata->set_value_port() );
    p->connect( proc_pass->get_output_shot_break_flags_port(), pad_output_shot_break_flags->set_value_port() );
    p->connect( proc_pass->get_output_homography_port(),       pad_output_src2ref_homog->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),        pad_output_timestamp->set_value_port() );
  }


  template< typename Pipeline >
  void helper_image( Pipeline* p, vxl_byte /*dont care*/ )
  {
    p->connect( proc_pass->get_output_image_port(),           proc_data_write->set_input_image_8_port() );
    proc_data_write->set_image_8_connected (true);
  }

  template< typename Pipeline >
  void helper_image( Pipeline* p, vxl_uint_16 /*dont care*/ )
  {
    p->connect( proc_pass->get_output_image_port(),           proc_data_write->set_input_image_16_port() );
    proc_data_write->set_image_16_connected (true);
  }

// ----------------------------------------------------------------
/**
 * Compute mode using MAP-Tk f2f stabilization
 *
 * Similar to the compute mode above, except the KLT components are replaced
 * with a process that uses MAP-Tk for f-to-f tracking and homography
 * generation.
 */
  template <class PIPELINE>
  void setup_pipeline_compute_maptk_mode( PIPELINE *p )
  {
#ifdef WITH_MAPTK_ENABLED
    LOG_DEBUG( "stab_sp: Setting up MAP-Tk computation pipeline" );

    // Add source pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_metadata );

    p->add( proc_timestamper );
    p->connect( pad_source_image->value_port(),       proc_timestamper->set_source_image_port() );
    p->connect( pad_source_timestamp->value_port(),   proc_timestamper->set_source_timestamp_port() );
    p->connect( pad_source_metadata->value_port(),    proc_timestamper->set_source_metadata_port() );

    p->add( proc_prestab_image_writer );
    p->connect( proc_timestamper->image_port(), proc_prestab_image_writer->set_image_port() );
    p->connect( proc_timestamper->timestamp_port(), proc_prestab_image_writer->set_timestamp_port() );

    p->add( proc_rescale );
    p->connect( proc_timestamper->image_port(), proc_rescale->set_image_port() );

    p->add( proc_convert_to_grey );
    p->connect( proc_rescale->image_port(), proc_convert_to_grey->set_image_port() );

    p->add( proc_maptk_tracking );
    p->connect( proc_timestamper->timestamp_port(),        proc_maptk_tracking->set_timestamp_port() );
    p->connect( proc_convert_to_grey->copied_image_port(), proc_maptk_tracking->set_image_port() );

    p->add( proc_shot_stitcher );
    p->connect( proc_maptk_tracking->active_tracks_port(),                proc_shot_stitcher->set_input_klt_tracks_port() );
    p->connect( proc_timestamper->image_port(),                           proc_shot_stitcher->set_input_source_image_port() );
    p->connect( proc_convert_to_grey->copied_image_port(),                proc_shot_stitcher->set_input_rescaled_image_port() );
    p->connect( proc_timestamper->timestamp_port(),                       proc_shot_stitcher->set_input_timestamp_port() );
    p->connect( proc_maptk_tracking->src_to_ref_homography_port(),        proc_shot_stitcher->set_input_src2ref_h_port() );
    p->connect( proc_maptk_tracking->get_output_shot_break_flags_port(),  proc_shot_stitcher->set_input_shot_break_flags_port() );

    p->add( proc_pass );

    // Test for masking support enabled
    bool masking_enabled = config.get<bool>( "masking_enabled" );
    if( masking_enabled )
    {
      std::string masking_source = config.get< std::string >( "masking_source" );

      p->add( proc_mask_rescale );

      if( masking_source == "internal" )
      {
        p->add( proc_metadata_mask );

        p->connect( pad_source_image->value_port(),               proc_metadata_mask->set_image_port() );
        p->connect( proc_timestamper->timestamp_port(),           proc_metadata_mask->set_timestamp_port() );
        p->connect( proc_timestamper->metadata_vec_port(),        proc_metadata_mask->set_metadata_vector_port() );

        p->connect( proc_metadata_mask->mask_port(),              proc_mask_rescale->set_image_port() );

        p->connect( proc_mask_rescale->image_port(),              proc_maptk_tracking->set_mask_port() );
        p->connect( proc_mask_rescale->image_port(),              proc_shot_stitcher->set_input_metadata_mask_port() );
        p->connect( proc_metadata_mask->metadata_vector_port(),   proc_shot_stitcher->set_input_metadata_vector_port() );
      }
      else
      {
        p->add( pad_source_mask );

        p->connect( pad_source_mask->value_port(),                proc_mask_rescale->set_image_port() );

        p->connect( proc_mask_rescale->image_port(),              proc_maptk_tracking->set_mask_port() );
        p->connect( proc_mask_rescale->image_port(),              proc_shot_stitcher->set_input_metadata_mask_port() );
        p->connect( proc_timestamper->metadata_vec_port(),        proc_shot_stitcher->set_input_metadata_vector_port() );
      }

      p->add( pad_output_mask );

      p->connect( proc_shot_stitcher->output_metadata_mask_port(),  proc_pass->set_input_mask_port() );
      p->connect( proc_pass->get_output_mask_port(),                pad_output_mask->set_value_port() );
    }
    else
    {
      p->connect( proc_timestamper->metadata_vec_port(),            proc_shot_stitcher->set_input_metadata_vector_port() );
    }

    p->add( proc_project_homography );
    p->connect( proc_shot_stitcher->output_src2ref_h_port(),        proc_project_homography->set_source_homography_port() );

    bool writer_disabled = config.get<bool>( "homog_writer:disabled" );
    if (!writer_disabled)
    {
      p->add( proc_homog_writer );
      p->connect( proc_project_homography->homography_port(),
                  proc_homog_writer->set_source_vidtk_homography_port() );
      p->connect( proc_shot_stitcher->output_timestamp_port(),
                  proc_homog_writer->set_source_timestamp_port() );
    }

    // connect to pass-thru process
    p->connect( proc_shot_stitcher->output_source_image_port(),     proc_pass->set_input_image_port() );
    p->connect( proc_shot_stitcher->output_klt_tracks_port(),       proc_pass->set_input_klt_tracks_port() );
    p->connect( proc_shot_stitcher->output_metadata_vector_port(),  proc_pass->set_input_metadata_vector_port() );
    p->connect( proc_shot_stitcher->output_shot_break_flags_port(), proc_pass->set_input_shot_break_flags_port() );
    p->connect( proc_project_homography->homography_port(),         proc_pass->set_input_homography_port() );
    p->connect( proc_shot_stitcher->output_timestamp_port(),        proc_pass->set_input_timestamp_port() );

    // Connect to output pads
    p->add( pad_output_image );
    p->add( pad_output_active_tracks );
    p->add( pad_output_metadata );
    p->add( pad_output_shot_break_flags );
    p->add( pad_output_src2ref_homog );
    p->add( pad_output_timestamp );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),            pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_klt_tracks_port(),       pad_output_active_tracks->set_value_port() );
    p->connect( proc_pass->get_output_metadata_vector_port(),  pad_output_metadata->set_value_port() );
    p->connect( proc_pass->get_output_shot_break_flags_port(), pad_output_shot_break_flags->set_value_port() );
    p->connect( proc_pass->get_output_homography_port(),       pad_output_src2ref_homog->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),        pad_output_timestamp->set_value_port() );

    LOG_DEBUG( "stab_sp: Done setting up MAP-Tk pipeline" );
#else
    throw config_block_parse_error( "Tracker not built with MAP-Tk support." );
#endif
  }


// ----------------------------------------------------------------
/** Add data writer
 *
 * This method adds the tagged data writer, if it is enabled. Adding
 * the writer to the output of the pass-thru process simplifies the
 * connections.
 */
  template <class PIPELINE>
  void setup_pipeline_data_writer( PIPELINE * p )
  {
    if ( ! tdw_enable)
    {
      return;
    }

    p->add ( proc_data_write );

    p->connect( proc_pass->get_output_timestamp_port(),      proc_data_write->set_input_timestamp_port() );
    proc_data_write->set_timestamp_connected (true);

    helper_image(p, PixType());

    p->connect( proc_pass->get_output_metadata_vector_port(), proc_data_write->set_input_video_metadata_vector_port() );
    proc_data_write->set_video_metadata_vector_connected (true);

    p->connect( proc_pass->get_output_homography_port(),      proc_data_write->set_input_src_to_ref_homography_port() );
    proc_data_write->set_src_to_ref_homography_connected (true);

    p->connect( proc_pass->get_output_shot_break_flags_port(), proc_data_write->set_input_shot_break_flags_port() );
    proc_data_write->set_shot_break_flags_connected (true);

    bool masking_enabled = config.get< bool >( "masking_enabled" );
    if( masking_enabled )
    {
      p->connect( proc_pass->get_output_mask_port(),      proc_data_write->set_input_image_mask_port() );
      proc_data_write->set_image_mask_connected (true);
    }
  }


// ----------------------------------------------------------------
/** Create pipeline.
 *
 * Create a specific pipeline based on the operating mode.
 */
  template <class PIPELINE>
  void setup_pipeline( PIPELINE * p )
  {
    switch (mode)
    {
    case VIDTK_LOAD:
      setup_pipeline_load_mode(p);
      break;

    case VIDTK_COMPUTE:
      setup_pipeline_compute_mode(p);
      break;

    case VIDTK_COMPUTE_MAPTK:
      setup_pipeline_compute_maptk_mode(p);
      break;

    case VIDTK_DISABLED:
      setup_pipeline_disabled_mode(p);
      break;
    } // end switch

    // Attach data writer if specified.
    setup_pipeline_data_writer(p);
  }

}; // end class
// -----------------------------------------------------


template< class PixType>
stabilization_super_process<PixType>
::stabilization_super_process( std::string const& _name )
  : super_process( _name, "stabilization_super_process" ),
    impl_( new stabilization_super_process_impl<PixType> )
{
  impl_->create_process_configs();

  impl_->config.add_parameter( "run_async",
    "false",
    "Whether or not to run asynchronously" );

  impl_->config.add_parameter( "pipeline_edge_capacity",
    boost::lexical_cast<std::string>(10 / sizeof(PixType)),
    "Maximum size of the edge in an async pipeline." );

  impl_->config.add_parameter( "mode",
    "disabled",
    "Choose between load, compute, compute_maptk or disabled, i.e. load img2ref "
    "homographies from a file, compute new img2ref homographies, or disable "
    "this process and supply identity matrices by default." );

  impl_->config.add_parameter( "masking_enabled",
    "false",
    "Whether or not to run with masking support" );

  impl_->config.add_parameter( "masking_source",
    "internal",
    "Select either \"internal\" mask source or \"external\". If internal, "
    "the mask is created internally. If external, the mask image is generated externally "
    "and taken from an input port. This parameter is active only when masking is enabled." );

  impl_->config.add_parameter( "tracker_type", "klt",
    "Choose between \"klt\" and \"gpu\".\n"
    "  klt: Requires image pyramids and uses the traditional KLT point\n"
    "       tracking algorithm by Birchfield.\n"
    "  gpu: Require just images and uses a GPU accelerated implementation\n"
    "       of point descriptor matching with hessian corners and BRIEF\n"
    "       descriptors.  This algorithm depends on VisCL and OpenCL." );
}

template< class PixType>
stabilization_super_process<PixType>
::~stabilization_super_process()
{
  delete impl_;
}


// ------------------------------------------------------------------
template< class PixType>
config_block
stabilization_super_process<PixType>
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  config_block hidden;
  hidden.add_parameter(impl_->proc_homography_sp->name() + ":tracker_type", "Hidden option");
  hidden.add_subblock(impl_->proc_project_homography->params(),
                      impl_->proc_project_homography->name());
  tmp_config.remove(hidden);
  return tmp_config;
}


// ------------------------------------------------------------------
template< class PixType>
bool
stabilization_super_process<PixType>
::set_params( config_block const& blk )
{
  LOG_DEBUG( "stab_sp: entering set_params" );

  try
  {
    bool run_async = blk.get<bool>( "run_async" );

    config_enum_option op_mode;
    op_mode.add( "load",          impl_->VIDTK_LOAD )
           .add( "compute",       impl_->VIDTK_COMPUTE )
           .add( "disabled",      impl_->VIDTK_DISABLED )
           .add( "compute_maptk", impl_->VIDTK_COMPUTE_MAPTK )
      ;

    impl_->mode = blk.get< typename stabilization_super_process_impl<PixType>::sp_mode >( op_mode, "mode" );

    config_enum_option op_tracker_type;
    op_tracker_type.add( "klt", impl_->VIDTK_STAB_TRACK_KLT )
                   .add( "gpu", impl_->VIDTK_STAB_TRACK_GPU )
      ;

    impl_->tracker_type = blk.get< typename stabilization_super_process_impl<PixType>::tk_mode >( op_tracker_type, "tracker_type" );

#ifndef USE_VISCL
    if( impl_->tracker_type == impl_->VIDTK_STAB_TRACK_GPU )
    {
      throw std::runtime_error("Support for GPU tracking not compiled in, requires viscl.");
    }
#endif

    impl_->config.update( blk );

    impl_->tdw_enable = impl_->config.template get<bool>(impl_->proc_data_write->name() + ":enabled" );

    if (impl_->tdw_enable)
    {
      // Update parameters that have video name prefix.
      videoname_prefix::instance()->add_videoname_prefix(
        impl_->config, impl_->proc_data_write->name() + ":filename" );
    }

    if( run_async )
    {
      unsigned edge_capacity = blk.get<unsigned>( "pipeline_edge_capacity" );
      async_pipeline* p = new async_pipeline(async_pipeline::ENABLE_STATUS_FORWARDING, edge_capacity);

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else
    {
      sync_pipeline* p = new sync_pipeline;

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }

    impl_->config.set( impl_->proc_metadata_mask->name() + ":run_async", run_async );

    impl_->config.set( impl_->proc_homography_sp->name() + ":tracker_type",
                       blk.get<std::string>( "tracker_type" ) );

    double scale_factor = impl_->config.template get<double>(impl_->proc_rescale->name() + ":scale_factor");
    impl_->config.set( impl_->proc_project_homography->name() + ":scale",
                       scale_factor);

    LOG_DEBUG( "stab_sp: setting pipeline params" );
    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error( " unable to set pipeline parameters." );
    }

  }
  catch( config_block_parse_error const & e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
template< class PixType>
bool
stabilization_super_process<PixType>
::initialize()
{
  return this->pipeline_->initialize();
}


// ----------------------------------------------------------------
template< class PixType>
process::step_status
stabilization_super_process<PixType>
::step2()
{
  return this->pipeline_->execute();
}


// ------------------------------------------------------------------
template< class PixType>
void
stabilization_super_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}

template< class PixType>
void
stabilization_super_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  impl_->pad_source_image->set_value( img );
}

template< class PixType>
void
stabilization_super_process< PixType >
::set_source_mask( vil_image_view< bool > const& img )
{
  impl_->pad_source_mask->set_value( img );
}

template< class PixType >
timestamp
stabilization_super_process< PixType >
::source_timestamp() const
{
  return impl_->pad_output_timestamp->value();
}

template< class PixType >
vil_image_view< PixType >
stabilization_super_process< PixType >
::source_image() const
{
  return impl_->pad_output_image->value();
}

template< class PixType>
image_to_image_homography
stabilization_super_process<PixType>
::src_to_ref_homography() const
{
  return impl_->pad_output_src2ref_homog->value();
}

template< class PixType>
image_to_image_homography
stabilization_super_process<PixType>
::ref_to_src_homography() const
{
  image_to_image_homography h = impl_->pad_output_src2ref_homog->value();
  return h.get_inverse();
}

template< class PixType>
void
stabilization_super_process<PixType>
::set_source_metadata( video_metadata const& md )
{
  impl_->pad_source_metadata->set_value( md );
}

template< class PixType>
video_metadata::vector_t
stabilization_super_process<PixType>
::output_metadata_vector() const
{
  return impl_->pad_output_metadata->value();
}


template< class PixType>
std::vector< klt_track_ptr >
stabilization_super_process<PixType>
::active_tracks() const
{
  return impl_->pad_output_active_tracks->value();
}


template< class PixType >
vidtk::shot_break_flags
stabilization_super_process< PixType >::
get_output_shot_break_flags ( ) const
{
  return impl_->pad_output_shot_break_flags->value();
}


template< class PixType >
vil_image_view< bool >
stabilization_super_process< PixType >::
get_mask ( ) const
{
  return impl_->pad_output_mask->value();
}


} // end namespace vidtk
