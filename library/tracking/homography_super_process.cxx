/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "homography_super_process.h"

#include <pipeline_framework/sync_pipeline.h>

#include <kwklt/klt_tracking_process.h>
#ifdef USE_VISCL
#include <tracking/gpu_descr_tracking_process.h>
#endif
#include <tracking/homography_process.h>
#include <tracking/shot_detection_process.h>

#include <tracking_data/shot_break_flags.h>
#include <tracking_data/io/track_writer_process.h>
#include <tracking/shot_break_flags_process.h>
#include <utilities/config_block.h>
#include <utilities/videoname_prefix.h>
#include <utilities/homography_writer_process.h>
#include <utilities/apply_function_to_vector_process.txx>


#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_klt_homography_super_process_cxx__
VIDTK_LOGGER("klt_homography_super_process_cxx");

namespace
{
  vidtk::track_sptr klt_to_vidtk_track(vidtk::klt_track_ptr const& trk)
  {
    static unsigned int id = 0;
    vidtk::track_sptr result = vidtk::convert_from_klt_track(trk);
    result->set_id(id++);
    return result;
  }
}

namespace vidtk
{

class homography_super_process_impl
{
public:
  // Pipeline processes
  process_smart_pointer< klt_tracking_process > proc_klt_tracking;
  process_smart_pointer< apply_function_to_vector_process< klt_track_ptr, track_sptr, klt_to_vidtk_track > > proc_klt_converter;
  process_smart_pointer< track_writer_process > proc_klt_track_writer;

#ifdef USE_VISCL
  process_smart_pointer< gpu_descr_tracking_process > proc_gpu_tracking;
#endif
  process_smart_pointer< homography_process > proc_compute_homog;
  process_smart_pointer< homography_writer_process > proc_homog_writer;
  process_smart_pointer< shot_detection_process > proc_shot_detector_klt;
  process_smart_pointer< shot_detection_process > proc_shot_detector_homog;
  process_smart_pointer< shot_break_flags_process > proc_shot_break_flags;

  // Configuration parameters
  config_block config;

  enum mode{ VIDTK_STAB_TRACK_KLT, VIDTK_STAB_TRACK_GPU };
  mode tracker_type;

  // CTOR
  homography_super_process_impl()
  : proc_klt_tracking( NULL ),
    proc_klt_converter(NULL),
    proc_klt_track_writer(NULL),
#ifdef USE_VISCL
    proc_gpu_tracking( NULL ),
#endif
    proc_compute_homog( NULL ),
    proc_homog_writer( NULL ),
    proc_shot_detector_klt( NULL ),
    proc_shot_detector_homog( NULL ),
    proc_shot_break_flags( NULL ),
    tracker_type(VIDTK_STAB_TRACK_KLT)
  {
  }

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    proc_klt_tracking = new klt_tracking_process( "klt_tracking" );
    config.add_subblock( proc_klt_tracking->params(),
                         proc_klt_tracking->name() );

    proc_klt_converter = new  apply_function_to_vector_process< klt_track_ptr, track_sptr, klt_to_vidtk_track >("klt_converter");
    config.add_subblock( proc_klt_converter->params(),
                         proc_klt_converter->name() );
    proc_klt_track_writer = new track_writer_process("klt_track_writer");
    config.add_subblock( proc_klt_track_writer->params(),
                         proc_klt_track_writer->name() );

#ifdef USE_VISCL
    proc_gpu_tracking = new gpu_descr_tracking_process( "gpu_tracking" );
    config.add_subblock( proc_gpu_tracking->params(),
                         proc_gpu_tracking->name() );
#endif

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
    p->add( proc_compute_homog );
    p->add( proc_shot_detector_klt );
    p->add(proc_klt_converter);
    p->add(proc_klt_track_writer);

    if( tracker_type == VIDTK_STAB_TRACK_KLT )
    {
      p->add( proc_klt_tracking );


      p->connect( proc_klt_tracking->created_tracks_port(),
                  proc_compute_homog->set_new_tracks_port() );
      p->connect( proc_klt_tracking->active_tracks_port(),
                  proc_compute_homog->set_updated_tracks_port() );

      p->connect( proc_klt_tracking->active_tracks_port(),
                  proc_shot_detector_klt->set_active_tracks_port() );
      p->connect( proc_klt_tracking->terminated_tracks_port(),
                  proc_shot_detector_klt->set_terminated_tracks_port() );

      p->connect( proc_klt_tracking->terminated_tracks_port(),
                  proc_klt_converter->set_input_port() );

    }
#ifdef USE_VISCL
    else if( tracker_type == VIDTK_STAB_TRACK_GPU )
    {
      p->add( proc_gpu_tracking );

      p->connect( proc_gpu_tracking->created_tracks_port(),
                  proc_compute_homog->set_new_tracks_port() );
      p->connect( proc_gpu_tracking->active_tracks_port(),
                  proc_compute_homog->set_updated_tracks_port() );

      p->connect( proc_gpu_tracking->active_tracks_port(),
                  proc_shot_detector_klt->set_active_tracks_port() );
      p->connect( proc_gpu_tracking->terminated_tracks_port(),
                  proc_shot_detector_klt->set_terminated_tracks_port() );

      p->connect( proc_gpu_tracking->terminated_tracks_port(),
                  proc_klt_converter->set_input_port() );

    }
#endif

    p->connect( proc_klt_converter->get_output_port(),
                proc_klt_track_writer->set_tracks_port() );


    p->add( proc_homog_writer );
    p->connect( proc_compute_homog->image_to_world_vidtk_homography_image_port(),
                proc_homog_writer->set_source_vidtk_homography_port() );

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

homography_super_process
::homography_super_process( std::string const& _name )
  : super_process( _name, "homography_super_process" ),
    impl_( new homography_super_process_impl )
{
  impl_->create_process_configs();

  impl_->config.add_parameter( "tracker_type", "klt",
  "Choose between \"klt\" and \"gpu\".\n"
  "  klt: Requires image pyramids and uses the traditional KLT point\n"
  "       tracking algorithm by Birchfield.\n"
  "  gpu: Require just images and uses a GPU accelerated implementation\n"
  "       of point descriptor matching with hessian corners and BRIEF\n"
  "       descriptors.  This algorithm depends on VisCL and OpenCL." );
}

homography_super_process
::~homography_super_process()
{
  delete impl_;
}

config_block
homography_super_process
::params() const
{
  return impl_->config;
}

bool
homography_super_process
::set_params( config_block const& blk )
{
  try
  {
    std::string tracker_type_str = blk.get<std::string>( "tracker_type" );
    if( tracker_type_str == "klt" )
    {
      impl_->tracker_type = impl_->VIDTK_STAB_TRACK_KLT;
    }
    else if( tracker_type_str == "gpu" )
    {
      impl_->tracker_type = impl_->VIDTK_STAB_TRACK_GPU;
    }
    else
    {
      throw config_block_parse_error( " invalid mode " + tracker_type_str + " provided." );
    }

#ifndef USE_VISCL
    if( impl_->tracker_type == impl_->VIDTK_STAB_TRACK_GPU )
    {
      throw std::runtime_error("Support for GPU tracking not compiled in, requires viscl.");
    }
#endif

    config_block blk2( blk );
    videoname_prefix::instance()->add_videoname_prefix( blk2,
                      this->impl_->proc_homog_writer->name() + ":output_filename" );

    impl_->config.update( blk2 );

    sync_pipeline* p = new sync_pipeline;

    impl_->setup_pipeline( p );
    this->pipeline_ = p;

    // Force tracking to be enabled.
    impl_->config.set( impl_->proc_klt_tracking->name() + ":disabled", "false" );

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error( " unable to set pipeline parameters." );
    }
  }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );

    return false;
  }

  return true;
}


// ----------------------------------------------------------------
bool
homography_super_process
::initialize()
{
  return this->pipeline_->initialize();
}


// ----------------------------------------------------------------
process::step_status
homography_super_process
::step2()
{
  process::step_status st = this->pipeline_->execute();

  if ( st == process::SUCCESS )
  {
    if ( impl_->proc_shot_break_flags->flags().is_shot_end() )
    {
      if( impl_->tracker_type == impl_->VIDTK_STAB_TRACK_KLT )
      {
        impl_->proc_klt_tracking->reset();
      }
#ifdef USE_VISCL
      else if( impl_->tracker_type == impl_->VIDTK_STAB_TRACK_GPU )
      {
        impl_->proc_gpu_tracking->reset();
      }
#endif
      impl_->proc_compute_homog->reset();
    }
  }

  return st;
}


void
homography_super_process
::set_timestamp( timestamp const& ts )
{
  if( impl_->tracker_type == impl_->VIDTK_STAB_TRACK_KLT )
  {
    impl_->proc_klt_tracking->set_timestamp( ts );
  }
#ifdef USE_VISCL
  else if( impl_->tracker_type == impl_->VIDTK_STAB_TRACK_GPU )
  {
    impl_->proc_gpu_tracking->set_timestamp( ts );
  }
#endif
  impl_->proc_compute_homog->set_timestamp( ts );
  impl_->proc_homog_writer->set_source_timestamp( ts );
}

void
homography_super_process
::set_image( vil_image_view<vxl_byte> const& img )
{
#ifdef USE_VISCL
  impl_->proc_gpu_tracking->set_image( img );
#else
  (void)img;
#endif
}

void
homography_super_process
::set_homog_predict( vgl_h_matrix_2d<double> const& f2f )
{
  impl_->proc_klt_tracking->set_homog_predict(f2f);
}

void
homography_super_process
::set_image_pyramid( vil_pyramid_image_view<float> const& pyr )
{
  impl_->proc_klt_tracking->set_image_pyramid( pyr );
}

void
homography_super_process
::set_image_pyramid_gradx( vil_pyramid_image_view<float> const& pyr )
{
  impl_->proc_klt_tracking->set_image_pyramid_gradx( pyr );
}

void
homography_super_process
::set_image_pyramid_grady( vil_pyramid_image_view<float> const& pyr )
{
  impl_->proc_klt_tracking->set_image_pyramid_grady( pyr );
}

void
homography_super_process
::set_mask( vil_image_view<bool> const& mask )
{
  impl_->proc_compute_homog->set_mask_image( mask );
}

image_to_image_homography
homography_super_process
::src_to_ref_homography() const
{
  return impl_->proc_compute_homog->image_to_world_vidtk_homography_image();
}

image_to_image_homography
homography_super_process
::ref_to_src_homography() const
{
  image_to_image_homography h = impl_->proc_compute_homog->image_to_world_vidtk_homography_image();
  return h.get_inverse();
}

std::vector< klt_track_ptr >
homography_super_process
::active_tracks() const
{
#ifdef USE_VISCL
  if( impl_->tracker_type == impl_->VIDTK_STAB_TRACK_GPU )
  {
    return impl_->proc_gpu_tracking->active_tracks();
  }
#endif
  return impl_->proc_klt_tracking->active_tracks();
}

vidtk::shot_break_flags
homography_super_process::
get_output_shot_break_flags ( ) const
{
  return impl_->proc_shot_break_flags->flags();
}

} // end namespace vidtk
