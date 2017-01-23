/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "preprocessing_super_process.h"

#include <video_transforms/frame_downsampling_process.h>
#include <video_transforms/video_enhancement_process.h>
#include <video_transforms/rescale_image_process.h>

#include <utilities/transform_timestamp_process.h>

#include <object_detectors/metadata_mask_super_process.h>

#include <pipeline_framework/pipeline.h>
#include <pipeline_framework/sync_pipeline.h>
#include <pipeline_framework/async_pipeline.h>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_preprocessing_super_process_txx__
VIDTK_LOGGER( "preprocessing_super_process" );

namespace vidtk
{

template< class PixType >
class preprocessing_super_process_impl
{
public:

  typedef super_process_pad_impl< vil_image_view< PixType > > pad_image_t;
  typedef super_process_pad_impl< vil_image_view< bool > > pad_mask_t;
  typedef super_process_pad_impl< timestamp > pad_timestamp_t;
  typedef super_process_pad_impl< video_metadata > pad_metadata_t;

  process_smart_pointer< pad_image_t > pad_source_image;
  process_smart_pointer< pad_timestamp_t > pad_source_timestamp;
  process_smart_pointer< pad_metadata_t > pad_source_metadata;

  process_smart_pointer< pad_image_t > pad_output_image;
  process_smart_pointer< pad_mask_t > pad_output_mask;
  process_smart_pointer< pad_timestamp_t > pad_output_timestamp;
  process_smart_pointer< pad_metadata_t > pad_output_metadata;

  process_smart_pointer< transform_timestamp_process< PixType > > proc_timestamper;
  process_smart_pointer< frame_downsampling_process< PixType > > proc_downsampler;
  process_smart_pointer< video_enhancement_process< PixType > > proc_enhancer;
  process_smart_pointer< metadata_mask_super_process< PixType > > proc_metadata_mask;
  process_smart_pointer< rescale_image_process< PixType > > proc_image_resize;
  process_smart_pointer< rescale_image_process< bool > > proc_mask_resize;

  config_block config;
  bool masking_enabled;
  bool full_resolution_masking;

  preprocessing_super_process_impl()
    : masking_enabled( false ),
      full_resolution_masking( true )
  {
    pad_source_image = new pad_image_t( "source_image_pad" );
    pad_source_timestamp = new pad_timestamp_t( "source_timestamp_pad" );
    pad_source_metadata = new pad_metadata_t( "source_metadata_pad" );

    pad_output_image = new pad_image_t( "output_image_pad" );
    pad_output_mask = new pad_mask_t( "output_mask_pad" );
    pad_output_timestamp = new pad_timestamp_t( "output_timestamp_pad" );
    pad_output_metadata = new pad_metadata_t( "output_metadata_pad" );

    proc_timestamper = new transform_timestamp_process< PixType >( "timestamper" );
    config.add_subblock( proc_timestamper->params(), proc_timestamper->name() );

    proc_downsampler = new frame_downsampling_process< PixType >( "downsampler" );
    config.add_subblock( proc_downsampler->params(), proc_downsampler->name() );

    proc_enhancer = new video_enhancement_process< PixType >( "enhancer" );
    config.add_subblock( proc_enhancer->params(), proc_enhancer->name() );

    proc_metadata_mask = new metadata_mask_super_process< PixType >( "md_mask_sp" );
    config.add_subblock( proc_metadata_mask->params(), proc_metadata_mask->name() );

    proc_image_resize = new rescale_image_process< PixType >( "image_resizer" );
    config.add_subblock( proc_image_resize->params(), proc_image_resize->name() );

    proc_mask_resize = new rescale_image_process< bool >( "mask_resizer" );
    config.add_subblock( proc_mask_resize->params(), proc_mask_resize->name() );
  }

  virtual ~preprocessing_super_process_impl()
  {
  }

  template< class Pipeline >
  void
  setup_pipeline( Pipeline *p )
  {
    const bool downsampler_enabled = !config.get< bool >( proc_downsampler->name() + ":disabled" );
    const bool enhancer_enabled = !config.get< bool >( proc_enhancer->name() + ":disabled" );

    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_metadata );

    p->add( pad_output_image );
    p->add( pad_output_timestamp );
    p->add( pad_output_metadata );

    // Always add the timestamper and image resizer, they are disabled by default.
    p->add( proc_timestamper );
    p->add( proc_image_resize );

    p->connect( pad_source_image->value_port(),
                proc_timestamper->set_source_image_port() );
    p->connect( pad_source_timestamp->value_port(),
                proc_timestamper->set_source_timestamp_port() );
    p->connect( pad_source_metadata->value_port(),
                proc_timestamper->set_source_metadata_port() );

    // If masking is enabled, add it and connect it to output ports.
    if( masking_enabled )
    {
      p->add( pad_output_mask );
      p->add( proc_metadata_mask );

      p->connect( proc_metadata_mask->metadata_port(),
                  pad_output_metadata->set_value_port() );

      if( full_resolution_masking )
      {
        p->add( proc_mask_resize );

        p->connect( proc_metadata_mask->mask_port(),
                    proc_mask_resize->set_image_port() );
        p->connect( proc_mask_resize->image_port(),
                    pad_output_mask->set_value_port() );
      }
      else
      {
        p->connect( proc_metadata_mask->mask_port(),
                    pad_output_mask->set_value_port() );
      }
    }

    // Connect up downsampler, only enabling it if necessary.
    if( downsampler_enabled )
    {
      p->add( proc_downsampler );

      p->connect( proc_timestamper->image_port(),
                  proc_downsampler->set_input_image_port() );
      p->connect( proc_timestamper->timestamp_port(),
                  proc_downsampler->set_input_timestamp_port() );
      p->connect( proc_timestamper->metadata_port(),
                  proc_downsampler->set_input_metadata_port() );

      p->connect( proc_downsampler->get_output_timestamp_port(),
                  pad_output_timestamp->set_value_port() );
      p->connect( proc_downsampler->get_output_image_port(),
                  proc_image_resize->set_image_port() );

      if( masking_enabled )
      {
        p->connect( proc_downsampler->get_output_metadata_port(),
                    proc_metadata_mask->set_metadata_port() );

        if( full_resolution_masking )
        {
          p->connect( proc_downsampler->get_output_image_port(),
                      proc_metadata_mask->set_image_port() );
        }
      }
      else
      {
        p->connect( proc_downsampler->get_output_metadata_port(),
                    pad_output_metadata->set_value_port() );
      }
    }
    else
    {
      p->connect( proc_timestamper->timestamp_port(),
                  pad_output_timestamp->set_value_port() );

      p->connect( proc_timestamper->image_port(),
                  proc_image_resize->set_image_port() );

      if( masking_enabled )
      {
        p->connect( proc_timestamper->metadata_port(),
                    proc_metadata_mask->set_metadata_port() );

        if( full_resolution_masking )
        {
          p->connect( proc_timestamper->image_port(),
                      proc_metadata_mask->set_image_port() );
        }
      }
      else
      {
        p->connect( proc_timestamper->metadata_port(),
                    pad_output_metadata->set_value_port() );
      }
    }

    if( masking_enabled && !full_resolution_masking )
    {
      p->connect( proc_image_resize->image_port(),
                  proc_metadata_mask->set_image_port() );
    }

    // Add enhancer if enabled, otherwise just connect the resizer to output port.
    if( enhancer_enabled )
    {
      p->add( proc_enhancer );

      p->connect( proc_image_resize->image_port(),
                  proc_enhancer->set_source_image_port() );
      p->connect( proc_enhancer->copied_output_image_port(),
                  pad_output_image->set_value_port() );
    }
    else
    {
      p->connect( proc_image_resize->image_port(),
                  pad_output_image->set_value_port() );
    }
  }
}; // class preprocessing_super_process_impl


template< class PixType >
preprocessing_super_process< PixType >
::preprocessing_super_process( std::string const& _name )
  : super_process( _name, "preprocessing_super_process" ),
    impl_( NULL )
{
  this->impl_ = new preprocessing_super_process_impl< PixType >();

  this->impl_->config.add_parameter( "run_async",
    "false",
    "Should this pipeline be run in a sync or async configuration?" );

  this->impl_->config.add_parameter( "pipeline_edge_capacity",
    boost::lexical_cast< std::string >( 10 / sizeof( PixType ) ),
    "Maximum size of the edge in an async pipeline." );

  this->impl_->config.add_parameter( "masking_enabled",
    "false",
    "Whether or not to run with internal masking generation." );

  this->impl_->config.add_parameter( "full_resolution_masking",
    "true",
    "Whether or not to run mask detection at full resolution, or the"
    "resized image resolution." );
}


template< class PixType >
preprocessing_super_process< PixType >
::~preprocessing_super_process()
{
  delete this->impl_;
}


template< class PixType >
bool
preprocessing_super_process< PixType >
::initialize()
{
  return this->pipeline_->initialize();
}


template< class PixType >
config_block
preprocessing_super_process< PixType >
::params() const
{
  return impl_->config;
}


template< class PixType >
bool
preprocessing_super_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    impl_->config.update( blk );

    impl_->masking_enabled = blk.get< bool >( "masking_enabled" );
    impl_->full_resolution_masking = blk.get< bool >( "full_resolution_masking" );

    if( blk.get< bool >( "run_async" ) )
    {
      unsigned edge_capacity = blk.get< unsigned >( "pipeline_edge_capacity" );
      async_pipeline* p = new async_pipeline( async_pipeline::ENABLE_STATUS_FORWARDING, edge_capacity );

      this->impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else
    {
      sync_pipeline* p = new sync_pipeline;

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
preprocessing_super_process< PixType >
::step2()
{
  return this->pipeline_->execute();
}


template< class PixType >
bool
preprocessing_super_process< PixType >
::step()
{
  return ( this->step2() == process::SUCCESS );
}


template< class PixType >
bool
preprocessing_super_process< PixType >
::set_pipeline_to_monitor( const vidtk::pipeline* p )
{
  return impl_->proc_downsampler->set_pipeline_to_monitor( p );
}


template < class PixType >
void
preprocessing_super_process< PixType >
::set_image( vil_image_view< PixType > const& img )
{
  impl_->pad_source_image->set_value( img );
}


template < class PixType >
void
preprocessing_super_process< PixType >
::set_timestamp( vidtk::timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}


template < class PixType >
void
preprocessing_super_process< PixType >
::set_metadata( vidtk::video_metadata const& md )
{
  impl_->pad_source_metadata->set_value( md );
}


template< class PixType >
vil_image_view< PixType >
preprocessing_super_process< PixType >
::image() const
{
  return this->impl_->pad_output_image->value();
}


template< class PixType >
vil_image_view< bool >
preprocessing_super_process< PixType >
::mask() const
{
  return this->impl_->pad_output_mask->value();
}


template< class PixType >
vidtk::timestamp
preprocessing_super_process< PixType >
::timestamp() const
{
  return this->impl_->pad_output_timestamp->value();
}


template< class PixType >
vidtk::video_metadata
preprocessing_super_process< PixType >
::metadata() const
{
  return this->impl_->pad_output_metadata->value();
}

}
