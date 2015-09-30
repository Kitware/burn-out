/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "metadata_mask_super_process.h"

#include <pipeline/async_pipeline.h>
#include <pipeline/sync_pipeline.h>

#include <utilities/log.h>

#include <utilities/config_block.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/log.h>

#include <video/generic_frame_process.h>
#include <video/threshold_image_process.h>
#include <video/metadata_burnin_filter_process.h>
#include <video/identify_metadata_burnin_process.h>
#include <video/mask_image_process.h>
#include <video/mask_merge_process.h>
#include <video/mask_select_process.h>

#include <video/mask_overlay_process.h>
#include <video/image_list_writer_process.h>

namespace vidtk
{

template < class PixType >
class metadata_mask_super_process_impl
{
public:
  typedef vidtk::super_process_pad_impl< vil_image_view< vxl_byte > > image_pad;
  typedef vidtk::super_process_pad_impl<timestamp> timestamp_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > mask_pad;

  // Pipeline processes
  process_smart_pointer< generic_frame_process< bool > > proc_mask_reader;
  process_smart_pointer< metadata_burnin_filter_process > proc_mdb_filter;
  process_smart_pointer< mask_select_process > proc_mask_select;
  process_smart_pointer< identify_metadata_burnin_process > proc_burnin_detect1;
  process_smart_pointer< identify_metadata_burnin_process > proc_burnin_detect2;
  process_smart_pointer< mask_merge_process > proc_burnin_mask_merge;
  process_smart_pointer< mask_merge_process > proc_mask_merge;

  process_smart_pointer< mask_overlay_process > proc_mask_overlay;
  process_smart_pointer< image_list_writer_process< vxl_byte > > proc_overlay_writer;
  process_smart_pointer< image_list_writer_process< vxl_byte > > proc_filter_writer;
  process_smart_pointer< image_list_writer_process< bool > > proc_mask_writer;

  // Input Pads (dummy processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;

  // Output Pads (dummy processes)
  process_smart_pointer< mask_pad > pad_output_mask;

  // Configuration parameters
  config_block config;
  config_block default_config;

  // CTOR
  metadata_mask_super_process_impl()
  : proc_mask_reader( NULL ),
    proc_mdb_filter( NULL ),
    proc_mask_select( NULL ),
    proc_burnin_detect1( NULL ),
    proc_burnin_detect2( NULL ),
    proc_burnin_mask_merge( NULL ),
    proc_mask_merge( NULL ),
    proc_mask_overlay( NULL ),
    proc_overlay_writer( NULL ),
    proc_filter_writer( NULL ),
    proc_mask_writer( NULL ),
    // input pads
    pad_source_image( NULL ),
    pad_source_timestamp( NULL ),
    // output pads
    pad_output_mask( NULL )
  {
  }

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    pad_source_image = new image_pad("source_image");
    pad_source_timestamp = new timestamp_pad("source_timestamp");

    pad_output_mask = new mask_pad("output_mask");

    proc_mask_reader = new generic_frame_process< bool >( "metadata_mask_reader" );
    config.add_subblock( proc_mask_reader->params(), proc_mask_reader->name() );

    proc_mdb_filter = new metadata_burnin_filter_process( "mdb_filter" );
    config.add_subblock( proc_mdb_filter->params(), proc_mdb_filter->name() );

    proc_mask_select = new mask_select_process( "mask_select" );
    config.add_subblock( proc_mask_select->params(), proc_mask_select->name() );

    proc_burnin_detect1 = new identify_metadata_burnin_process( "burnin_detect1" );
    config.add_subblock( proc_burnin_detect1->params(),
                         proc_burnin_detect1->name() );

    proc_burnin_detect2 = new identify_metadata_burnin_process( "burnin_detect2" );
    config.add_subblock( proc_burnin_detect2->params(),
                         proc_burnin_detect2->name() );

    proc_burnin_mask_merge = new mask_merge_process( "burnin_mask_merge" );
    config.add_subblock( proc_burnin_mask_merge->params(),
                         proc_burnin_mask_merge->name() );

    proc_mask_merge = new mask_merge_process( "mask_merge" );
    config.add_subblock( proc_mask_merge->params(),
                         proc_mask_merge->name() );

    proc_mask_overlay = new mask_overlay_process( "mask_overlay" );
    config.add_subblock( proc_mask_overlay->params(),
                         proc_mask_overlay->name() );

    proc_overlay_writer = new image_list_writer_process< vxl_byte >( "overlay_writer" );
    config.add_subblock( proc_overlay_writer->params(),
                         proc_overlay_writer->name() );

    proc_filter_writer = new image_list_writer_process< vxl_byte >( "filter_writer" );
    config.add_subblock( proc_filter_writer->params(),
                         proc_filter_writer->name() );

    proc_mask_writer = new image_list_writer_process< bool >( "mask_writer" );
    config.add_subblock( proc_mask_writer->params(),
                         proc_mask_writer->name() );

    // Over-riding the process default with this super-process defaults.
    default_config.add( proc_mask_reader->name() + ":type", "image_list" );
    default_config.add( proc_mask_reader->name() + ":read_only_first_frame", "true" );
    default_config.add( proc_overlay_writer->name() + ":disabled", "true");
    default_config.add( proc_overlay_writer->name() + ":pattern", "overlay-%2$04d.png");
    default_config.add( proc_filter_writer->name() + ":disabled", "true");
    default_config.add( proc_filter_writer->name() + ":pattern", "filter-%2$04d.png");
    default_config.add( proc_mask_writer->name() + ":disabled", "true");
    default_config.add( proc_mask_writer->name() + ":pattern", "mask-%2$04d.png");
    config.update( default_config );
  }

  template <class PIPELINE>
  void setup_pipeline( PIPELINE * p )
  {

    p->add( proc_mask_reader );
    p->add( pad_source_image );
    p->add( proc_mdb_filter );
    p->add( proc_mask_select );
    p->add( proc_burnin_detect1 );
    p->add( proc_burnin_detect2 );
    p->add( proc_burnin_mask_merge );
    p->add( proc_mask_merge );
    p->add( pad_output_mask );

     /*
      *    src
      *     |
      *     v
      *  mdb_filter
      */
    p->connect( pad_source_image->value_port(),
                proc_mdb_filter->set_source_image_port() );


     /*
      *  mdb_filter   reader
      *     |           |
      *     v           |
      *  mask_select <---
      */
    p->connect( proc_mdb_filter->metadata_image_port(),
                proc_mask_select->set_data_image_port() );
    p->connect( proc_mask_reader->image_port(),
                proc_mask_select->set_mask_port() );


    /*
      *  mdb_filter
      *     |
      *     v
      *  detect1
      */
    p->connect( proc_mdb_filter->metadata_image_port(),
                proc_burnin_detect1->set_source_image_port() );

    /*
      *  mdb_filter
      *     |
      *     v
      *  detect2
      */
    p->connect( proc_mdb_filter->metadata_image_port(),
                proc_burnin_detect2->set_source_image_port() );

    /*
      *  detect1  detect2
      *     |        |
      *     v        |
      *   mmerge <----
      *     |
      *     v
      *   merge
      */
    p->connect( proc_burnin_detect1->metadata_mask_port(),
                proc_burnin_mask_merge->set_mask_a_port() );
    p->connect( proc_burnin_detect2->metadata_mask_port(),
                proc_burnin_mask_merge->set_mask_b_port() );
    p->connect( proc_burnin_mask_merge->mask_port(),
                proc_mask_merge->set_mask_a_port() );


    /*
      *          mask_select
      *              |
      *              |
      *   merge <-----
      *     |
      *     v
      *    pad
      */
    p->connect( proc_mask_select->mask_port(),
                proc_mask_merge->set_mask_b_port() );
    p->connect( proc_mask_merge->mask_port(),
                pad_output_mask->set_value_port() );


    bool overlay_writer_disabled(true);
    config.get( proc_overlay_writer->name() + ":disabled", overlay_writer_disabled );

    bool filter_writer_disabled(true);
    config.get( proc_filter_writer->name() + ":disabled", filter_writer_disabled );

    bool mask_writer_disabled(true);
    config.get( proc_mask_writer->name() + ":disabled", mask_writer_disabled );

    if ( !overlay_writer_disabled ||
         !filter_writer_disabled ||
         !mask_writer_disabled )
    {
      // only add timestamper pad if some writer is going to use it.
      // otherwise the pipeline will see it as an output pad and
      // the pipeline will never terminate.
      p->add( pad_source_timestamp );
    }

    if ( !overlay_writer_disabled )
    {
      p->add( proc_mask_overlay );
      p->add( proc_overlay_writer );

      /*
        *    src     merge
        *     |        |
        *     v        |
        *   overlay <---
        *     |
        *     v
        *  overlay_writer
        */
      p->connect( pad_source_image->value_port(),
                  proc_mask_overlay->set_source_image_port() );
      p->connect( proc_mask_merge->mask_port(),
                  proc_mask_overlay->set_mask_port() );
      p->connect( proc_mask_overlay->image_port(),
                  proc_overlay_writer->set_image_port() );
      p->connect( pad_source_timestamp->value_port(),
                  proc_overlay_writer->set_timestamp_port() );
    }

    if ( !filter_writer_disabled )
    {
      p->add( proc_filter_writer );
       /*
        *  mdb_filter
        *     |
        *     v
        *  filter_writer
        */
      p->connect( proc_mdb_filter->metadata_image_port(),
                  proc_filter_writer->set_image_port() );
      p->connect( pad_source_timestamp->value_port(),
                  proc_filter_writer->set_timestamp_port() );
    }

    if ( !mask_writer_disabled )
    {
      p->add( proc_mask_writer );
       /*
        *   merge
        *     |
        *     v
        *  mask_writer
        */
      p->connect( proc_mask_merge->mask_port(),
                  proc_mask_writer->set_image_port() );
      p->connect( pad_source_timestamp->value_port(),
                  proc_mask_writer->set_timestamp_port() );
    }

  }
};

template < class PixType >
metadata_mask_super_process<PixType>
::metadata_mask_super_process( vcl_string const& name )
  : super_process( name, "metadata_mask_super_process" ),
    impl_( NULL )
{
  impl_ = new metadata_mask_super_process_impl<PixType>;

  impl_->create_process_configs();

  impl_->config.add_parameter( "run_async",
    "false",
    "Whether or not to run asynchronously" );
}

template < class PixType >
metadata_mask_super_process<PixType>
::~metadata_mask_super_process()
{
  delete impl_;
}

template < class PixType >
config_block
metadata_mask_super_process<PixType>
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  return tmp_config;
}

template < class PixType >
bool
metadata_mask_super_process<PixType>
::set_params( config_block const& blk )
{
  typedef metadata_mask_super_process_impl<PixType> impl_t;
  try
  {
    bool run_async = false;
    blk.get( "run_async", run_async );

    impl_->config.update( blk );

    if( run_async )
    {
      async_pipeline* p = new async_pipeline(async_pipeline::ENABLE_STATUS_FOWARDING);

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else
    {
      sync_pipeline* p = new sync_pipeline;

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }

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
template < class PixType >
bool
metadata_mask_super_process<PixType>
::initialize()
{
  return this->pipeline_->initialize();
}


// ----------------------------------------------------------------
template < class PixType >
process::step_status
metadata_mask_super_process<PixType>
::step2()
{
  return this->pipeline_->execute();
}


template < class PixType >
void
metadata_mask_super_process<PixType>
::set_source_image( vil_image_view< vxl_byte > const& img )
{
  impl_->pad_source_image->set_value( img );
}

template< class PixType>
void
metadata_mask_super_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}

template < class PixType >
vil_image_view< bool >
metadata_mask_super_process<PixType>
::get_mask ( ) const
{
  return impl_->pad_output_mask->value();
}


} // end namespace vidtk
