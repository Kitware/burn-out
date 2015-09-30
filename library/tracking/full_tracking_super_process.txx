/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline/async_pipeline.h>
#include <pipeline/sync_pipeline.h>

#include <tracking/full_tracking_super_process.h>

#include <tracking/stabilization_super_process.h>
#include <tracking/world_coord_super_process.h>
#include <tracking/detect_and_track_super_process.h>

#include <logger/logger.h>

#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/filter_video_metadata_process.h>
#include <utilities/timestamp.h>
#include <utilities/kw_archive_writer_process.h>
#include <utilities/tagged_data_writer_process.h>

#include <logger/logger.h>


#include <vcl_sstream.h>
#include <vcl_algorithm.h>
#include <vul/vul_sprintf.h>
#include <vul/vul_file.h>


// Declare logger object
VIDTK_LOGGER("full_tracking_super_process") ;

namespace vidtk
{

VIDTK_LOGGER("full_tracking_super_process");

template< class PixType>
class full_tracking_super_process_impl
{
public:
  typedef vidtk::super_process_pad_impl< vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl< timestamp> timestamp_pad;
  typedef vidtk::super_process_pad_impl< video_metadata > metadata_pad;
  typedef vidtk::super_process_pad_impl< vcl_vector< track_sptr > > tracks_pad;
  typedef vidtk::super_process_pad_impl< image_to_image_homography > i2i_homog_pad;
  typedef vidtk::super_process_pad_impl< image_to_utm_homography > i2u_homog_pad;
  typedef vidtk::super_process_pad_impl< image_to_plane_homography > i2p_homog_pad;
  typedef vidtk::super_process_pad_impl< double > gsd_pad;
  typedef vidtk::super_process_pad_impl< vidtk::video_modality > video_modality_pad;
  typedef vidtk::super_process_pad_impl< vidtk::shot_break_flags > shot_break_flags_pad;

  // Pipeline processes
  process_smart_pointer< stabilization_super_process< PixType > > proc_stab_sp;
  process_smart_pointer< world_coord_super_process< PixType > > proc_world_coord_sp;
  process_smart_pointer< kw_archive_writer_process > proc_kwa_writer;
  process_smart_pointer< detect_and_track_super_process< PixType > > proc_dtsp;
  process_smart_pointer< tagged_data_writer_process > proc_data_write;

  // Input Pads (dummy processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< metadata_pad > pad_source_metadata;

  // Output Pads (dummy processes)
  process_smart_pointer< timestamp_pad > pad_finalized_timestamp;
  process_smart_pointer< tracks_pad > pad_finalized_tracks;
  process_smart_pointer< i2i_homog_pad > pad_finalized_src_to_ref_homography;
  process_smart_pointer< i2u_homog_pad > pad_finalized_src_to_utm_homography;
  process_smart_pointer< i2p_homog_pad > pad_finalized_ref_to_wld_homography;
  process_smart_pointer< gsd_pad > pad_finalized_world_units_per_pixel;
  process_smart_pointer< video_modality_pad > pad_finalized_video_modality;
  process_smart_pointer< shot_break_flags_pad > pad_finalized_shot_break_flags;

  // Configuration parameters
  config_block config;
  config_block dependency_config;

  unsigned long step_count;
  bool print_timing_tags;


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
full_tracking_super_process_impl()
  : proc_stab_sp( NULL ),
    proc_world_coord_sp( NULL ),
    proc_kwa_writer( NULL ),
    proc_dtsp( NULL ),
    proc_data_write( NULL ),

    // input pads
    pad_source_image( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_metadata( NULL ),

    // output pads
    pad_finalized_timestamp( NULL ),
    pad_finalized_tracks( NULL ),
    pad_finalized_src_to_ref_homography( NULL ),
    pad_finalized_src_to_utm_homography( NULL ),
    pad_finalized_ref_to_wld_homography( NULL ),
    pad_finalized_world_units_per_pixel( NULL ),
    pad_finalized_video_modality( NULL ),
    pad_finalized_shot_break_flags( NULL),

    step_count( 0 ),
    print_timing_tags( false )
  {
    create_process_configs();

    config.add_parameter( "run_async",
      "false",
      "Whether or not to run asynchronously" );

    config.add_parameter( "masking_enabled",
      "false",
      "Whether or not to run with (metadata burnin) masking support" );

    config.add_parameter( "print_timing_tags",
      "false",
      "Whether or not to print timing data when done." );
  }


  // ----------------------------------------------------------------
  /** Create process objects.
   *
   * This method creates all needed processes objects and creates the
   * associated config block.
   */
  void create_process_configs()
  {
    // input pads
    pad_source_image = new image_pad("source_image_ftsp_in_pad");
    pad_source_timestamp = new timestamp_pad("source_timestamp_ftsp_in_pad");
    pad_source_metadata = new metadata_pad("source_metadata_ftsp_in_pad");

    //output pads
    pad_finalized_timestamp = new timestamp_pad("finalized_timestamp_ftsp_out_pad");
    pad_finalized_tracks = new tracks_pad("finalized_tracks_ftsp_out_pad");
    pad_finalized_src_to_ref_homography = new i2i_homog_pad("finalized_src_to_ref_homography_ftsp_out_pad");
    pad_finalized_src_to_utm_homography = new i2u_homog_pad( "finalized_src_to_utm_homography_ftsp_out_pad" );
    pad_finalized_ref_to_wld_homography = new i2p_homog_pad( "finalized_ref_to_wld_homography_ftsp_out_pad" );
    pad_finalized_world_units_per_pixel = new gsd_pad ( "finalized_world_units_per_pixel_ftsp_out_pad" );
    pad_finalized_video_modality = new video_modality_pad ( "finalized_video_modality_ftsp_out_pad" );
    pad_finalized_shot_break_flags = new shot_break_flags_pad ( "finalized_shot_break_flags_ftsp_out_pad" );

    proc_stab_sp = new stabilization_super_process< PixType >( "stab_sp" );
    config.add_subblock( proc_stab_sp->params(), proc_stab_sp->name() );

    proc_kwa_writer = new kw_archive_writer_process ("kwa_writer");
    config.add_subblock( proc_kwa_writer->params(), proc_kwa_writer->name() );

    proc_world_coord_sp = new world_coord_super_process< PixType >( "world_coord_sp" );
    config.add_subblock( proc_world_coord_sp->params(), proc_world_coord_sp->name() );

    proc_dtsp = new detect_and_track_super_process< PixType >( "detect_and_track_sp" );
    config.add_subblock( proc_dtsp->params(), proc_dtsp->name() );

    proc_data_write = new tagged_data_writer_process("tagged_data_writer");
    config.add_subblock( proc_data_write->params(), proc_data_write->name() );

    // Parameter to "hide" from user because they will be over-written by this process.
    dependency_config.add( proc_stab_sp->name() + ":masking_enabled", "false" );
    dependency_config.add( proc_world_coord_sp->name() + ":masking_enabled", "false" );
    dependency_config.add( proc_dtsp->name() + ":masking_enabled", "false" );
  }


// ----------------------------------------------------------------
/** Setup pipeline.
 *
 * This method adds all processes to the pipeline and connects them in
 * the correct order.
 */
template < class PIPELINE >
  void setup_pipeline( PIPELINE* p )
  {
    // input pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_metadata );

    // processes
    p->add( proc_stab_sp );
    p->add( proc_world_coord_sp );
    p->add( proc_dtsp );

    // output pads
    p->add( pad_finalized_timestamp );
    p->add( pad_finalized_tracks );
    p->add( pad_finalized_src_to_ref_homography );
    p->add( pad_finalized_src_to_utm_homography );
    p->add( pad_finalized_ref_to_wld_homography );
    p->add( pad_finalized_world_units_per_pixel );
    p->add( pad_finalized_video_modality );
    p->add( pad_finalized_shot_break_flags );

    // Input Pad connections -> stab
    p->connect( pad_source_image->value_port(),       proc_stab_sp->set_source_image_port() );
    p->connect( pad_source_timestamp->value_port(),   proc_stab_sp->set_source_timestamp_port() );
    p->connect( pad_source_metadata->value_port(),    proc_stab_sp->set_source_metadata_port() );

    // connect stab -> world-coord
    p->connect( proc_stab_sp->output_metadata_vector_port(),      proc_world_coord_sp->set_source_metadata_vector_port() );
    p->connect( proc_stab_sp->source_image_port(),                proc_world_coord_sp->set_source_image_port() );
    p->connect( proc_stab_sp->source_timestamp_port(),            proc_world_coord_sp->set_source_timestamp_port() );
    p->connect( proc_stab_sp->src_to_ref_homography_port(),       proc_world_coord_sp->set_src_to_ref_homography_port() );
    p->connect( proc_stab_sp->get_output_timestamp_vector_port(), proc_world_coord_sp->set_input_timestamp_vector_port() );
    p->connect( proc_stab_sp->get_output_shot_break_flags_port(), proc_world_coord_sp->set_input_shot_break_flags_port() );
    p->connect( proc_stab_sp->get_mask_port(),                    proc_world_coord_sp->set_mask_port() );

    // connect world-coord -> dtsp
    // these connections are optional to support extra cycles in da_tracker_process running under sync mode.
    p->connect_optional( proc_world_coord_sp->source_timestamp_port(),                 proc_dtsp->set_input_timestamp_port() );
    p->connect_optional( proc_world_coord_sp->source_image_port(),                     proc_dtsp->set_input_image_port() );
    p->connect_optional( proc_world_coord_sp->src_to_ref_homography_port(),            proc_dtsp->set_input_src_to_ref_homography_port() );
    p->connect_optional( proc_world_coord_sp->src_to_wld_homography_port(),            proc_dtsp->set_input_src_to_wld_homography_port() );
    p->connect_optional( proc_world_coord_sp->wld_to_src_homography_port(),            proc_dtsp->set_input_wld_to_src_homography_port() );
    p->connect_optional( proc_world_coord_sp->wld_to_utm_homography_port(),            proc_dtsp->set_input_wld_to_utm_homography_port() );
    p->connect_optional( proc_world_coord_sp->get_output_ref_to_wld_homography_port(), proc_dtsp->set_input_ref_to_wld_homography_port() );
    p->connect_optional( proc_world_coord_sp->get_output_timestamp_vector_port(),      proc_dtsp->set_input_timestamp_vector_port() );
    p->connect_optional( proc_world_coord_sp->world_units_per_pixel_port(),            proc_dtsp->set_input_world_units_per_pixel_port() );
    p->connect_optional( proc_world_coord_sp->get_output_shot_break_flags_port(),      proc_dtsp->set_input_shot_break_flags_port() );
    p->connect_optional( proc_world_coord_sp->get_output_mask_port(),                  proc_dtsp->set_input_mask_port() );

    // add tap for tagged data writer
    bool pmw_enable;
    config.get(proc_data_write->name() + ":enabled", pmw_enable);
    if (pmw_enable)
    {
      p->add ( proc_data_write );

      p->connect( proc_world_coord_sp->source_timestamp_port(),                 proc_data_write->set_input_timestamp_port() );
      proc_data_write->set_timestamp_connected (true);

      p->connect( proc_world_coord_sp->source_image_port(),                     proc_data_write->set_input_image_port() );
      proc_data_write->set_image_connected (true);

      p->connect( proc_world_coord_sp->src_to_ref_homography_port(),            proc_data_write->set_input_src_to_ref_homography_port() );
      proc_data_write->set_src_to_ref_homography_connected (true);

      p->connect( proc_world_coord_sp->src_to_wld_homography_port(),            proc_data_write->set_input_src_to_wld_homography_port() );
      proc_data_write->set_src_to_wld_homography_connected (true);

      p->connect( proc_world_coord_sp->wld_to_src_homography_port(),            proc_data_write->set_input_wld_to_src_homography_port() );
      proc_data_write->set_wld_to_src_homography_connected (true);

      p->connect( proc_world_coord_sp->wld_to_utm_homography_port(),            proc_data_write->set_input_wld_to_utm_homography_port() );
      proc_data_write->set_wld_to_utm_homography_connected (true);

      p->connect( proc_world_coord_sp->get_output_ref_to_wld_homography_port(), proc_data_write->set_input_ref_to_wld_homography_port() );
      proc_data_write->set_ref_to_wld_homography_connected (true);

      // p->connect( proc_world_coord_sp->get_output_timestamp_vector_port(),      proc_data_write->set_input_timestamp_vector_port() );
      p->connect( proc_world_coord_sp->world_units_per_pixel_port(),            proc_data_write->set_input_world_units_per_pixel_port() );
      proc_data_write->set_world_units_per_pixel_connected (true);

      p->connect( proc_world_coord_sp->get_output_shot_break_flags_port(),      proc_data_write->set_input_shot_break_flags_port() );
      proc_data_write->set_shot_break_flags_connected (true);

      p->connect( proc_world_coord_sp->get_output_mask_port(), proc_data_write->set_input_image_mask_port() );
      proc_data_write->set_image_mask_connected (true);
    }


    // Add a tap here for kwa writer
    bool kwa_disable;
    config.get(proc_kwa_writer->name() + ":disabled", kwa_disable);
    if ( ! kwa_disable)
    {
      LOG_INFO ("FTSP: kwa writer is enabled");
      p->add( proc_kwa_writer );

      p->connect( proc_world_coord_sp->source_timestamp_port(),      proc_kwa_writer->set_input_timestamp_port() );
      p->connect( proc_world_coord_sp->source_image_port(),          proc_kwa_writer->set_input_image_port() );
      p->connect( proc_world_coord_sp->src_to_ref_homography_port(), proc_kwa_writer->set_input_src_to_ref_homography_port() );
      p->connect( proc_world_coord_sp->wld_to_utm_homography_port(), proc_kwa_writer->set_input_wld_to_utm_homography_port() );
      p->connect( proc_world_coord_sp->src_to_wld_homography_port(), proc_kwa_writer->set_input_src_to_wld_homography_port() );
      p->connect( proc_world_coord_sp->world_units_per_pixel_port(), proc_kwa_writer->set_input_world_units_per_pixel_port() );
    }

    // connect dtsp -> Output Pads
    p->connect ( proc_dtsp->get_output_active_tracks_port(),         pad_finalized_tracks->set_value_port() );
    p->connect ( proc_dtsp->get_output_timestamp_port(),             pad_finalized_timestamp->set_value_port() );
    p->connect ( proc_dtsp->get_output_src_to_ref_homography_port(), pad_finalized_src_to_ref_homography->set_value_port() );
    p->connect ( proc_dtsp->get_output_src_to_utm_homography_port(), pad_finalized_src_to_utm_homography->set_value_port() );
    p->connect ( proc_dtsp->get_output_ref_to_wld_homography_port(), pad_finalized_ref_to_wld_homography->set_value_port() );
    p->connect ( proc_dtsp->get_output_world_units_per_pixel_port(), pad_finalized_world_units_per_pixel->set_value_port() );
    p->connect ( proc_dtsp->get_output_video_modality_port(),        pad_finalized_video_modality->set_value_port() );
    p->connect ( proc_dtsp->get_output_shot_break_flags_port(),      pad_finalized_shot_break_flags->set_value_port() );

  }
};


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
template< class PixType>
full_tracking_super_process< PixType >
::full_tracking_super_process( vcl_string const& name )
: super_process( name, "full_tracking_super_process" ),
  videoname_prefixing_done_(false),
  impl_( NULL ),
  wall_clock_()
{
  impl_ = new full_tracking_super_process_impl<PixType>;
}


template< class PixType>
full_tracking_super_process< PixType >
::~full_tracking_super_process()
{
  // check for timing output option
  if (impl_->print_timing_tags)
  {
    std::ofstream timing_file( "pipeline_timing.txt" );
    if (! timing_file)
    {
      LOG_ERROR ("Could not open process timing file");
    }
    else
    {
      pipeline_->output_timing_measurements(timing_file);
      timing_file.close();
    }
  }

  delete impl_;
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
full_tracking_super_process< PixType >
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  tmp_config.remove( impl_->dependency_config );
  return tmp_config;
}


// ----------------------------------------------------------------
/** Accept parameter values.
 *
 * This method accepts the supplied parameter values.
 */
template< class PixType > bool
full_tracking_super_process< PixType >
::set_params(config_block const& blk)
{
  try
  {
    // Update parameters for this process
    bool run_async = false;
    blk.get( "run_async", run_async );

    blk.get( "print_timing_tags", impl_->print_timing_tags);

    bool masking_enabled = false;
    blk.get( "masking_enabled", masking_enabled );
    impl_->dependency_config.set( impl_->proc_stab_sp->name() + ":masking_enabled",
                                  masking_enabled );
    impl_->dependency_config.set( impl_->proc_world_coord_sp->name() + ":masking_enabled",
                                  masking_enabled );
    impl_->dependency_config.set( impl_->proc_dtsp->name() + ":masking_enabled",
                                  masking_enabled );
    impl_->config.update( impl_->dependency_config );

    // Update our local copy of the config with what we are given.
    impl_->config.update( blk );

    if( run_async )
    {
      LOG_INFO("Starting async full_tracking_super_process");
      async_pipeline* p = new async_pipeline(async_pipeline::ENABLE_STATUS_FOWARDING);
      pipeline_ = p;
      impl_->setup_pipeline( p );
    }
    else
    {
      LOG_INFO("Starting sync full_tracking_super_process");
      sync_pipeline* p = new sync_pipeline;
      pipeline_ = p;
      impl_->setup_pipeline( p );
    }

    impl_->config.set( impl_->proc_dtsp->name() + ":run_async", run_async );
    impl_->config.set( impl_->proc_stab_sp->name() + ":run_async", run_async );

    // Set videoname prefix onto KWA otuput file
    vcl_string kwa_dir;
    blk.get( impl_->proc_kwa_writer->name() + ":file_path", kwa_dir );
    kwa_dir += "/" + get_videoname_prefix();
    impl_->config.set( impl_->proc_kwa_writer->name() + ":file_path", kwa_dir );

    if( !this->get_videoname_prefix().empty() )
    {
      if (videoname_prefixing_done_)
      {
        throw unchecked_return_value(" set_params unexpectedly called more than once.");
      }

      videoname_prefixing_done_ = true;

      // Update parameters that have video name prefix.
      // Note updating is done in the returned copy of the config, not in the saved copy.
      add_videoname_prefix( impl_->config, impl_->proc_data_write->name() + ":filename" );
    }

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw unchecked_return_value(" unable to set pipeline parameters.");
    }
  }
  catch(unchecked_return_value & e)
  {
    LOG_ERROR(this->name() << ": couldn't set parameters: " << e.what());

    return ( false );
  }

  // This call must be after pipeline_->set_params() above.
  this->set_print_detailed_report( false );

  return ( true );
} /* set_params */



// ----------------------------------------------------------------
/** Initialize the pipeline.
 *
 *
 */
template< class PixType >
bool
full_tracking_super_process< PixType >
::initialize()
{
  return pipeline_->initialize();
}


// ----------------------------------------------------------------
/** Process step method.
 *
 * This method is not called if we are running an async pipeline.
 */
template< class PixType >
process::step_status
full_tracking_super_process< PixType >
::step2()
{
  return pipeline_->execute();
}


// ----------------------------------------------------------------
/** Post step hook.
 *
 * Called after each step to log current state (in both sync and
 * async) This function is called after step in the sync pipeline and
 * after all outputs are available in the async pipeline.  Log
 * statements should go here instead of in \c step() because \c step()
 * is not called in the async case.
 *
 */
template< class PixType >
void
full_tracking_super_process< PixType >
::post_step()
  {
  ++impl_->step_count;
  timestamp t = impl_->pad_finalized_timestamp->value();
  double seconds = wall_clock_.real() / 1000.0;

  if( !t.is_valid() )
  {
    LOG_DEBUG("==========================================================================\n"
              << "Frame processing done, finalized timestamp not valid.\n"
              << "   number of steps = " << impl_->step_count << "\n"
              << "   wall clock time = " << seconds << " seconds\n"
              << "   wall clock speed = "
              << impl_->step_count / seconds<< " steps/second\n"
              << "   " << impl_->pad_finalized_tracks->value().size()
              << " Finalized track(s)\n"
              << "==========================================================================");
  }
  else
  {
    LOG_DEBUG("==========================================================================\n"
              << "Frame processing done, finalized time = "
             << vul_sprintf("%.3f",t.has_time()?t.time()/1e6:-1.0)
             << " (frame=" << (t.has_frame_number()?t.frame_number():-1)
              << ")\n"
              << "   number of steps = " << impl_->step_count << "\n"
              << "   wall clock time = " << seconds << " seconds\n"
              << "   wall clock speed = "
              << impl_->step_count / seconds << " steps/second\n"
              << "   " << impl_->pad_finalized_tracks->value().size()
              << " Finalized track(s)\n"
              << "==========================================================================");
  }
}


// ---------- utility methods ---------
template< class PixType >
void
full_tracking_super_process< PixType >
::set_multi_features_dir( vcl_string const & dir )
{
  impl_->proc_dtsp->set_multi_features_dir( dir );
}


template< class PixType >
void
full_tracking_super_process< PixType >
::set_guis( process_smart_pointer< gui_process > ft_gui,
            process_smart_pointer< gui_process > bt_gui )
{
  impl_->proc_dtsp->set_guis( ft_gui, bt_gui );
}


template< class PixType >
void
full_tracking_super_process< PixType >
::set_config_data( vcl_string const & vidname )
{
  set_videoname_prefix (vidname);
}


// ----------- input methods ------------
template< class PixType >
void
full_tracking_super_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  impl_->pad_source_image->set_value( img );
}


template< class PixType >
void
full_tracking_super_process< PixType >
::set_source_timestamp( timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}


template< class PixType >
void
full_tracking_super_process< PixType >
::set_source_metadata( vidtk::video_metadata const& md )
{
  impl_->pad_source_metadata->set_value( md );
}


// ----------- output methods ------------
template< class PixType >
timestamp const&
full_tracking_super_process< PixType >
::finalized_timestamp() const
{
  return impl_->pad_finalized_timestamp->value();
}


template< class PixType >
vcl_vector< track_sptr > const&
full_tracking_super_process< PixType >
::finalized_tracks() const
{
  return impl_->pad_finalized_tracks->value();
}


template< class PixType >
image_to_image_homography const&
full_tracking_super_process< PixType >
::finalized_src_to_ref_homography() const
{
  return impl_->pad_finalized_src_to_ref_homography->value();
}


template< typename PixType >
image_to_utm_homography const&
full_tracking_super_process< PixType >
::finalized_src_to_utm_homography() const
{
  return impl_->pad_finalized_src_to_utm_homography->value();
}


template< typename PixType >
double
full_tracking_super_process< PixType >
::get_output_world_units_per_pixel () const
{
  return impl_->pad_finalized_world_units_per_pixel->value();
}


template< class PixType >
vcl_vector< track_sptr > const&
full_tracking_super_process< PixType >
::linked_tracks() const
{
  return impl_->proc_dtsp->get_output_linked_tracks();
}


template< class PixType >
vcl_vector< track_sptr > const&
full_tracking_super_process< PixType >
::filtered_tracks() const
{
  return impl_->proc_dtsp->get_output_filtered_tracks();
}



template< class PixType >
vidtk::video_modality
full_tracking_super_process< PixType >
::get_output_video_modality() const
{
  return impl_->pad_finalized_video_modality->value();
}


template< class PixType >
vidtk::shot_break_flags
full_tracking_super_process< PixType >
::get_output_shot_break_flags() const
{
  return impl_->pad_finalized_shot_break_flags->value();
}

} // namespace vidtk
