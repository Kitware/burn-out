/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "remove_burnin_pipeline.h"

#include <video_io/generic_frame_process.h>
#include <video_io/image_list_writer_process.h>

#include <object_detectors/metadata_mask_super_process.h>
#include <object_detectors/diff_buffer_process.h>
#include <object_detectors/detector_factory.h>
#include <object_detectors/detector_super_process.h>

#include <video_transforms/inpainting_process.h>
#include <video_transforms/warp_image_process.h>

#include <tracking/stabilization_super_process.h>

#include <pipeline_framework/pipeline.h>
#include <pipeline_framework/sync_pipeline.h>
#include <pipeline_framework/async_pipeline.h>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_remove_burnin_pipeline_txx__
VIDTK_LOGGER( "remove_burnin_pipeline" );

namespace vidtk
{

template< class PixType >
class remove_burnin_pipeline_impl
{
public:

  typedef super_process_pad_impl< vil_image_view< PixType > > pad_image_t;
  typedef super_process_pad_impl< vil_image_view< bool > > pad_mask_t;
  typedef super_process_pad_impl< timestamp > pad_timestamp_t;
  typedef super_process_pad_impl< video_metadata > pad_metadata_t;

  process_smart_pointer< pad_image_t > pad_source_image;
  process_smart_pointer< pad_timestamp_t > pad_source_timestamp;
  process_smart_pointer< pad_metadata_t > pad_source_metadata;

  process_smart_pointer< pad_image_t > pad_output_original_image;
  process_smart_pointer< pad_image_t > pad_output_inpainted_image;
  process_smart_pointer< pad_mask_t > pad_output_mask;
  process_smart_pointer< pad_timestamp_t > pad_output_timestamp;
  process_smart_pointer< pad_metadata_t > pad_output_metadata;

  process_smart_pointer< generic_frame_process< PixType > > proc_source;
  process_smart_pointer< metadata_mask_super_process< PixType > > proc_mask_sp;
  process_smart_pointer< image_list_writer_process< bool > > proc_mask_writer;
  process_smart_pointer< stabilization_super_process< PixType > > proc_stab_sp;
  process_smart_pointer< detector_super_process< PixType > > proc_detect_sp;
  process_smart_pointer< inpainting_process< PixType > > proc_inpainter;
  process_smart_pointer< image_list_writer_process< PixType > > proc_inpainted_writer;

  detector_factory< PixType > detection_factory;

  config_block config;
  config_block default_config;

  unsigned default_edge_capacity;
  unsigned output_pad_edge_capacity;

  remove_burnin_pipeline_impl()
  : detection_factory( "detector_sp" ),
    default_edge_capacity( 10 ),
    output_pad_edge_capacity( 10 )
  {
    pad_source_image = new pad_image_t( "source_image_pad" );
    pad_source_timestamp = new pad_timestamp_t( "source_timestamp_pad" );
    pad_source_metadata = new pad_metadata_t( "source_metadata_pad" );

    pad_output_original_image = new pad_image_t( "output_original_image_pad" );
    pad_output_inpainted_image = new pad_image_t( "output_inpainted_image_pad" );
    pad_output_mask = new pad_mask_t( "output_mask_pad" );
    pad_output_timestamp = new pad_timestamp_t( "output_timestamp_pad" );
    pad_output_metadata = new pad_metadata_t( "output_metadata_pad" );

    proc_mask_sp = new metadata_mask_super_process< PixType >( "md_mask_sp" );
    config.add_subblock( proc_mask_sp->params(), proc_mask_sp->name() );

    proc_mask_writer = new image_list_writer_process< bool >( "mask_writer" );
    config.add_subblock( proc_mask_writer->params(), proc_mask_writer->name() );

    proc_stab_sp = new stabilization_super_process< PixType >( "stab_sp" );
    config.add_subblock( proc_stab_sp->params(), proc_stab_sp->name() );

    config.add_subblock( detection_factory.params(), detection_factory.name() );

    proc_inpainter = new inpainting_process< PixType >( "inpainter" );
    config.add_subblock( proc_inpainter->params(), proc_inpainter->name() );

    proc_inpainted_writer = new image_list_writer_process< PixType >( "inpainted_writer" );
    config.add_subblock( proc_inpainted_writer->params(), proc_inpainted_writer->name() );

    default_config.add_parameter( proc_stab_sp->name() + ":masking_enabled", "true", "" );
    default_config.add_parameter( proc_stab_sp->name() + ":masking_source", "external", "" );
    default_config.add_parameter( proc_mask_writer->name() + ":disabled", "true", "" );
    default_config.add_parameter( proc_mask_writer->name() + ":pattern", "mask%2$04d.ppm", "" );
    default_config.add_parameter( proc_inpainted_writer->name() + ":disabled", "true", "" );
    default_config.add_parameter( proc_inpainted_writer->name() + ":pattern", "output%2$04d.png", "" );
    default_config.add_parameter( detection_factory.name() + ":run_async", "true", "" );
    default_config.add_parameter( detection_factory.name() + ":masking_enabled", "true", "" );
    default_config.add_parameter( detection_factory.name() + ":gui_feedback_enabled", "true", "" );
    config.update( default_config );
  }

  virtual ~remove_burnin_pipeline_impl()
  {
  }

  template< class Pipeline >
  void
  setup_pipeline( Pipeline *p )
  {
    // Add all input pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_metadata );

    // Add all output pads
    p->add( pad_output_original_image );
    p->add( pad_output_inpainted_image );
    p->add( pad_output_mask );
    p->add( pad_output_timestamp );
    p->add( pad_output_metadata );

    // Add all processes
    p->add( proc_mask_sp );
    p->add( proc_inpainter );

    // Pass timestamp and metadata to core detector
    p->connect( pad_source_timestamp->value_port(),
                proc_mask_sp->set_timestamp_port() );
    p->connect( pad_source_metadata->value_port(),
                proc_mask_sp->set_metadata_port() );

    // Pass images through pipeline
    p->connect( pad_source_image->value_port(),
                proc_mask_sp->set_image_port() );
    p->connect( proc_mask_sp->image_port(),
                proc_inpainter->set_source_image_port() );
    p->connect( proc_mask_sp->timestamp_port(),
                proc_inpainter->set_source_timestamp_port() );
    p->connect( proc_mask_sp->mask_port(),
                proc_inpainter->set_mask_port() );
    p->connect( proc_mask_sp->border_port(),
                proc_inpainter->set_border_port() );
    p->connect( proc_mask_sp->detected_type_port(),
                proc_inpainter->set_type_port() );

    p->connect( proc_inpainter->inpainted_image_port(),
                pad_output_inpainted_image->set_value_port() );

    // Connect auxiliary output ports
    p->set_edge_capacity( output_pad_edge_capacity );
    p->connect( proc_mask_sp->image_port(),
                pad_output_original_image->set_value_port() );
    p->connect( proc_mask_sp->timestamp_port(),
                pad_output_timestamp->set_value_port() );
    p->connect( proc_mask_sp->metadata_port(),
                pad_output_metadata->set_value_port() );
    p->connect( proc_mask_sp->mask_port(),
                pad_output_mask->set_value_port() );
    p->set_edge_capacity( default_edge_capacity );

    // Add optional stabilization processes
    if( config.get< double >( proc_inpainter->name() + ":stab_image_factor" ) > 0.0 )
    {
      p->add( proc_stab_sp );

      p->connect( proc_mask_sp->image_port(),
                  proc_stab_sp->set_source_image_port() );
      p->connect( proc_mask_sp->mask_port(),
                  proc_stab_sp->set_source_mask_port() );
      p->connect( proc_mask_sp->timestamp_port(),
                  proc_stab_sp->set_source_timestamp_port() );
      p->connect( proc_mask_sp->metadata_port(),
                  proc_stab_sp->set_source_metadata_port() );

      p->connect( proc_stab_sp->src_to_ref_homography_port(),
                  proc_inpainter->set_homography_port() );
      p->connect( proc_stab_sp->get_output_shot_break_flags_port(),
                  proc_inpainter->set_shot_break_flags_port() );

      if( config.get< bool >( "use_motion" ) )
      {
        // Optionally detect a motion mask and connect it to the inpainter
        proc_detect_sp = detection_factory.create_detector( this->config );

        p->add( proc_detect_sp );

        p->connect( proc_mask_sp->image_port(),
                    proc_detect_sp->input_image_port() );
        p->connect( proc_mask_sp->timestamp_port(),
                    proc_detect_sp->input_timestamp_port() );
        p->connect( proc_mask_sp->mask_port(),
                    proc_detect_sp->input_mask_image_port() );
        p->connect( proc_mask_sp->world_units_per_pixel_port(),
                    proc_detect_sp->input_world_units_per_pixel_port() );

        p->connect( proc_stab_sp->src_to_ref_homography_port(),
                    proc_detect_sp->input_src_to_ref_homography_port() );
        p->connect( proc_stab_sp->get_output_shot_break_flags_port(),
                    proc_detect_sp->input_shot_break_flags_port() );

        p->connect( proc_detect_sp->output_morph_image_port(),
                    proc_inpainter->set_motion_mask_port() );
      }
    }

    // Add optional mask writer
    if( !config.get< bool >( proc_mask_writer->name() + ":disabled" ) )
    {
      p->add( proc_mask_writer );

      p->connect( proc_mask_sp->timestamp_port(),
                  proc_mask_writer->set_timestamp_port() );
      p->connect( proc_mask_sp->mask_port(),
                  proc_mask_writer->set_image_port() );
    }

    // Add optional inpainted image writer
    if( !config.get< bool >( proc_inpainted_writer->name() + ":disabled" ) )
    {
      p->add( proc_inpainted_writer );

      p->connect( proc_mask_sp->timestamp_port(),
                  proc_inpainted_writer->set_timestamp_port() );
      p->connect( proc_inpainter->inpainted_image_port(),
                  proc_inpainted_writer->set_image_port() );
    }
  }
};


template< class PixType >
remove_burnin_pipeline< PixType >
::remove_burnin_pipeline( std::string const& _name )
  : super_process( _name, "remove_burnin_pipeline" ),
    impl_( NULL )
{
  this->impl_ = new remove_burnin_pipeline_impl< PixType >();

  this->impl_->config.add_parameter( "run_async",
    "true",
    "Should this pipeline be run in a sync or async configuration?" );

  this->impl_->config.add_parameter( "pipeline_edge_capacity",
    boost::lexical_cast< std::string >( 10 / sizeof( PixType ) ),
    "Maximum size of the edge in an async pipeline." );

  this->impl_->config.add_parameter( "use_motion",
    "false",
    "Whether or not to use motion information in inpainting." );
}


template< class PixType >
remove_burnin_pipeline< PixType >
::~remove_burnin_pipeline()
{
  delete this->impl_;
}


template< class PixType >
bool
remove_burnin_pipeline< PixType >
::initialize()
{
  return this->pipeline_->initialize();
}


template< class PixType >
config_block
remove_burnin_pipeline< PixType >
::params() const
{
  return impl_->config;
}


template< class PixType >
bool
remove_burnin_pipeline< PixType >
::set_params( config_block const& blk )
{
  try
  {
    impl_->config.update( blk );

    if( blk.get< bool >( "run_async" ) )
    {
      impl_->default_edge_capacity = blk.get< unsigned >(
        "pipeline_edge_capacity" );

      impl_->output_pad_edge_capacity = blk.get< unsigned >(
        impl_->proc_inpainter->name() + ":max_buffer_size" ) + 1;

      async_pipeline* p =
       new async_pipeline( async_pipeline::ENABLE_STATUS_FORWARDING,
         impl_->default_edge_capacity );

      this->impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else
    {
      sync_pipeline* p =
        new sync_pipeline;

      this->impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }

    if( !pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error( "Unable to set pipeline parameters." );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  return true;
}


template< class PixType >
process::step_status
remove_burnin_pipeline< PixType >
::step2()
{
  return this->pipeline_->execute();
}


template< class PixType >
bool
remove_burnin_pipeline< PixType >
::step()
{
  return ( this->step2() == process::SUCCESS );
}


template < class PixType >
void
remove_burnin_pipeline< PixType >
::set_image( vil_image_view< PixType > const& img )
{
  impl_->pad_source_image->set_value( img );
}


template < class PixType >
void
remove_burnin_pipeline< PixType >
::set_timestamp( vidtk::timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}


template < class PixType >
void
remove_burnin_pipeline< PixType >
::set_metadata( vidtk::video_metadata const& md )
{
  impl_->pad_source_metadata->set_value( md );
}


template< class PixType >
vil_image_view< PixType >
remove_burnin_pipeline< PixType >
::original_image() const
{
  return this->impl_->pad_output_original_image->value();
}


template< class PixType >
vil_image_view< PixType >
remove_burnin_pipeline< PixType >
::inpainted_image() const
{
  return this->impl_->pad_output_inpainted_image->value();
}


template< class PixType >
vil_image_view< bool >
remove_burnin_pipeline< PixType >
::detected_mask() const
{
  return this->impl_->pad_output_mask->value();
}


template< class PixType >
timestamp
remove_burnin_pipeline< PixType >
::timestamp() const
{
  return this->impl_->pad_output_timestamp->value();
}


template< class PixType >
video_metadata
remove_burnin_pipeline< PixType >
::metadata() const
{
  return this->impl_->pad_output_metadata->value();
}

}
