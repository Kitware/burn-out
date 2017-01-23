/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "tot_super_process.h"

#include <descriptors/online_descriptor_computer_process.h>
#include <descriptors/tot_collector_process.h>

#include <utilities/track_demultiplexer_process.h>

#include <tracking_data/io/tot_writer_process.h>
#include <tracking_data/io/raw_descriptor_writer_process.h>

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <logger/logger.h>

namespace vidtk
{

VIDTK_LOGGER("tot_super_process_cxx");

template< typename PixType >
class tot_super_process_impl
{
public:

  // Pad Typedefs
  typedef vidtk::super_process_pad_impl< vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl< std::vector<track_sptr> > tracks_pad;
  typedef vidtk::super_process_pad_impl< raw_descriptor::vector_t  > descriptors_pad;
  typedef vidtk::super_process_pad_impl< timestamp  > timestamp_pad;
  typedef vidtk::super_process_pad_impl< image_to_utm_homography  > homography_pad;
  typedef vidtk::super_process_pad_impl< video_metadata > metadata_pad;
  typedef vidtk::super_process_pad_impl< video_modality > modality_pad;
  typedef vidtk::super_process_pad_impl< shot_break_flags > shot_break_pad;
  typedef vidtk::super_process_pad_impl< double > gsd_pad;

  // Internal Processes
  process_smart_pointer< online_descriptor_computer_process< vxl_byte > > proc_descriptor;
  process_smart_pointer< tot_collector_process > proc_classifer;
  process_smart_pointer< track_demultiplexer_process > proc_track_demux;
  process_smart_pointer< raw_descriptor_writer_process > proc_descriptor_writer;
  process_smart_pointer< tot_writer_process > proc_tot_writer;

  // Input Pads
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< tracks_pad > pad_source_tracks;
  process_smart_pointer< metadata_pad > pad_source_metadata;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< homography_pad > pad_source_homography;
  process_smart_pointer< modality_pad > pad_source_modality;
  process_smart_pointer< shot_break_pad > pad_source_shot_breaks;
  process_smart_pointer< gsd_pad > pad_source_gsd;

  // Output Pads
  process_smart_pointer< tracks_pad > pad_output_tracks;
  process_smart_pointer< descriptors_pad > pad_output_descriptors;

  // Internal Settings
  config_block config;
  config_block default_config;
  bool disabled;
  bool tot_writer_disabled;
  bool desc_writer_disabled;

  // Initialize Parameters List
  tot_super_process_impl() :
    config(),
    default_config(),
    disabled( false ),
    tot_writer_disabled( true ),
    desc_writer_disabled( true )
  {
    config.add_parameter(
      "disabled",
      "false",
      "Should this process be disabled?" );

    config.add_parameter(
      "run_async",
      "false",
      "Whether or not to run asynchronously." );
  }

  // Initialize Process Variables
  void create_process_configs()
  {
    // Pad initialization
    pad_source_image = new image_pad( "source_image_pad" );
    pad_source_tracks = new tracks_pad( "source_tracks_pad" );
    pad_source_metadata = new metadata_pad( "source_metadata_pad" );
    pad_source_timestamp = new timestamp_pad( "source_timestamp_pad" );
    pad_source_homography = new homography_pad( "source_homography_pad" );
    pad_source_modality = new modality_pad( "source_modality_pad" );
    pad_source_shot_breaks = new shot_break_pad( "source_shot_break_pad" );
    pad_source_gsd = new gsd_pad( "source_gsd_pad" );

    pad_output_tracks = new tracks_pad( "output_tracks_pad" );
    pad_output_descriptors = new descriptors_pad( "output_descriptors_pad" );

    // Process initialization
    proc_descriptor = new online_descriptor_computer_process< PixType >( "descriptor_generator" );
    config.add_subblock( proc_descriptor->params(), proc_descriptor->name() );

    proc_classifer = new tot_collector_process( "tot_classifier" );
    config.add_subblock( proc_classifer->params(), proc_classifer->name() );

    proc_track_demux = new track_demultiplexer_process( "track_demux" );
    config.add_subblock( proc_track_demux->params(), proc_track_demux->name() );

    proc_descriptor_writer = new raw_descriptor_writer_process( "descriptor_writer" );
    config.add_subblock( proc_descriptor_writer->params(), proc_descriptor_writer->name() );

    proc_tot_writer = new tot_writer_process( "tot_writer" );
    config.add_subblock( proc_tot_writer->params(), proc_tot_writer->name() );

    // Default config overrides
    default_config.add_parameter( proc_descriptor_writer->name() + ":disabled", "true", "" );
    default_config.add_parameter( proc_tot_writer->name() + ":disabled", "true", "" );
    config.update( default_config );
  }

  // Setup Port Connections
  template <typename Pipeline>
  void setup_pipeline( Pipeline* p )
  {
    p->add( pad_source_timestamp );
    p->add( pad_source_tracks );
    p->add( pad_output_tracks );

    if( this->disabled )
    {
      // pad_source_timestamp left disconnected intentionally, used as a cache time checked in step2()
      p->connect( pad_source_tracks->value_port(), pad_output_tracks->set_value_port() );
    }
    else
    {
      p->add( pad_source_image );
      p->add( pad_source_metadata );
      p->add( pad_source_homography );
      p->add( pad_source_modality );
      p->add( pad_source_shot_breaks );
      p->add( pad_source_gsd );

      p->add( proc_descriptor );

      p->connect( pad_source_image->value_port(),        proc_descriptor->set_source_image_port() );
      p->connect( pad_source_tracks->value_port(),       proc_descriptor->set_source_tracks_port() );
      p->connect( pad_source_metadata->value_port(),     proc_descriptor->set_source_metadata_port() );
      p->connect( pad_source_timestamp->value_port(),    proc_descriptor->set_source_timestamp_port() );
      p->connect( pad_source_homography->value_port(),   proc_descriptor->set_source_image_to_world_homography_port() );
      p->connect( pad_source_modality->value_port(),     proc_descriptor->set_source_modality_port() );
      p->connect( pad_source_shot_breaks->value_port(),  proc_descriptor->set_source_shot_break_flags_port() );
      p->connect( pad_source_gsd->value_port(),          proc_descriptor->set_source_gsd_port() );

      p->add( proc_classifer );

      p->connect( pad_source_tracks->value_port(),       proc_classifer->set_source_tracks_port() );
      p->connect( proc_descriptor->descriptors_port(),   proc_classifer->set_descriptors_port() );
      p->connect( pad_source_gsd->value_port(),          proc_classifer->set_source_gsd_port() );

      if( !tot_writer_disabled )
      {
        p->add( proc_track_demux );

        p->connect( proc_classifer->output_tracks_port(), proc_track_demux->input_tracks_port() );
        p->connect( pad_source_timestamp->value_port(),   proc_track_demux->input_timestamp_port() );

        p->add( proc_tot_writer );

        p->connect( proc_track_demux->output_terminated_tracks_port(), proc_tot_writer->set_tracks_port() );
      }

      if( !desc_writer_disabled )
      {
        p->add( proc_descriptor_writer );

        p->connect( proc_descriptor->descriptors_port(),  proc_descriptor_writer->set_descriptors_port() );
      }

      p->add( pad_output_descriptors );

      p->connect( proc_classifer->output_tracks_port(),   pad_output_tracks->set_value_port() );
      p->connect( proc_descriptor->descriptors_port(),    pad_output_descriptors->set_value_port() );
    } // else disabled
  } // setup_pipeline()
};

template< typename PixType >
tot_super_process< PixType >
::tot_super_process( std::string const& _name )
 : super_process( _name, "tot_super_process" ),
   impl_( new tot_super_process_impl<PixType>() )
{
  impl_->create_process_configs();
}

template< typename PixType >
tot_super_process< PixType >
::~tot_super_process()
{
}

template< typename PixType >
config_block
tot_super_process< PixType >
::params() const
{
  return impl_->config;
}

template< typename PixType >
bool
tot_super_process< PixType >
::set_params( config_block const& blk )
{
  try
  {
    // Update internal config block
    impl_->config.update( blk );

    // Check disabled flag, don't configure a pipeline if so.
    impl_->disabled = blk.get<bool>( "disabled" );

    if( impl_->disabled )
    {
      // NOTE - Even though we are disabled, we must initialize the
      // base class pipeline. This is used by the upper level process
      // to report statistics on super-processes.  Oh the burden of
      // being a super-process!
      sync_pipeline* p = new sync_pipeline;
      pipeline_ = p;
      impl_->setup_pipeline( p );
    }
    else
    {
      // Update parameters for this process
      bool run_async = blk.get<bool>( "run_async" );
      impl_->tot_writer_disabled = blk.get<bool>( impl_->proc_tot_writer->name() + ":disabled" );
      impl_->desc_writer_disabled = blk.get<bool>( impl_->proc_descriptor_writer->name() + ":disabled" );

      // Configure internal pipeline
      if( run_async )
      {
        LOG_INFO( "Starting async tot_super_process");
        async_pipeline* p = new async_pipeline( async_pipeline::ENABLE_STATUS_FORWARDING );
        pipeline_ = p;
        impl_->setup_pipeline( p );
      }
      else
      {
        LOG_INFO( "Starting sync tot_super_process" );
        sync_pipeline* p = new sync_pipeline;
        pipeline_ = p;
        impl_->setup_pipeline( p );
      }

      // Set internal parameters
      if( !pipeline_->set_params( impl_->config ) )
      {
        throw config_block_parse_error( "Unable to set pipeline parameters." );
      }
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what());
    return false;
  }

  return true;
}

template< typename PixType >
bool
tot_super_process< PixType >
::initialize()
{
  return ( this->pipeline_ ? this->pipeline_->initialize() : true );
}

template< typename PixType >
bool
tot_super_process< PixType >
::reset()
{
  return ( this->pipeline_ ? this->pipeline_->reset() : true );
}

template< typename PixType >
process::step_status
tot_super_process< PixType >
::step2()
{
  // Simply pass thru tracks if disabled
  if( impl_->disabled )
  {
    return impl_->pad_source_timestamp->value().is_valid() ? pipeline_->execute() : process::FAILURE;
  }

  // Step sync pipeline
  process::step_status p_status = pipeline_->execute();
  return p_status;
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_image( vil_image_view< PixType > const& image )
{
  impl_->pad_source_image->set_value( image );
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_image_res( vil_image_resource_sptr img )
{
  if( img )
  {
    vil_image_view_base_sptr v = img->get_view();
    impl_->pad_source_image->set_value( static_cast< vil_image_view<PixType> >( *v ) );
  }
}


template< typename PixType >
void
tot_super_process< PixType >
::set_source_tracks( std::vector< track_sptr > const& tracks )
{
  impl_->pad_source_tracks->set_value( tracks );
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_metadata( video_metadata const& meta )
{
  impl_->pad_source_metadata->set_value( meta );
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_timestamp( timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_image_to_world_homography( image_to_utm_homography const& homog )
{
  impl_->pad_source_homography->set_value( homog );
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_modality( video_modality modality )
{
  impl_->pad_source_modality->set_value( modality );
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_shot_break_flags( shot_break_flags sb_flags )
{
  impl_->pad_source_shot_breaks->set_value( sb_flags );
}

template< typename PixType >
void
tot_super_process< PixType >
::set_source_gsd( double gsd )
{
  impl_->pad_source_gsd->set_value( gsd );
}

template< typename PixType >
std::vector<track_sptr>
tot_super_process< PixType >
::output_tracks() const
{
  return impl_->pad_output_tracks->value();
}

template< typename PixType >
raw_descriptor::vector_t
tot_super_process< PixType >
::descriptors() const
{
  return impl_->pad_output_descriptors->value();
}

} //namespace vidtk
