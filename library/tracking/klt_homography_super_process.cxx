/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "klt_homography_super_process.h"

#include <pipeline/sync_pipeline.h>

#include <kwklt/klt_tracking_process.h>
#include <utilities/log.h>
#include <tracking/homography_process.h>
#include <tracking/shot_detection_process.h>

#include <tracking/shot_break_flags.h>
#include <tracking/shot_break_flags_process.h>
#include <utilities/config_block.h>
#include <utilities/homography_writer_process.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

namespace vidtk
{

class klt_homography_super_process_impl
{
public:
  // Pipeline processes
  process_smart_pointer< klt_tracking_process > proc_klt_tracking;
  process_smart_pointer< homography_process > proc_compute_homog;
  process_smart_pointer< homography_writer_process > proc_homog_writer;
  process_smart_pointer< shot_detection_process > proc_shot_detector_klt;
  process_smart_pointer< shot_detection_process > proc_shot_detector_homog;
  process_smart_pointer< shot_break_flags_process > proc_shot_break_flags;

  // Configuration parameters
  config_block config;

  // CTOR
  klt_homography_super_process_impl()
  : proc_klt_tracking( NULL ),
    proc_compute_homog( NULL ),
    proc_homog_writer( NULL ),
    proc_shot_detector_klt( NULL ),
    proc_shot_detector_homog( NULL ),
    proc_shot_break_flags( NULL )
  {
  }

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    proc_klt_tracking = new klt_tracking_process( "klt_tracking" );
    config.add_subblock( proc_klt_tracking->params(),
                         proc_klt_tracking->name() );

    proc_shot_detector_klt = new shot_detection_process( "shot_generator_klt" );
    config.add_subblock( proc_shot_detector_klt->params(),
                         proc_shot_detector_klt->name() );

    proc_compute_homog = new homography_process( "compute_homog" );
    config.add_subblock( proc_compute_homog->params(),
                         proc_compute_homog->name() );

    proc_homog_writer = new homography_writer_process( "src2ref_homog_writer" );
    config.add_subblock( proc_homog_writer->params(),
                         proc_homog_writer->name() );

    proc_shot_detector_homog = new shot_detection_process( "shot_generator_homog" );
    config.add_subblock( proc_shot_detector_homog->params(),
                         proc_shot_detector_homog->name() );

    proc_shot_break_flags = new shot_break_flags_process( "shot_break_flags_proc" );
    config.add_subblock( proc_shot_break_flags->params(),
                         proc_shot_break_flags->name() );
  }

  void setup_pipeline( sync_pipeline * p )
  {
    p->add( proc_klt_tracking );
    p->add( proc_compute_homog );
    p->connect( proc_klt_tracking->created_tracks_port(),
                proc_compute_homog->set_new_tracks_port() );
    p->connect( proc_klt_tracking->active_tracks_port(),
                proc_compute_homog->set_updated_tracks_port() );

    p->add( proc_homog_writer );
    p->connect( proc_compute_homog->image_to_world_vidtk_homography_image_port(),
                proc_homog_writer->set_source_vidtk_homography_port() );

    p->add( proc_shot_detector_klt );
    p->connect( proc_klt_tracking->active_tracks_port(),
                proc_shot_detector_klt->set_active_tracks_port() );
    p->connect( proc_klt_tracking->terminated_tracks_port(),
                proc_shot_detector_klt->set_terminated_tracks_port() );

    // We want to compute homography only when we are certain about the
    // quality of the correspondences. Adding this depedency because the
    // shot_detector will fail in case of bad correspondences.
    p->connect( proc_shot_detector_klt->is_end_of_shot_port(),
                proc_compute_homog->set_unusable_frame_port() );

    p->add( proc_shot_detector_homog );
    p->connect( proc_compute_homog->image_to_world_vidtk_homography_image_port(),
                proc_shot_detector_homog->set_src_to_ref_vidtk_homography_port() );

    p->add( proc_shot_break_flags );
    p->connect( proc_shot_detector_klt->is_end_of_shot_port(),
                proc_shot_break_flags->set_is_tracker_shot_break_port() );
    p->connect( proc_shot_detector_homog->is_end_of_shot_port(),
                proc_shot_break_flags->set_is_homography_shot_break_port() );
  }
};

klt_homography_super_process
::klt_homography_super_process( vcl_string const& name )
  : super_process( name, "klt_homography_super_process" ),
    impl_( new klt_homography_super_process_impl )
{
  impl_->create_process_configs();
}

klt_homography_super_process
::~klt_homography_super_process()
{
  delete impl_;
}

config_block
klt_homography_super_process
::params() const
{
  return impl_->config;
}

bool
klt_homography_super_process
::set_params( config_block const& blk )
{
  try
  {
    sync_pipeline* p = new sync_pipeline;

    config_block blk2( blk );
    add_videoname_prefix( blk2, this->impl_->proc_homog_writer->name() + ":output_filename" );

    impl_->config.update( blk2 );

    impl_->setup_pipeline( p );
    this->pipeline_ = p;

    // Force tracking to be enabled.
    impl_->config.set( impl_->proc_klt_tracking->name() + ":disabled", "false" );

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw unchecked_return_value( " unable to set pipeline parameters." );
    }
  }
  catch( unchecked_return_value& e)
  {
    log_error( this->name() << ": couldn't set parameters: "<< e.what() <<"\n" );

    return false;
  }

  return true;
}


// ----------------------------------------------------------------
bool
klt_homography_super_process
::initialize()
{
  return this->pipeline_->initialize();
}


// ----------------------------------------------------------------
process::step_status
klt_homography_super_process
::step2()
{
  process::step_status st = this->pipeline_->execute();

  if ( st == process::SUCCESS )
  {
    if ( impl_->proc_shot_break_flags->flags().is_shot_end() )
    {
      impl_->proc_klt_tracking->reset();
      impl_->proc_compute_homog->reset();
    }
  }

  return st;
}


void
klt_homography_super_process
::set_timestamp( timestamp const& ts )
{
  impl_->proc_klt_tracking->set_timestamp( ts );
  impl_->proc_compute_homog->set_timestamp( ts );
  impl_->proc_homog_writer->set_source_timestamp( ts );
}

void
klt_homography_super_process
::set_image_pyramid( vil_pyramid_image_view<float> const& pyr )
{
  impl_->proc_klt_tracking->set_image_pyramid( pyr );
}

void
klt_homography_super_process
::set_image_pyramid_gradx( vil_pyramid_image_view<float> const& pyr )
{
  impl_->proc_klt_tracking->set_image_pyramid_gradx( pyr );
}

void
klt_homography_super_process
::set_image_pyramid_grady( vil_pyramid_image_view<float> const& pyr )
{
  impl_->proc_klt_tracking->set_image_pyramid_grady( pyr );
}

void
klt_homography_super_process
::set_mask( vil_image_view<bool> const& mask )
{
  impl_->proc_compute_homog->set_mask_image( mask );
}

image_to_image_homography const &
klt_homography_super_process
::src_to_ref_homography() const
{
  return impl_->proc_compute_homog->image_to_world_vidtk_homography_image();
}

image_to_image_homography
klt_homography_super_process
::ref_to_src_homography() const
{
  image_to_image_homography h = impl_->proc_compute_homog->image_to_world_vidtk_homography_image();
  return h.get_inverse();
}

vcl_vector< klt_track_ptr > const&
klt_homography_super_process
::active_tracks() const
{
  return impl_->proc_klt_tracking->active_tracks();
}

vidtk::shot_break_flags
klt_homography_super_process::
get_output_shot_break_flags ( ) const
{
  return impl_->proc_shot_break_flags->flags();
}

} // end namespace vidtk
