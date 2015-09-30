/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/world_coord_super_process.h>

#include <geographic/geo_coords.h>
#include <pipeline/sync_pipeline.h>
#include <tracking/gen_world_units_per_pixel_process.h>
#include <tracking/shot_break_flags.h>
#include <tracking/orthorectified_metadata_source_process.h>

#include <utilities/video_metadata.h>
#include <utilities/compute_gsd.h>
#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/transform_vidtk_homography_process.h>
#include <utilities/queue_process.h>
#include <utilities/compute_transformations.h>
#include <utilities/homography_holder_process.h>
#include <utilities/filter_video_metadata_process.h>


#include <video/visualize_world_super_process.h>

#include <vcl_sstream.h>
#include <boost/foreach.hpp>
#include <vcl_iomanip.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER ("world_coord_super_process");

// ----------------------------------------------------------------
/** World coordinate Super Process implementation.
 *
 * Inputs:
 * - source timestamp - current timestamp (queued)
 * - source image - current image (queued)
 * - src2ref homography - transform to reference frame (queued)
 * - source metadata - metadata for current frame
 *
 * Outputs:
 * - source timestamp -
 * - source image -
 * - src2ref homography -
 * - src2world homography -
 * - world2src homography -
 * - world2utm homography -
 * - world units per pixel -
 *
 * Most of the inputs (as noted above) are entered into queues as they
 * are received. This leaves it to us to manage the queues in this
 * process.
 *
 * The process starts in buffering-mode where the inptus are buffered
 * until usable metadata is found. Once it is found, the homographies
 * are computed and we turn off buffering mode and enable buffered
 * data to be sent down stream. All buffered data is immediately
 * flushed downstream
 *
 * While not in buffering-mode, the inputs are queued (buffered) and
 * outputs are send down stream. This is the one-in-one-out mode and
 * since the buffers were emptied when good metadata was found, our
 * buffer usage is just one element at this point.
 *
 * This process goes back into buffering mode when there is an end of
 * shot detected.
 *
 * Note: This process uses the validity of corner points from the
 * metadata as the indication of valid metadata. If the raw video deos
 * not contain valid corner points, then they will have to be derived
 * by another process upstream of this one.
 *
 * @todo Add state chart/diagram.
 */
template< class PixType>
class world_coord_super_process_impl
{
public:
  typedef transform_vidtk_homography_process< timestamp, timestamp,
                                              timestamp, plane_ref_t >
          transform_img2gnd_ptype;

  // Pipeline processes
  process_smart_pointer< filter_video_metadata_process > proc_filter_meta;
  process_smart_pointer< queue_process< vil_image_view< PixType > > > proc_src_img_q;
  process_smart_pointer< queue_process< timestamp > > proc_src_ts_q;
  process_smart_pointer< queue_process< image_to_image_homography > > proc_src2ref_homog_q;
  process_smart_pointer< queue_process< timestamp::vector_t > > proc_src_ts_vector_q;
  process_smart_pointer< queue_process< vil_image_view<bool> > > proc_src_mask_q;
  process_smart_pointer< queue_process< vidtk::shot_break_flags > > proc_shot_break_flags_q;
  process_smart_pointer< transform_img2gnd_ptype > proc_trans_for_ground;
  process_smart_pointer< gen_world_units_per_pixel_process< PixType > > proc_gen_gsd;
  process_smart_pointer< visualize_world_super_process< PixType > > proc_vis_wld_sp;

  process_smart_pointer< homography_holder_process > proc_fixed_md_src;
  process_smart_pointer< orthorectified_metadata_source_process< PixType> > proc_ortho_md_src;

  // Configuration parameters
  config_block config;
  config_block default_config;
  enum mode{ VIDTK_METADATA_FIXED, VIDTK_METADATA_STREAM, VIDTK_METADATA_ORTHO };
  mode metadata_type;

  bool apply_hd_correction;
  double hd_to_sd_hfov_correction_1;
  double hd_to_sd_hfov_correction_2;
  double hd_to_sd_hfov_angle;

  vidtk::timestamp curr_timestamp;
  vidtk::image_to_image_homography curr_src2ref;
  vidtk::shot_break_flags m_in_shot_break_flags;

  double min_allowed_gsd;
  double fov_change_threshold;
  unsigned min_md_ignore_count;
  bool buffer_mode;
  bool using_preselected_packet;
  bool end_of_video;
  unsigned ignore_count;
  vcl_vector< video_metadata > curr_md;
  timestamp::vector_t m_saved_ts;

  plane_to_utm_homography H_wld2utm;
  image_to_plane_homography H_ref2wld;
  vil_image_view< PixType > curr_img;

  double fallback_gsd;
  bool enable_fallback;
  unsigned int min_size_for_fallback;
  video_metadata fallback_meta_data_packet;
  unsigned int fallback_height;
  unsigned int fallback_width;
  bool masking_enabled;

  world_coord_super_process < PixType > * m_parent;  // pointer to process object


  //------------------------------------------------------------------------

  explicit world_coord_super_process_impl( world_coord_super_process < PixType > * parent)
  : proc_filter_meta( NULL ),
    proc_src_img_q( NULL ),
    proc_src_ts_q( NULL ),
    proc_src2ref_homog_q( NULL ),
    proc_src_ts_vector_q( NULL ),
    proc_src_mask_q( NULL ),
    proc_shot_break_flags_q ( NULL ),

    proc_trans_for_ground( NULL ),
    proc_gen_gsd( NULL ),
    proc_vis_wld_sp( NULL ),
    proc_fixed_md_src( NULL ),
    proc_ortho_md_src( NULL ),

    metadata_type( VIDTK_METADATA_FIXED ),

    apply_hd_correction( false ),
    hd_to_sd_hfov_correction_1( 0.375 ),
    hd_to_sd_hfov_correction_2( 0.5 ),
    hd_to_sd_hfov_angle( 1.59 ),

    end_of_video( false ),
    m_saved_ts(),
    fallback_gsd( 1 ),
    enable_fallback( true ),
    min_size_for_fallback(0),
    masking_enabled( false ),
    m_parent( parent )
  {
    this->create_process_configs();
    end_of_shot_reset();
    fallback_meta_data_packet = video_metadata();
    fallback_meta_data_packet.is_valid( false );
  }


// ----------------------------------------------------------------
/** Reset input data areas
 *
 * Reset iput data areas to initial/invalid values.
 */
  void reset_inputs()
  {
    curr_img = vil_image_view< PixType >();
    curr_timestamp = vidtk::timestamp();
    curr_src2ref.set_valid(false);
  }


// ----------------------------------------------------------------
/** reset at end of shot.
 *
 * Reset all internal values back to initial/invalid state.
 * Also switch to buffering mode
 */
  void end_of_shot_reset()
  {
    buffer_mode = true;
    using_preselected_packet = false;
    end_of_video = false;
    ignore_count = 0;

    if( metadata_type == VIDTK_METADATA_STREAM )
    {
      // We only want to do this for "STREAM" type, because in other (FIXED &
      // ORTHO) types we are currently always keeping the queue sizes to 1.

      proc_src_img_q->set_disable_read( true );
      proc_src_ts_q->set_disable_read( true );
      proc_src2ref_homog_q->set_disable_read( true );
      proc_src_ts_vector_q->set_disable_read( true );
      proc_src_mask_q->set_disable_read( true );
      proc_shot_break_flags_q->set_disable_read( true );
    }

    H_ref2wld.set_identity( false );

    proc_src_img_q->clear();
    proc_src_ts_q->clear();
    proc_src_ts_vector_q->clear();
    proc_src_mask_q->clear();
    proc_src2ref_homog_q->clear();
    proc_shot_break_flags_q->clear();
  }

  //------------------------------------------------------------------------

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    proc_filter_meta = new filter_video_metadata_process("metadata_filter");
    config.add_subblock( proc_filter_meta->params(), proc_filter_meta->name() );

    proc_src_img_q = new queue_process< vil_image_view< PixType > >( "src_img_queue" );
    config.add_subblock( proc_src_img_q->params(), proc_src_img_q->name() );

    proc_src_ts_q = new queue_process< timestamp >( "src_timestamp_queue" );
    config.add_subblock( proc_src_ts_q->params(), proc_src_ts_q->name() );

    proc_src2ref_homog_q = new queue_process< image_to_image_homography >( "src2ref_homog_queue" );
    config.add_subblock( proc_src2ref_homog_q->params(), proc_src2ref_homog_q->name() );

    proc_src_ts_vector_q = new queue_process< timestamp::vector_t >("src_timestamp_vector_queue");
    config.add_subblock( proc_src_ts_vector_q->params(), proc_src_ts_vector_q->name() );

    proc_src_mask_q = new queue_process< vil_image_view<bool> >("src_mask_queue");
    config.add_subblock( proc_src_mask_q->params(), proc_src_mask_q->name() );

    proc_shot_break_flags_q = new queue_process< vidtk::shot_break_flags >("shot_break_flags_queue");
    config.add_subblock( proc_shot_break_flags_q->params(), proc_shot_break_flags_q->name() );

    proc_trans_for_ground = new transform_img2gnd_ptype( "trans_for_ground" );
    config.add_subblock( proc_trans_for_ground->params(), proc_trans_for_ground->name() );

    proc_gen_gsd =  new gen_world_units_per_pixel_process< PixType >( "gen_gsd" );
    config.add_subblock( proc_gen_gsd->params(), proc_gen_gsd->name() );

    proc_vis_wld_sp = new visualize_world_super_process< PixType >( "visualize_world_sp" );
    config.add_subblock( proc_vis_wld_sp->params(), proc_vis_wld_sp->name() );

    proc_fixed_md_src = new homography_holder_process( "fixed_metadata" );
    config.add_subblock( proc_fixed_md_src->params(), proc_fixed_md_src->name() );

    proc_ortho_md_src = new orthorectified_metadata_source_process< PixType >( "ortho_metadata" );
    config.add_subblock( proc_ortho_md_src->params(), proc_ortho_md_src->name() );

    default_config.add( proc_src_img_q->name() + ":max_length", "512" );
    default_config.add( proc_src_ts_q->name() + ":max_length", "512" );
    default_config.add( proc_src_ts_vector_q->name() + ":max_length", "512" );
    default_config.add( proc_src2ref_homog_q->name() + ":max_length", "512" );
    default_config.add( proc_src_mask_q->name() + ":max_length", "512" );
    default_config.add( proc_shot_break_flags_q->name() + ":max_length", "512" );

    default_config.add( proc_vis_wld_sp->name() + ":disabled", "true" );
    default_config.add( proc_gen_gsd->name() + ":disabled", "false" );

    config.update( default_config );
  }


// ----------------------------------------------------------------
/** Connenct pipeline.
 *
 * This method creates a new pipeline and connects all the processes.
 *
 * @param[out] pp - pointer to the newly created pipeline.
 */
  void setup_pipeline( pipeline_sptr & pp )
  {
    sync_pipeline* p = new sync_pipeline();
    pp = p;

    p->add_without_execute( proc_filter_meta ); // needed to get config set
    p->add( proc_src_ts_q );
    p->add( proc_src_ts_vector_q );
    p->add( proc_src_img_q );
    p->add( proc_src2ref_homog_q );
    p->add( proc_shot_break_flags_q );

    if( masking_enabled )
    {
      p->add( proc_src_mask_q );
    }

    p->add( proc_trans_for_ground );
    p->connect( proc_src2ref_homog_q->get_output_datum_port(),
                proc_trans_for_ground->set_source_homography_port() );

    p->add( proc_gen_gsd );
    p->connect( proc_src_img_q->get_output_datum_port(),
                proc_gen_gsd->set_image_port() );

    if( this->metadata_type == VIDTK_METADATA_ORTHO )
    {
      p->add( proc_ortho_md_src );
      p->connect( proc_src_img_q->get_output_datum_port(),
                  proc_ortho_md_src->set_source_image_port() );

      p->connect( proc_ortho_md_src->ref_to_wld_homography_port(),
                  proc_trans_for_ground->set_premult_homography_port() );

      p->connect( proc_ortho_md_src->ref_to_wld_homography_port(),
                  proc_gen_gsd->set_source_vidtk_homography_port() );
    }
    else if( this->metadata_type == VIDTK_METADATA_FIXED )
    {
      p->add( proc_fixed_md_src );
      p->connect( proc_fixed_md_src->homography_ref_to_wld_port(),
                  proc_trans_for_ground->set_premult_homography_port() );

      p->connect( proc_fixed_md_src->homography_ref_to_wld_port(),
                  proc_gen_gsd->set_source_vidtk_homography_port() );
    }

    /*
    // Incoming edges into proc_vis_wld_sp
    p->add( proc_vis_wld_sp );
    p->connect( proc_src_img_q->get_output_datum_port(),    proc_vis_wld_sp->set_source_image_port() );
    p->connect( proc_trans_for_ground->homography_port(),   proc_vis_wld_sp->set_src_to_wld_homography_port() );
    p->connect( proc_src_ts_q->get_output_datum_port(),     proc_vis_wld_sp->set_source_timestamp_port() );
    p->connect( proc_gen_gsd->world_units_per_pixel_port(), proc_vis_wld_sp->set_world_units_per_pixel_port() );
    */
  } // setup_pipeline()


// ----------------------------------------------------------------
/** Main pipeline processing.
 *
 * State machine description:
 *
 * s0 - (start state) looking for good metadata
 * s1 - passing input to output, waiting for end of shot
 *
 * s0 -> s1 :: when metadata is usable or meet fallback mode criteria
 * s1 -> s0 :: when end of shot is detected
 *
 * Metadata is usable when
 * 1) using a preselected metadata packet.
 * 2) -or- current metadata packed is valid and have ignored enough md packets
 */
  process::step_status
  step2( pipeline_sptr p )
  {

    // All queues must be the same length
    assert (proc_src_ts_q->length() == proc_src_img_q->length() );
    assert (proc_src_ts_q->length() == proc_src_ts_vector_q->length() );
    assert (proc_src_ts_q->length() == proc_src2ref_homog_q->length() );
    assert (proc_src_ts_q->length() == proc_shot_break_flags_q->length() );
    if( masking_enabled )
    {
      assert (proc_src_ts_q->length() == proc_src_mask_q->length() );
    }

    // Check if this is the end of the video
    if( !curr_img )
    {
      LOG_INFO( m_parent->name() << ": End of Video Detected" );
      end_of_video = true;
    }

    // Is this the first frame observed?
    static bool first_frame = true;

    // Return status of process
    process::step_status p_status (process::SUCCESS);

    // If it's not the end of the video, step the internal pipeline
    if( !end_of_video )
    {
      if( metadata_type == VIDTK_METADATA_STREAM )
      {
        // look for new homography reference.
        if( curr_src2ref.is_new_reference() )
        {
          LOG_INFO ( m_parent->name() << ": New src2ref reference at "
                     << curr_timestamp << "\n"
                     << curr_src2ref);
        }
        else
        {
          LOG_INFO (m_parent->name() << ": shot break flags: "
                    << m_in_shot_break_flags << " @ "
                    << curr_timestamp);
        }

        // Lagging data.

        // Check the timestamp vector queue to see if it is about to overflow.
        // If an element is about to be lost, save the poor little fella.
        if (proc_src_ts_vector_q->get_max_length() == proc_src_ts_vector_q->length())
        {
          timestamp::vector_t tsv = proc_src_ts_vector_q->datum_at(0);

          BOOST_FOREACH (timestamp ts, tsv)
          {
            m_saved_ts.push_back (ts);
          }

          LOG_INFO("world_coord_super_process: tsv queue overflow - saving " <<  tsv.size()
                   << " entry(s),  total saved: " << m_saved_ts.size());
        }

        // Supply homography to use for calculating outputs.  These are
        // process inputs (normally supplied by pipeline).  Only needed
        // for VIDTK_METADATA_STREAM mode. All other modes have ports
        // connected to pipeline.
        proc_trans_for_ground->set_premult_homography( this->H_ref2wld );
        proc_gen_gsd->set_source_vidtk_homography( this->H_ref2wld );

        // We need to call the filter step explicitly here becuase we need
        // the results applied to the input only, and we need them now!
        // If this was added to the internal pipeline, it would also be
        // called when we pump the outputs (not good).
        proc_filter_meta->step();
      }
      else
      {
        // No lag => queues are in one-in one-out mode.
        if( first_frame )
        {
          proc_src_img_q->set_disable_read( false );
          proc_src_ts_q->set_disable_read( false );
          proc_src_ts_vector_q->set_disable_read( false );
          proc_src2ref_homog_q->set_disable_read( false );
          proc_src_mask_q->set_disable_read( false );
          proc_shot_break_flags_q->set_disable_read( false );
        }
      }

      // Explicitly step the pipeline to cycle the buffers et. al.
      p_status = p->execute();
      if( p_status == process::FAILURE )
      {
        reset_inputs();
        return p_status;
      }
    }

    // Buffer data, ending buffer mode if metadata is usable or criteria for
    // fallback mode is satisfied
    if( (this->metadata_type == VIDTK_METADATA_STREAM)
        && this->buffer_mode )
    {
      LOG_INFO (m_parent->name() << ": buffering, not passing data downstream...");

      // Collect filtered metadata (if any). This is the output of the
      // md-filter process.  It may not produce any output if the
      // meta-data is bad.
      curr_md = proc_filter_meta->output_metadata_vector();

      p_status = process::SKIP; // default for buffering mode

      if( this->is_metadata_usable() || this->use_fallback() )
      {
        LOG_INFO (m_parent->name() << ": metadata is now usable at frame " << curr_timestamp);

        // Now the metadata is good enough, so we will use it and start pushing
        // the data out to the downstream nodes.
        this->buffer_mode = false;

        // If we have saved some timestamp vectors.
        // This check triggers only once so we can make one really long vector
        // of stamps the first cycle after a reset.
        if (m_saved_ts.size() > 0)
        {
          // Add our saved timestamp vector to the oldest entry in the
          // current ts vector queue. (front() has the oldest entry)
          // (1) is proc_src_ts_vector_q empty ?
          // (2) is proc_src_ts_vector_q.front() empty ?
          if (proc_src_ts_vector_q->get_data_store().size() == 0)
          {
            // ts vector queue empty -
            LOG_ERROR ("world_coord_super_process: ts vector empty - unexpected");
          }
          else
          {
            timestamp::vector_t * first_v = &proc_src_ts_vector_q->get_data_store().front();
            first_v->insert( first_v->begin(), m_saved_ts.begin(), m_saved_ts.end() );

            LOG_DEBUG ("world_coord_super_process: restored entry has "
                       << first_v->size()
                       << " entries");
          }

          // erase our saved list so we only do this once after a reset.
          m_saved_ts.erase (m_saved_ts.begin(), m_saved_ts.end());

        } // end saved timestamp vector

        // Queues can now start poping the data out.
        proc_src_img_q->set_disable_read( false );
        proc_src_ts_q->set_disable_read( false );
        proc_src_ts_vector_q->set_disable_read( false );
        proc_src2ref_homog_q->set_disable_read( false );
        proc_src_mask_q->set_disable_read( false );
        proc_shot_break_flags_q->set_disable_read( false );

        LOG_INFO ( m_parent->name() << " -> flushing " << proc_src_ts_q->length() << " queued elements" );

        // Flush our buffers.
        p_status = flush_buffers(p);
      }
    } // end buffer_mode
    else
    {
      LOG_INFO ( m_parent->name() << ": not buffering, passing the data downstream - " << curr_timestamp );

      // Set homography
      if( this->metadata_type == VIDTK_METADATA_ORTHO && first_frame )
      {
        this->H_wld2utm = proc_ortho_md_src->wld_to_utm_homography();
      }

      // Set metadata packet for possible fallback mode if it is valid
      if( this->metadata_type == VIDTK_METADATA_STREAM && enable_fallback )
      {
        for( unsigned i = 0; i < curr_md.size(); i++ )
        {
          if( curr_md[i].is_valid()  )
          {
            fallback_meta_data_packet = curr_md[i];
            fallback_width = curr_img.ni();
            fallback_height = curr_img.nj();
          }
        }
      }
    }

    if( end_of_video )
    {
      LOG_INFO (m_parent->name() << ": End of video or invalid input causing process termination");
      return process::FAILURE;
    }

    // check for end of shot at this frame
    if( m_in_shot_break_flags.is_shot_end() )
    {
      // May also be able to use homography break flag in input homography.
      LOG_INFO (m_parent->name() << ":metadata is not usable because end of shot detected at " << curr_timestamp
                << "\n\tignoring "<< proc_src_ts_q->length() << " frames");

      // Remember the timestamp_list instead of throwing away
      // Collect all timestamps in the buffer and append to our saved vector.
      // list may be empty.
      BOOST_FOREACH (timestamp::vector_t v, proc_src_ts_vector_q->get_data_store())
      {
        // append current list of timestamps
        m_saved_ts.insert (m_saved_ts.end(),
                           v.begin(), v.end() );
      }

      // Currently we will just throw the buffered frames away and keep
      // buffering. Is this the best thing to do here?
      end_of_shot_reset();
    }

    if( first_frame )
    {
      first_frame = false;
    }

    // indicate we have consumed the input
    reset_inputs();
    return p_status;
  } // step2()



// ----------------------------------------------------------------
/** Flush our buffers.
 *
 * This method flushes all accumulated data from the buffers. Note
 * that there is special handling for last item in buffer.
 *
 * Need to leave the last set of outputs in our slots because they
 * will be consomed by the framework when we return.
 *
 * Assumption is we are in VIDTK_METADATA_STREAM mode.
 *
 * @param[in] p - our pipeline
 *
 * @return The process status is returned indicating success or error.
 */
  process::step_status flush_buffers(pipeline_sptr p)
  {
    process::step_status p_status = process::SUCCESS;

    while (proc_src_ts_q->length() > 0)
    {
      // Need to execute the pipeline because:
      // 1) the queues are processes and need to be stepped.
      // 2) the other processes need to run to create expected output.

      // Supply homography to use for calculating outputs.
      // These are process inputs (normally supplied by pipeline).
      proc_trans_for_ground->set_premult_homography( this->H_ref2wld );
      proc_gen_gsd->set_source_vidtk_homography( this->H_ref2wld );

      // Note that this execute call decrements the queue size.
      p_status = p->execute();
      if( p_status == process::FAILURE )
      {
        LOG_ERROR ("Pipeline execute unexpected return code");
        break;
      }

      // Leave one last set of outputs in the buckets.  Length
      // should be 1 or more at this point, so all we need to do is
      // execute.  The "pump" operation will be done for us.
      if (proc_src_ts_q->length() >= 1)
      {
        m_parent->pump_output();
      }
    } // end while

    return p_status;
  }


// ----------------------------------------------------------------
/** Compute image to world transformations using metadata packet.
 *
 * The reference to world homography is computed and saved in
 * this->H_ref2wld for all to use. This only needs to be done once
 * after a new reference frame is established.
 *
 * @param[in] md - valid video metadata object.
 * @param[in] img_height - width of image metadata was calculated from
 * @param[in] img_width - height of image metadata was calculated from
 *
 * @return \b False if metadata object is not valid or homography can not
 * be computed. \b True if metadata is valid and also compute new
 * H_ref2wld homography.
 */
  bool set_data_related_to_metadata( video_metadata const & md,
                                     unsigned const & img_width,
                                     unsigned const & img_height )
  {
    if(!md.is_valid())
    {
      return false;
    }
    vcl_pair<unsigned, unsigned> img_wh( img_width, img_height );

    video_metadata corrected_md = md;
    if( this->apply_hd_correction )
    {
      LOG_WARN( "Applying correction to HFOV angle to account for HD to SD conversion." );

      // NOTE: This correction is merely a hack based on empirical observations
      // made on four videos of a particular sensor.
      double hfov_angle = (md.sensor_horiz_fov() *  hd_to_sd_hfov_correction_1 );
      const double epsilon = 1e-3;
      if( vcl_abs( md.sensor_horiz_fov() - hd_to_sd_hfov_angle ) < epsilon )
      {
        LOG_WARN( "Applying fixed correction factor to HFOV angle " << hd_to_sd_hfov_angle );
        hfov_angle *= hd_to_sd_hfov_correction_2;
      }
      corrected_md.sensor_horiz_fov( hfov_angle );
    }

    homography::transform_t H_curr2wld;
    if( compute_image_to_geographic_homography( corrected_md,
                                                img_wh,
                                                H_curr2wld,
                                                this->H_wld2utm ) )
    {
      // recompute the video modality (EO/IR and M/N-FOV) for the new shot
      vgl_box_2d<unsigned> img_box( 1, proc_src_img_q->get_output_datum().ni(),
                                    1, proc_src_img_q->get_output_datum().nj() );

      // ref should *not* be the current frame and should be what is on
      // output port of the lag queues. src2ref homography should be used to
      // transfer this reference from the current to that frame.

      unsigned length = proc_src2ref_homog_q->length();
      vgl_h_matrix_2d< double > H_curr2ref = proc_src2ref_homog_q->datum_at( length-1 ).get_transform();
      this->H_ref2wld.set_transform( H_curr2wld * H_curr2ref.get_inverse() );
      this->H_ref2wld.set_source_reference( proc_src_ts_q->datum_at( 0 ) );
      // Now we have new ref frame and associated transformations (H_ref2wld,
      // H_wld2utm)

      double new_gsd = compute_gsd( img_box, this->H_ref2wld.get_transform().get_matrix() );
      if( new_gsd < min_allowed_gsd )
      {
        // If the new GSD is unexpectedly small, then cap it at some predefined value.
        // We will intorduce an internal transformation to the computed-world to
        // used-world==world as far as the tracker is concerned.

        LOG_DEBUG( "md_lag_sp: Choosing to use min_allowed_gsd ("<< min_allowed_gsd
                   << ") over the computed gsd ("<< new_gsd << ")");

        double new_to_allowed_gsd_scaling = min_allowed_gsd / new_gsd;
        new_gsd = min_allowed_gsd;

        vgl_h_matrix_2d< double > H_scale_wld;
        H_scale_wld.set_identity();
        H_scale_wld.set_scale( new_to_allowed_gsd_scaling );

        this->H_ref2wld.set_transform( H_scale_wld * this->H_ref2wld.get_transform() );
        this->H_wld2utm.set_transform( this->H_wld2utm.get_transform() *  H_scale_wld.get_inverse() );
      }

      LOG_DEBUG( "md_lag_sp: Successfully used new metadata packet " << md.timeUTC()
              << " GSD: "<< new_gsd <<" to compute H_ref2wld: " << this->H_ref2wld );

      this->using_preselected_packet = true;

      return true;
    }
    return false;
  }


// ----------------------------------------------------------------
/** Is metadata usable.
 *
 * This method determines if the current metadata is usable. As a side
 * effect of finding usable metadata, the appropriate homographies are
 * computed and passed to another process for further work.
 *
 * @retval true - metadata is usable. The metadata is either good to
 * go, or we are not lagging the video waiting for good metadata.
 *
 * @retval false - metadata is not usable.
 */
  bool is_metadata_usable()
  {
    if( this->using_preselected_packet )
    {
      return true;
    }

    // We only recompute the H_ref2wld in the beginning on a shot.

    for( unsigned i = 0; i < curr_md.size(); i++ ) // size can be zero
    {
      LOG_DEBUG( "is_metadata_usable()- ignore_count: " << ignore_count << " crnrs: "
                  << curr_md[i].has_valid_corners()
                  << " time: "
                 << curr_md[i].timeUTC() );

      // Check to see if the metadata is good
      //                  OR
      // Inspect the metadata packets coming in through the ARIES system and
      // and choose a *suitable* number of packets to skip before we are relatively
      // certain that alignment is reasonable.
      if( !curr_md[i].is_valid()  )
      {
        continue;
      }
      else if( ignore_count++ < min_md_ignore_count )
      {
        fallback_meta_data_packet = curr_md[i];
        fallback_width = curr_img.ni();
        fallback_height = curr_img.nj();
        continue;
      }
      else if ( set_data_related_to_metadata( curr_md[i], curr_img.ni(), curr_img.nj() ) )
      {
        return true;
      } // if compute_homography
      else
      {
        this->H_ref2wld.set_valid( false );
        this->H_wld2utm.set_valid( false );
      }
    } // for curr_md

    return false;
  } // is_metadata_usable()


// ----------------------------------------------------------------
/** Should we use fallback mode.
 *
 * This method determines if we should use fallback mode.
 */
  bool use_fallback()
  {
    if( this->using_preselected_packet )
    {
      return true;
    }

    if( !this->enable_fallback )
    {
      return false;
    }

    //
    // There are two reasons to use fall back mode before throwing
    // away the queued frames:
    //
    // a) If the queue is (close to) full and we are still waiting for
    // good metadata packet and the shot has not yet ended.
    //
    // b) If the queue is not full yet, has significant number of
    // entries (>= min_size_for_fallback) and the end of shot has been
    // reported.
    //
    if( proc_src_ts_q->is_close_to_full() ||
        ( ( m_in_shot_break_flags.is_shot_end() || end_of_video )
          && ( proc_src_ts_q->length() >= min_size_for_fallback )
          )
      )
    {
      // check to see if we have a fallback metadata packet
      if( set_data_related_to_metadata( fallback_meta_data_packet, fallback_width, fallback_height ) )
      {
        LOG_DEBUG( "md_lag_sp: Using fallback mode, metadata packet ("
                 <<  fallback_meta_data_packet.timeUTC() <<")" );
      }
      else
      {
        LOG_DEBUG( "md_lag_sp: Using fallback mode, GSD : "<< fallback_gsd <<")" );
        homography::transform_t fb;
        fb.set_identity();
        fb.set_scale(fallback_gsd);
        this->H_ref2wld.set_transform(fb);
        this->H_ref2wld.set_source_reference( proc_src_ts_q->datum_at( 0 ) );
        this->H_wld2utm.set_identity(false);
      }

      return true;
    }

    return false;
  }

}; // end impl class


// ----------------------------------------------------------------
/** Constructor.
 *
 *
 */
template< class PixType>
world_coord_super_process<PixType>
::world_coord_super_process( vcl_string const& name )
  : super_process( name, "world_coord_super_process" ),
    impl_(0)
{
  impl_ = new world_coord_super_process_impl< PixType >( this );

  impl_->config.add_parameter( "metadata_type", "fixed",
    "Choose between fixed, stream and ortho. fixed: uses manually specified "
    "ref-to-world transformation, stream: creates a lag based on metadata values "
    "& computes the UTM coordinates, and ortho: means orthorectified metadata that "
    "withoutlag and produces both world and UTM coordinates." );

  impl_->config.add_parameter( "min_md_ignore_count", "3",
    "Minimum number of metadata packets to ignore in the beginning of a shot." );

  impl_->config.add_parameter( "fallback_gsd", "1.0",
                               "The fall back gsd, used when metadata does not show up soon enough" );
  impl_->config.add_parameter( "disable_fallback_gsd", "false",
                               "Disables the use of the fallback gsd.");

  impl_->config.add_parameter( "eo_multiplier", "4",
                               "Multiplier used to help determin if "
                               "the video is in eo or ir." );
  impl_->config.add_parameter( "rg_diff", "8",
                               "Difference threshold for red and green."
                               "  Used when determining eo or ir." );
  impl_->config.add_parameter( "rb_diff", "8",
                               "Difference threshold for red and blue."
                               "  Used when determining eo or ir." );
  impl_->config.add_parameter( "gb_diff", "8",
                               "Difference threshold for green and blue."
                               "  Used when determining eo or ir." );
  //min_size_for_fallback
  impl_->config.add_parameter( "fallback_count", "50",
                               "This is used when a shot or end occurs to determin it to use the fallback gsd" );

  impl_->config.add_parameter( "min_allowed_gsd", "0.03",
    "We will ignore any GSD values lower than this number for the sake of "
    "numerical stability of the tracking pipeline parameters. Any computed"
    " H_ref2wld transformation will be scaled to match this GSD." );

  impl_->config.add_parameter( "masking_enabled", "false",
    "Whether or not to run with (metadata burnin) masking support" );

  // Parameters for HD to SD HFOV correction.
  impl_->config.add_parameter( "apply_hd_correction", "false",
    "If true, will change the horizontal field-of-view angle in the metadata "
    "packet before using it. This only affects stream mode. This feature is "
    "a hack to adjust the metadata when the source video was originally HD." );

  impl_->config.add_parameter( "hfov_correction_1", "0.375",
                               "HD to SD correction factor 1" );

  impl_->config.add_parameter( "hfov_correction_2", "0.5",
                               "HD to SD correction factor 2" );

  impl_->config.add_parameter( "hfov_angle", "1.59",
                               "HD to SD fov angle");
}


template< class PixType>
world_coord_super_process<PixType>
::~world_coord_super_process()
{
  delete impl_;
}


template< class PixType>
config_block
world_coord_super_process<PixType>
::params() const
{
  return impl_->config;
}


template< class PixType>
bool
world_coord_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    vcl_string mdtype_str = blk.get<vcl_string>( "metadata_type" );
    if( mdtype_str == "stream" )
    {
      impl_->metadata_type = impl_->VIDTK_METADATA_STREAM;
      // TODO: Add a sanity check where user should not be allowed to assign
      // a value to "trans_for_ground:premult_scale" or
      // "trans_for_ground:premult_matrix"
    }
    else if( mdtype_str == "fixed" )
    {
      impl_->metadata_type = impl_->VIDTK_METADATA_FIXED;
    }
    else if( mdtype_str == "ortho" )
    {
      impl_->metadata_type = impl_->VIDTK_METADATA_ORTHO;
    }
    else
    {
      throw unchecked_return_value( " invalid mode " + mdtype_str + " provided." );
    }

    impl_->fallback_gsd = blk.get<double>("fallback_gsd");
    impl_->enable_fallback = !blk.get<bool>("disable_fallback_gsd");

    impl_->min_size_for_fallback = blk.get<unsigned int>("fallback_count");

    impl_->min_md_ignore_count = blk.get< unsigned >( "min_md_ignore_count" );

    blk.get( "min_allowed_gsd", impl_->min_allowed_gsd );

    blk.get( "masking_enabled", impl_->masking_enabled );

    blk.get( "apply_hd_correction", impl_->apply_hd_correction );
    blk.get( "hfov_correction_1", impl_->hd_to_sd_hfov_correction_1);
    blk.get( "hfov_correction_2", impl_->hd_to_sd_hfov_correction_2);
    blk.get( "hfov_angle", impl_->hd_to_sd_hfov_angle );

    impl_->config.update( blk );

    impl_->setup_pipeline( this->pipeline_ );

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw unchecked_return_value( " unable to set pipeline parameters." );
    }
  }
  catch( unchecked_return_value& e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what());

    return false;
  }

  return true;
}


// ----------------------------------------------------------------
// Super Process interface methods

template< class PixType>
bool
world_coord_super_process<PixType>
::initialize()
{
  impl_->end_of_shot_reset();
  return this->pipeline_->initialize();
}


template< class PixType>
process::step_status
world_coord_super_process<PixType>
::step2()
{
  return impl_->step2( this->pipeline_ );
}


// ----------------------------------------------------------------
/** Pump one set of outputs out of this process.
 *
 *
 */
template< class PixType>
void
world_coord_super_process<PixType>
::pump_output()
{
  push_output (process::SUCCESS);
}


// ================================================================
// Process inputs
template< class PixType>
void
world_coord_super_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->curr_timestamp = ts;
  impl_->proc_src_ts_q->set_input_datum( ts );
}


template< class PixType>
void
world_coord_super_process< PixType >
::set_source_image( vil_image_view< PixType > const& img )
{
  impl_->curr_img = img;
  impl_->proc_src_img_q->set_input_datum( img );
}


template< class PixType>
void
world_coord_super_process< PixType >
::set_src_to_ref_homography(image_to_image_homography const& H )
{
  impl_->curr_src2ref = H;
  impl_->proc_src2ref_homog_q->set_input_datum( H );
}


template< class PixType>
void
world_coord_super_process< PixType >
::set_source_metadata_vector( vcl_vector< video_metadata > const& md )
{
  impl_->proc_filter_meta->set_source_metadata_vector(md);
}


template< class PixType>
void
world_coord_super_process<PixType>
::set_input_timestamp_vector( timestamp::vector_t const& ts )
{
  impl_->proc_src_ts_vector_q->set_input_datum( ts );
}


template< class PixType>
void
world_coord_super_process<PixType>
::set_input_shot_break_flags ( vidtk::shot_break_flags const & v)
{
  impl_->proc_shot_break_flags_q->set_input_datum( v );
  impl_->m_in_shot_break_flags = v;
}


template< class PixType>
void
world_coord_super_process<PixType>
::set_mask ( vil_image_view<bool> const & img)
{
  impl_->proc_src_mask_q->set_input_datum( img );
}


// ================================================================
// Process outputs
template< class PixType >
timestamp const&
world_coord_super_process< PixType >
::source_timestamp() const
{
  return impl_->proc_src_ts_q->get_output_datum();
}


template< class PixType >
vil_image_view< PixType > const&
world_coord_super_process< PixType >
::source_image() const
{
  return impl_->proc_src_img_q->get_output_datum();
}


template< class PixType>
image_to_image_homography const &
world_coord_super_process<PixType>
::src_to_ref_homography() const
{
  return impl_->proc_src2ref_homog_q->get_output_datum();
}


template< class PixType>
image_to_plane_homography const &
world_coord_super_process<PixType>
::src_to_wld_homography() const
{
  return impl_->proc_trans_for_ground->homography();
}


template< class PixType>
plane_to_image_homography const &
world_coord_super_process<PixType>
::wld_to_src_homography() const
{
  return impl_->proc_trans_for_ground->inv_homography();
}


template< class PixType>
plane_to_utm_homography const &
world_coord_super_process<PixType>
::wld_to_utm_homography() const
{
  return impl_->H_wld2utm;
}


template< class PixType>
double
world_coord_super_process<PixType>
::world_units_per_pixel() const
{
  return impl_->proc_gen_gsd->world_units_per_pixel();
}


template< class PixType >
timestamp::vector_t const&
world_coord_super_process< PixType >
::get_output_timestamp_vector() const
{
  return impl_->proc_src_ts_vector_q->get_output_datum();
}


template< class PixType >
image_to_plane_homography const&
world_coord_super_process< PixType >
::get_output_ref_to_wld_homography() const
{
  return impl_->H_ref2wld;
}


template< class PixType >
vidtk::shot_break_flags
world_coord_super_process< PixType >
::get_output_shot_break_flags() const
{
  return impl_->proc_shot_break_flags_q->get_output_datum();
}

template< class PixType >
vil_image_view<bool>
world_coord_super_process< PixType >
::get_output_mask() const
{
  return impl_->proc_src_mask_q->get_output_datum();
}

} // end namespace vidtk
