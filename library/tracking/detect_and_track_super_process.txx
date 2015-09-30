/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "detect_and_track_super_process.h"

#include <pipeline/async_pipeline.h>
#include <pipeline/sync_pipeline.h>

#include <tracking/diff_super_process.h>
#include <tracking/conn_comp_super_process.h>
#include <tracking/tracking_super_process.h>
#include <tracking/shot_break_flags.h>
#include <tracking/shot_break_monitor.h>
#include <tracking/finalize_output_process.h>

#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/tagged_data_writer_process.h>

#include <vcl_sstream.h>
#include <vcl_algorithm.h>
#include <vul/vul_sprintf.h>

#include <logger/logger.h>


// ----------------------------------------------------------------
/*! \class detect_and_track_super_process
  \brief Detect and track processes.

  This class contains the main object detection and tracking
  processes.

  \msc
  FTSP-pipeline, DTSP, pipeline, diff_sp, conn_comp, track
  --- [label="normal operation"];
  FTSP-pipeline->DTSP [label="step2()"];
  DTPS->pipeline [label="execute()"];
  pipeline->diff_sp [label="step()"];
  pipeline->conn_comp [label="step()"];
  pipeline->track [label="step()"];

  --- [label="modality change"];
  FTSP-pipeline->DTSP [label="step2()"];
  FTSP-pipeline<<-DTSP [label="FAILURE"];
  FTSP-pipeline->DTSP [label="fail_recover()"];
  DTSP->pipeline [label="reset_downstream()"];
  FTSP-pipeline->DTSP [label="step2()"];
  \endmsc

 */


namespace vidtk
{

VIDTK_LOGGER ("detect_and_track_super_process");

template< class PixType>
class detect_and_track_super_process_impl
{
public:

  // Pipeline processes
  process_smart_pointer< shot_break_monitor< PixType > > proc_break_monitor;
  process_smart_pointer< diff_super_process< PixType > > proc_diff_sp;
  process_smart_pointer< conn_comp_super_process< PixType > > proc_conn_comp_sp;
  process_smart_pointer< tracking_super_process< PixType > > proc_tracking_sp;
  process_smart_pointer< finalize_output_process > proc_finalize;
  process_smart_pointer< tagged_data_writer_process > proc_data_write;


  // Input Pads (dummy processes)
#define CONNECTION_DEF(T,N)                                           \
  typedef vidtk::super_process_pad_impl < T >  pad_input_ ## N ## _t;   \
  vidtk::process_smart_pointer< pad_input_ ## N ## _t> pad_input_ ## N;

  DTSP_PROCESS_INPUTS // expand macro over inputs

#undef CONNECTION_DEF


  // Output Pads (dummy processes)
#define CONNECTION_DEF(T,N)                                            \
typedef vidtk::super_process_pad_impl < T > pad_output_ ## N ## _t;   \
vidtk::process_smart_pointer< pad_output_ ## N ## _t > pad_output_ ## N;

  DTSP_PROCESS_OUTPUTS // expand macro over outputs

#undef CONNECTION_DEF


  // Members used by get_modality_config()
  vcl_map< video_modality, config_block > modality_config_map;
  config_block pipeline_default_config;
  vcl_string modality_cfg_file[ VIDTK_VIDEO_MODALITY_END ]; // length should be equal to that of enum video_modality

  // Configuration parameters
  config_block config;
  config_block dependency_config;
  bool pmw_enable;

  detect_and_track_super_process < PixType > * m_parent;


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
 explicit detect_and_track_super_process_impl(detect_and_track_super_process <PixType> * parent)
    : proc_break_monitor( NULL ),
      proc_diff_sp( NULL ),
      proc_conn_comp_sp( NULL ),
      proc_tracking_sp( NULL ),
      proc_finalize( NULL ),
      proc_data_write( NULL ),

// -- input pads
#define CONNECTION_DEF(T,N) pad_input_ ## N( NULL ),
      DTSP_PROCESS_INPUTS // apply macro over inputs
#undef CONNECTION_DEF

// -- output pads
#define CONNECTION_DEF(T,N) pad_output_ ## N( NULL ),
      DTSP_PROCESS_OUTPUTS // apply macro over outputs
#undef CONNECTION_DEF

    modality_config_map(),
    pipeline_default_config(),
    modality_cfg_file(),
    config(),
      pmw_enable(false),
    m_parent(parent)
  {
    create_process_configs();

    config.add_parameter( "run_async",
      "false",
      "Whether or not to run asynchronously" );

    config.add_parameter( "masking_enabled",
      "false",
      "Whether or not to run with (metadata burnin) masking support" );

    config.add_parameter( "location_type",
      "centroid",
      "Location of the target for tracking: bottom or centroid. "
      "This parameter is used in conn_comp_sp:conn_comp, tracking_sp:tracker, and tracking_sp:state_to_image");

    config.add_parameter( "modality_config:EOM",
      "",
      "Modality specific config file for EO and MFOV. Path relative to the "
      "common config file (e.g. *_Common.conf)" );

    config.add_parameter( "modality_config:EON",
      "",
      "Modality specific config file for EO and NFOV. Path relative to the "
      "common config file (e.g. *_Common.conf)" );

    config.add_parameter( "modality_config:IRM",
      "",
      "Modality specific config file for IR and MFOV. Path relative to the "
      "common config file (e.g. *_Common.conf)" );

    config.add_parameter( "modality_config:IRN",
      "",
      "Modality specific config file for IR and NFOV. Path relative to the "
      "common config file (e.g. *_Common.conf)" );

    config.add_parameter( "modality_config:EOFB",
      "",
      "Modality specific config file for EO and Fallback. Path relative to the "
      "common config file (e.g. *_Common.conf)" );

    config.add_parameter( "modality_config:IRFB",
      "",
      "Modality specific config file for IR and Fallback. Path relative to the "
      "common config file (e.g. *_Common.conf)" );

  }


  // ----------------------------------------------------------------
  /** Create process objects.
   *
   * This method creates all needed processes objects and creates the
   * associated config tree.
   */
  void create_process_configs()
  {
    // input pads
#define CONNECTION_DEF(T,N) pad_input_ ## N = new pad_input_ ## N ## _t( # N "_dtsp_in_pad" );

    DTSP_PROCESS_INPUTS // expand macros over input ports

#undef CONNECTION_DEF

// output pads
#define CONNECTION_DEF(T,N) pad_output_ ## N = new pad_output_ ## N ## _t( # N "_dtsp_out_pad" );
    DTSP_PROCESS_OUTPUTS // expand macros over output ports

#undef CONNECTION_DEF

    proc_break_monitor = new  shot_break_monitor< PixType >( "shot_break_mon" );
    config.add_subblock( proc_break_monitor->params(), proc_break_monitor->name() );

    proc_diff_sp = new diff_super_process< PixType >( "diff_sp" );
    config.add_subblock( proc_diff_sp->params(), proc_diff_sp->name() );

    proc_conn_comp_sp = new conn_comp_super_process< PixType >( "conn_comp_sp" );
    config.add_subblock( proc_conn_comp_sp->params(), proc_conn_comp_sp->name() );

    proc_tracking_sp = new tracking_super_process< PixType >( "tracking_sp" );
    config.add_subblock( proc_tracking_sp->params(), proc_tracking_sp->name() );

    proc_finalize = new finalize_output_process ("output_finalizer");
    config.add_subblock( proc_finalize->params(), proc_finalize->name() );

    proc_data_write = new tagged_data_writer_process("tagged_data_writer");
    config.add_subblock( proc_data_write->params(), proc_data_write->name() );

    // Parameters which will be over-written by this process.
    dependency_config.add( proc_diff_sp->name() + ":masking_enabled", "false" );
    dependency_config.add( proc_conn_comp_sp->name()
                           + ":conn_comp:location_type",
                           "centroid" );

    dependency_config.add( proc_tracking_sp->name()
                           + ":tracker:location_type",
                           "centroid" );

    dependency_config.add( proc_tracking_sp->name()
                           + ":state_to_image:smooth_img_loc:location_type",
                           "centroid" );
  }


// ----------------------------------------------------------------
/** setup pipeline
 *
 *
 */
template <class PIPELINE>
void setup_pipeline( PIPELINE* p )
{
  // -- add input pads
#define CONNECTION_DEF(T,N) p->add( pad_input_ ## N );
  DTSP_PROCESS_INPUTS // apply macro over input ports
#undef CONNECTION_DEF

  p->add (proc_break_monitor);
  p->add( proc_diff_sp );
  p->add( proc_conn_comp_sp );
  p->add( proc_tracking_sp );
  p->add( proc_finalize );

  // -- add output pads
#define CONNECTION_DEF(T,N) p->add( pad_output_ ## N );
  DTSP_PROCESS_OUTPUTS // apply macro over output ports
#undef CONNECTION_DEF

// These macros make connections between processes for the pass-thru ports.
// They are basically making use of the port naming convention.
#define CONNECT_PASS_THRU(N,SRC,DST)                    \
    p->connect( SRC->get_output_ ## N ## _port(),       \
                DST->set_input_ ## N ## _port() )

#define CONNECT_OPTIONAL_PASS_THRU(N,SRC,DST)                   \
    p->connect_optional( SRC->get_output_ ## N ## _port(),      \
                         DST->set_input_ ## N ## _port() )


  // Input Pad connections connect to proc_break_monitor
  p->connect( pad_input_timestamp->value_port(),             proc_break_monitor->set_input_timestamp_port());
  p->connect( pad_input_timestamp_vector->value_port(),      proc_break_monitor->set_input_timestamp_vector_port());
  p->connect( pad_input_image->value_port(),                 proc_break_monitor->set_input_image_port() );
  p->connect( pad_input_src_to_ref_homography->value_port(), proc_break_monitor->set_input_src_to_ref_homography_port() );
  p->connect( pad_input_wld_to_src_homography->value_port(), proc_break_monitor->set_input_wld_to_src_homography_port() );
  p->connect( pad_input_wld_to_utm_homography->value_port(), proc_break_monitor->set_input_wld_to_utm_homography_port() );
  p->connect( pad_input_src_to_wld_homography->value_port(), proc_break_monitor->set_input_src_to_wld_homography_port() );
  p->connect( pad_input_ref_to_wld_homography->value_port(), proc_break_monitor->set_input_ref_to_wld_homography_port() );
  p->connect( pad_input_world_units_per_pixel->value_port(), proc_break_monitor->set_input_world_units_per_pixel_port() );
  p->connect( pad_input_shot_break_flags->value_port(),      proc_break_monitor->set_input_shot_break_flags_port() );
  p->connect( pad_input_mask->value_port(),                  proc_break_monitor->set_input_mask_port() );

  // connect proc_diff_sp
  // foreach port (connect proc_break_monitor, proc_diff_sp )
  p->connect( proc_break_monitor->get_output_timestamp_port(),             proc_diff_sp->set_source_timestamp_port());
  p->connect( proc_break_monitor->get_output_timestamp_vector_port(),      proc_diff_sp->set_input_timestamp_vector_port());
  p->connect( proc_break_monitor->get_output_image_port(),                 proc_diff_sp->set_source_image_port() );
  p->connect( proc_break_monitor->get_output_src_to_ref_homography_port(), proc_diff_sp->set_src_to_ref_homography_port() );
  p->connect( proc_break_monitor->get_output_wld_to_src_homography_port(), proc_diff_sp->set_wld_to_src_homography_port() );
  p->connect( proc_break_monitor->get_output_wld_to_utm_homography_port(), proc_diff_sp->set_wld_to_utm_homography_port() );
  p->connect( proc_break_monitor->get_output_src_to_wld_homography_port(), proc_diff_sp->set_src_to_wld_homography_port() );
  p->connect( proc_break_monitor->get_output_world_units_per_pixel_port(), proc_diff_sp->set_world_units_per_pixel_port() );
  p->connect( proc_break_monitor->get_output_shot_break_flags_port(),      proc_diff_sp->set_input_shot_break_flags_port() );
  p->connect( proc_break_monitor->get_output_mask_port(),                  proc_diff_sp->set_mask_port() );

  CONNECT_PASS_THRU (video_modality,     proc_break_monitor, proc_diff_sp );


  // connect conn_comp
  p->connect( proc_diff_sp->source_timestamp_port(),      proc_conn_comp_sp->set_source_timestamp_port() );
  p->connect( proc_diff_sp->source_image_port(),          proc_conn_comp_sp->set_source_image_port() );
  p->connect( proc_diff_sp->copied_fg_out_port(),         proc_conn_comp_sp->set_fg_image_port() );
  p->connect( proc_diff_sp->world_units_per_pixel_port(), proc_conn_comp_sp->set_world_units_per_pixel_port() );
  p->connect( proc_diff_sp->src_to_ref_homography_port(), proc_conn_comp_sp->set_src_to_ref_homography_port() );
  p->connect( proc_diff_sp->src_to_wld_homography_port(), proc_conn_comp_sp->set_src_to_wld_homography_port() );
  p->connect( proc_diff_sp->wld_to_src_homography_port(), proc_conn_comp_sp->set_wld_to_src_homography_port() );
  p->connect( proc_diff_sp->wld_to_utm_homography_port(), proc_conn_comp_sp->set_wld_to_utm_homography_port() );
  p->connect( proc_diff_sp->copied_diff_out_port(),       proc_conn_comp_sp->set_input_diff_out_image_port() );

  CONNECT_PASS_THRU (ref_to_wld_homography,  proc_diff_sp, proc_conn_comp_sp );
  CONNECT_PASS_THRU (video_modality,         proc_diff_sp, proc_conn_comp_sp );
  CONNECT_PASS_THRU (shot_break_flags,       proc_diff_sp, proc_conn_comp_sp );
  CONNECT_PASS_THRU (timestamp_vector,       proc_diff_sp, proc_conn_comp_sp );


  // connect tracking
  p->connect( proc_conn_comp_sp->source_timestamp_port(),      proc_tracking_sp->set_timestamp_port() );
  p->connect( proc_conn_comp_sp->source_image_port(),          proc_tracking_sp->set_source_image_port() );
  p->connect( proc_conn_comp_sp->src_to_wld_homography_port(), proc_tracking_sp->set_src_to_wld_homography_port() );
  p->connect( proc_conn_comp_sp->wld_to_src_homography_port(), proc_tracking_sp->set_wld_to_src_homography_port() );
  p->connect( proc_conn_comp_sp->wld_to_utm_homography_port(), proc_tracking_sp->set_wld_to_utm_homography_port() );
  p->connect( proc_conn_comp_sp->output_objects_port(),        proc_tracking_sp->set_source_objects_port() );
  p->connect( proc_conn_comp_sp->src_to_ref_homography_port(), proc_tracking_sp->set_src_to_ref_homography_port() );
  p->connect( proc_conn_comp_sp->fg_image_port(),              proc_tracking_sp->set_fg_image_port() );
  p->connect( proc_conn_comp_sp->get_output_diff_out_image_port(), proc_tracking_sp->set_diff_image_port() );
  p->connect( proc_conn_comp_sp->copied_morph_image_port(),    proc_tracking_sp->set_morph_image_port() );
  p->connect( proc_conn_comp_sp->projected_objects_port(),     proc_tracking_sp->set_unfiltered_objects_port() );
  p->connect( proc_conn_comp_sp->world_units_per_pixel_port(), proc_tracking_sp->set_world_units_per_pixel_port() );

  CONNECT_OPTIONAL_PASS_THRU (ref_to_wld_homography,      proc_conn_comp_sp, proc_tracking_sp );
  CONNECT_OPTIONAL_PASS_THRU (video_modality,             proc_conn_comp_sp, proc_tracking_sp );
  CONNECT_OPTIONAL_PASS_THRU (shot_break_flags,           proc_conn_comp_sp, proc_tracking_sp );
  CONNECT_OPTIONAL_PASS_THRU (timestamp_vector,           proc_conn_comp_sp, proc_tracking_sp );


  // Connect tracking -> finalizer
  // The finalizer must go here (for now) because if is placed in FTSP, it can not tell the difference between
  // end of video and a DTSP reset, so it causes FTSP to *termiate* when the DTSP resets :-(
  p->connect ( proc_tracking_sp->out_timestamp_port(),                    proc_finalize->set_input_timestamp_port() );
  p->connect ( proc_tracking_sp->get_output_timestamp_vector_port(),      proc_finalize->set_input_timestamp_vector_port() );
  p->connect ( proc_tracking_sp->active_tracks_port(),                    proc_finalize->set_input_active_tracks_port() );
  p->connect ( proc_tracking_sp->src_to_ref_homography_port(),            proc_finalize->set_input_src_to_ref_homography_port() );
  p->connect ( proc_tracking_sp->src_to_utm_homography_port(),            proc_finalize->set_input_src_to_utm_homography_port() );
  p->connect ( proc_tracking_sp->get_output_ref_to_wld_homography_port(), proc_finalize->set_input_ref_to_wld_homography_port() );
  p->connect ( proc_tracking_sp->world_units_per_pixel_port(),            proc_finalize->set_input_world_units_per_pixel_port() );
  p->connect ( proc_tracking_sp->get_output_video_modality_port(),        proc_finalize->set_input_video_modality_port() );
  p->connect ( proc_tracking_sp->get_output_shot_break_flags_port(),      proc_finalize->set_input_shot_break_flags_port() );
  p->connect ( proc_tracking_sp->linked_tracks_port(),                    proc_finalize->set_input_linked_tracks_port() );
  p->connect ( proc_tracking_sp->filtered_tracks_port(),                  proc_finalize->set_input_filtered_tracks_port() );


  // connect finalizer -> Output Pad connections (finalizer order)
  p->connect ( proc_finalize->get_output_timestamp_port(),             pad_output_timestamp->set_value_port() );
  p->connect ( proc_finalize->get_output_timestamp_vector_port(),      pad_output_timestamp_vector->set_value_port() );
  p->connect ( proc_finalize->get_output_src_to_ref_homography_port(), pad_output_src_to_ref_homography->set_value_port() );
  p->connect ( proc_finalize->get_output_src_to_utm_homography_port(), pad_output_src_to_utm_homography->set_value_port() );
  p->connect ( proc_finalize->get_output_ref_to_wld_homography_port(), pad_output_ref_to_wld_homography->set_value_port() );
  p->connect ( proc_finalize->get_output_world_units_per_pixel_port(), pad_output_world_units_per_pixel->set_value_port() );
  p->connect ( proc_finalize->get_output_video_modality_port(),        pad_output_video_modality->set_value_port() );
  p->connect ( proc_finalize->get_output_shot_break_flags_port(),      pad_output_shot_break_flags->set_value_port() );

  // See finalize_output_process.h for the validity of the output (finalized,
  // terminated, filtered, and linked tracks) track ports.
  p->connect ( proc_finalize->get_output_finalized_tracks_port(),      pad_output_active_tracks->set_value_port() );
  p->connect ( proc_finalize->get_output_filtered_tracks_port(),       pad_output_filtered_tracks->set_value_port() );
  p->connect ( proc_finalize->get_output_linked_tracks_port(),         pad_output_linked_tracks->set_value_port() );

#undef CONNECT_PASS_THRU
#undef CONNECT_OPTIONAL_PASS_THRU

    // add tap for tagged data writer
    if (pmw_enable)
    {
      p->add ( proc_data_write );

      p->connect( proc_finalize->get_output_timestamp_port(),                proc_data_write->set_input_timestamp_port() );
      proc_data_write->set_timestamp_connected (true);

      p->connect( proc_finalize->get_output_src_to_ref_homography_port(),    proc_data_write->set_input_src_to_ref_homography_port() );
      proc_data_write->set_src_to_ref_homography_connected (true);

      p->connect( proc_finalize->get_output_src_to_utm_homography_port(),    proc_data_write->set_input_src_to_utm_homography_port() );
      proc_data_write->set_src_to_utm_homography_connected (true);

      p->connect( proc_finalize->get_output_ref_to_wld_homography_port(),    proc_data_write->set_input_ref_to_wld_homography_port() );
      proc_data_write->set_ref_to_wld_homography_connected (true);

      p->connect( proc_finalize->get_output_world_units_per_pixel_port(),    proc_data_write->set_input_world_units_per_pixel_port() );
      proc_data_write->set_world_units_per_pixel_connected (true);

      p->connect( proc_finalize->get_output_shot_break_flags_port(),         proc_data_write->set_input_shot_break_flags_port() );
      proc_data_write->set_shot_break_flags_connected (true);
    }
}


  //------------------------------------------------------------------------
  /**
   *  \brief This function provides config from the modality specific file(s).
   *
   *  This function first starts with the default config_block for the
   *  pipeline as it was in the beginning of the application. Then it
   *  upates the values in the provided config file, which is
   *  considered as the *common* (X_Common.conf) configuration for all
   *  the video modalities. After this the config file specific to the
   *  current modality is loaded into the config block, which is then
   *  returned to the calling function.
   *
   *  A few important points to note:
   *  - The video modality (curr_mode) is computed outside this function.
   *  - Names of the modality specific config files are specified in config
   *    file through relativepath keyword.
   *  - If there is a redundant parameter in both common and modality specific
   *    file, then the value in modality specific file will prevail.
   *
   * @param[out] blk - filled in with config for current mode
   */
  bool get_modality_config( config_block & blk)
  {
    video_modality curr_mode = proc_break_monitor->current_modality();
    if( ! modality_cfg_file[ curr_mode ].empty() )
    {
      LOG_INFO( m_parent->name() << ": New video modality: " << video_modality_to_string( curr_mode ));
    }

    // Then load the mondality specific file (EON/IRN/EOM/EON)
    if( modality_config_map.find( curr_mode ) == modality_config_map.end() )
    {
      // First load the Common config
      blk = modality_config_map[ VIDTK_COMMON ];

      // We haven't loaded config_block for this modality before, hence
      // parse the corresponding config file.

      if( modality_cfg_file[ curr_mode ].empty() )
      {
        LOG_WARN( "Not loading video modality specific config file. (mode: "
                  << curr_mode << "). File name not specified, using common config." );
      }
      else
      {
        LOG_INFO( "Loading config file: " << modality_cfg_file[ curr_mode ]);
        // Parse modality config file. Add modal config entries to
        // common config block.

        try
        {
        blk.parse( modality_cfg_file[ curr_mode ] );
      }
        catch( unchecked_return_value const& e )
        {
          LOG_ERROR( "Could not parse file ("<< modality_cfg_file[ curr_mode ]
            <<"), because: " << e.what() );
          return false;
        }
      }

      // store for reusability
      modality_config_map[ curr_mode ] = blk;
    }
    else
    {
      // just return the config block
      blk = modality_config_map[ curr_mode ];
    }

    return true;
  } // bool get_modality_config()

}; // end impl class


// ----------------------------------------------------------------
/** Constructor
 *
 *
 */
template< class PixType >
detect_and_track_super_process< PixType >
::detect_and_track_super_process( vcl_string const& name )
: super_process( name, "detect_and_track_super_process" ),
  m_impl( NULL ),
  videoname_prefixing_done_( false )
{
  m_impl = new detect_and_track_super_process_impl < PixType >(this);
}


template< class PixType>
detect_and_track_super_process< PixType >
::~detect_and_track_super_process()
{
  delete m_impl;
}


// ----------------------------------------------------------------
/** Provide needed parameters
 *
 * This method returns a config block containint all needed
 * parameters.  These parameters have been collected from all
 * submodules/processes.
 */
template< class PixType >
config_block
detect_and_track_super_process< PixType >
::params() const
{
  config_block tmp_config;
  tmp_config = m_impl->config;
  tmp_config.remove( m_impl->dependency_config );
  return tmp_config;
}


// ----------------------------------------------------------------
/** Accept parameter values.
 *
 * This method accepts the supplied parameter values.  These
 * values of the parameters have been set by the config file
 *
 * @param[in] blk - config block for this super-process
 *
 * @retval true - config accepted o.k.
 * @retval false - error in config
 */
template< class PixType >
bool
detect_and_track_super_process< PixType >
::set_params(config_block const& blk)
{
  try
  {
    // update our config early so final values are available when we
    // setup the pipeline.
    m_impl->config.update( blk );

    // Update parameters for this process
    bool run_async = false;
    blk.get( "run_async", run_async );

    // Update diff_sp configs with mask settings from ftsp
    bool mask_enabled;
    blk.get( "masking_enabled", mask_enabled );
    m_impl->dependency_config.set( m_impl->proc_diff_sp->name() + ":masking_enabled", mask_enabled );

    // dependency of location_type
    vcl_string loc = blk.get<vcl_string>( "location_type" );
    m_impl->dependency_config.set( m_impl->proc_conn_comp_sp->name()
                                   + ":conn_comp:location_type", loc );
    m_impl->dependency_config.set( m_impl->proc_tracking_sp->name()
                                   + ":tracker:location_type", loc );
    m_impl->dependency_config.set( m_impl->proc_tracking_sp->name()
                                   + ":state_to_image:smooth_img_loc:location_type", loc );

    // update now so that dependency config is not over-written by last statement above
    m_impl->config.update(m_impl->dependency_config);

    // Set whether tagged data writer is enabled
    m_impl->config.get(m_impl->proc_data_write->name() + ":enabled", m_impl->pmw_enable);

    // Note that it may look like a good idea to refactor the last two
    // lines of the following if and else block to common code, it
    // really is type specific and must remain as is.
    if( run_async )
    {
      LOG_INFO( "Starting async detect_and_track_super_process");
      async_pipeline* p = new async_pipeline(async_pipeline::ENABLE_STATUS_FOWARDING);
      pipeline_ = p;
      m_impl->setup_pipeline( p );
    }
    else
    {
      LOG_INFO( "Starting sync detect_and_track_super_process" );
      sync_pipeline* p = new sync_pipeline;
      pipeline_ = p;
      m_impl->setup_pipeline( p );
    }

    blk.get( "modality_config:EOM",  m_impl->modality_cfg_file[ VIDTK_EO_M ] );
    blk.get( "modality_config:EON",  m_impl->modality_cfg_file[ VIDTK_EO_N ] );
    blk.get( "modality_config:IRM",  m_impl->modality_cfg_file[ VIDTK_IR_M ] );
    blk.get( "modality_config:IRN",  m_impl->modality_cfg_file[ VIDTK_IR_N ] );
    blk.get( "modality_config:EOFB", m_impl->modality_cfg_file[ VIDTK_EO_FB ] );
    blk.get( "modality_config:IRFB", m_impl->modality_cfg_file[ VIDTK_IR_FB ] );

    //
    // Check configuration constraints
    //
    bool finalizer_enabled(false);
    blk.get( m_impl->proc_finalize->name() + ":enabled", finalizer_enabled );

    if( finalizer_enabled )
    {
      int finalizer_delay(0);
      blk.get( m_impl->proc_finalize->name() + ":delay_frames", finalizer_delay );

      int track_init(0);
      blk.get( m_impl->proc_tracking_sp->name() + ":track_init_duration_frames", track_init );

      int track_term(0);
      blk.get( m_impl->proc_tracking_sp->name() + ":track_termination_duration_frames", track_term );

      int back_track(0);
      blk.get( m_impl->proc_tracking_sp->name() + ":back_tracking_duration_frames", back_track );

      bool back_track_off(true);
      blk.get( m_impl->proc_tracking_sp->name() + ":back_tracking_disabled", back_track_off );

      int min_delay = vcl_max (track_init, track_term) + 1;
      if( !back_track_off )
      {
        min_delay += back_track;
      }

      if ( finalizer_delay < min_delay )
      {
        LOG_ERROR( name() << ": finalizer_delay: " << finalizer_delay
                  << "  track_init; " << track_init
                  << "  track_term: " << track_term
                  << "  min_delay: " << min_delay
                  << "\n" );
        LOG_ERROR( name() << ": Configuration constraint violated: finalizer delay not large enough.\n"
                   "Need to fix the config file. Delay should be at least " << min_delay );

        return false;
      } // finalizer_delay < min_delay
    } // finalizer_enabled


    if( !this->get_videoname_prefix().empty() )
    {
      if (videoname_prefixing_done_)
      {
        throw unchecked_return_value(" set_params unexpectedly called more than once.");
      }

      videoname_prefixing_done_ = true;

      // Update parameters that have video name prefix.
      // Note updating is done in the returned copy of the config, not in the saved copy.
      add_videoname_prefix( m_impl->config, m_impl->proc_diff_sp->name() + ":diff_fg_out:pattern" );
      add_videoname_prefix( m_impl->config, m_impl->proc_diff_sp->name() + ":diff_float_out:pattern" );
      add_videoname_prefix( m_impl->config, m_impl->proc_diff_sp->name() + ":diff_fg_zscore_out:pattern" );

      add_videoname_prefix( m_impl->config, m_impl->proc_tracking_sp->name() + ":output_tracks_unfiltered:filename" );
      add_videoname_prefix( m_impl->config, m_impl->proc_tracking_sp->name() + ":output_tracks_filtered:filename" );

      // The filename passed to the finalizer is for the currently
      // mis-placed metadata writer
      add_videoname_prefix( m_impl->config, m_impl->proc_finalize->name() + ":filename" );
    }

    // Store settings for reusability in the shot break monitor
    m_impl->modality_config_map[ VIDTK_COMMON ] = m_impl->config;

    if( ! pipeline_->set_params( m_impl->config ) )
    {
      throw unchecked_return_value(" unable to set pipeline parameters.");
    }
  }
  catch(unchecked_return_value & e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what());

    return ( false );
  }

  return ( true );
} /* set_params */


// ----------------------------------------------------------------
/** Initialise super process.
 *
 *
 */
template< class PixType >
bool
detect_and_track_super_process< PixType >
::initialize()
{
  return pipeline_->initialize();
}


// ----------------------------------------------------------------
/** Main process step
 *
 * This method is not called if we are running an async pipeline.
 */
template< class PixType >
process::step_status
detect_and_track_super_process< PixType >
::step2()
{

  // -- run our pipeline -- sync mode only --
  process::step_status p_status = pipeline_->execute();

  return (p_status);
}


// ----------------------------------------------------------------
/** Recover from pipeline failure.
 *
 * This function is called when the pipeline detects failure in the
 * sp.  It gives the sp a last chance to recover from the failure
 * before propigating the failure out the the external pipeline.
 *
 * @retval true - recovery o.k.
 * @retval false - could not recover
 */
template< class PixType >
bool
detect_and_track_super_process< PixType >
::fail_recover()
{
  // Checking to see if the pipeline failed due to one of the planned pipeline
  // resets.
  LOG_INFO( this->name() << ": Entering fail_recover()." );

  // 1. Start of shot reset. Based on change of video modality.
  if ( m_impl->proc_break_monitor->is_modality_change() )
  {
    LOG_INFO(this->name() << ": Attempting to recover from failure, "
                          << "due to start of shot (modality change).");

    config_block curr_config;
    if( !m_impl->get_modality_config( curr_config) )
    {
      LOG_ERROR(this->name() << ": Could not get modality config.");
      return false;
    }

    if( ! (this->pipeline_->set_params_downstream( curr_config, m_impl->proc_diff_sp.ptr() ) &&
      this->pipeline_->reset_downstream( m_impl->proc_break_monitor.ptr() ) ) )
    {
      LOG_ERROR( this->name() << ": Could not set parameters and reset "
                 "pipeline after a start of shot reset." );
      return false;
    }

    LOG_INFO(this->name() << ": Recovering o.k. returning true." );
    return true;
  }

  // 2. End of shot reset.
  if ( m_impl->proc_break_monitor->is_end_of_shot() )
  {
    LOG_INFO(this->name() << ": Attempting to recover from failure, due to end of shot.");

    if ( this->pipeline_->reset_downstream(m_impl->proc_break_monitor.ptr()) )
    {
      return true;
    }
  }

  LOG_WARN(this->name() << ": Could not recover from failure." );

  return false;
}


// ---------- utility methods ---------
template< class PixType >
void
detect_and_track_super_process< PixType >
::set_multi_features_dir( vcl_string const & dir )
{
  m_impl->proc_tracking_sp->set_multi_features_dir( dir );
}


template< class PixType >
void
detect_and_track_super_process< PixType >
::set_guis( process_smart_pointer< gui_process > ft_gui,
            process_smart_pointer< gui_process > bt_gui )
{
  m_impl->proc_tracking_sp->set_guis( ft_gui, bt_gui );
}


// ----------- input methods ------------
// Automatically generate, since they are all pads
// Pushes value into pads.
#define CONNECTION_DEF(T,N)                   \
template< class PixType >                       \
void detect_and_track_super_process< PixType >  \
::set_input_ ##N ( T const& val )               \
{ m_impl->pad_input_ ## N->set_value( val ); }

  DTSP_PROCESS_INPUTS // expand macro over inputs

#undef CONNECTION_DEF


// ----------- output methods ------------
// Automatically generate, since they are all pads
// Pulls value from pads.
#define CONNECTION_DEF(T,N)                             \
template< class PixType >                               \
T const& detect_and_track_super_process< PixType >      \
::get_output_ ## N() const                              \
{ return m_impl->pad_output_ ## N->value(); }

  DTSP_PROCESS_OUTPUTS // expand macro over outputs

#undef CONNECTION_DEF


} // namespace vidtk
