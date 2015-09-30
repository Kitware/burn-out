/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "stabilization_super_process.h"

#include <pipeline/async_pipeline.h>
#include <pipeline/sync_pipeline.h>

#include <kwklt/klt_pyramid_process.h>
#include <utilities/log.h>
#include <video/greyscale_process.h>
#include <tracking/klt_homography_super_process.h>

#include <tracking/shot_break_flags.h>
#include <tracking/shot_stitching_process.h>
#include <tracking/stab_pass_thru_process.h>

#include <video/metadata_mask_super_process.h>
#include <utilities/config_block.h>
#include <utilities/homography_reader_process.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/timestamp_reader_process.h>
#include <utilities/tagged_data_writer_process.h>

#include <video/greyscale_process.h>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER ("stabilization_super_process");

template< class PixType>
class stabilization_super_process_impl
{
public:
  typedef vidtk::super_process_pad_impl<vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl<timestamp> timestamp_pad;
  typedef vidtk::super_process_pad_impl< video_metadata > metadata_pad;
  typedef vidtk::super_process_pad_impl< vcl_vector< video_metadata > > metadata_vec_pad;
  typedef vidtk::super_process_pad_impl< image_to_image_homography > homog_pad;
  typedef vidtk::super_process_pad_impl< vcl_vector< klt_track_ptr > > klt_track_pad;
  typedef vidtk::super_process_pad_impl< timestamp::vector_t > timestamp_vec_pad;
  typedef vidtk::super_process_pad_impl< shot_break_flags > shot_break_flags_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > mask_pad;

  // Pipeline processes
  process_smart_pointer< timestamp_reader_process<vxl_byte> > proc_timestamper;
  process_smart_pointer< homography_reader_process > proc_homog_reader;
  process_smart_pointer< greyscale_process<PixType> > proc_convert_to_grey;
  process_smart_pointer< klt_pyramid_process > proc_klt_pyramid;
  process_smart_pointer< klt_homography_super_process > proc_klt_homography;
  process_smart_pointer< metadata_mask_super_process< PixType > > proc_metadata_mask;
  process_smart_pointer< shot_stitching_process< PixType > > proc_shot_stitcher;
  process_smart_pointer< tagged_data_writer_process > proc_data_write;
  process_smart_pointer< stab_pass_thru_process<PixType> > proc_pass;

  // Input Pads (dummy processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< metadata_pad > pad_source_metadata;

  // Output Pads (dummy processes)
  process_smart_pointer< timestamp_pad > pad_output_timestamp;
  process_smart_pointer< image_pad > pad_output_image;
  process_smart_pointer< metadata_vec_pad > pad_output_metadata;
  process_smart_pointer< homog_pad > pad_output_src2ref_homog;
  process_smart_pointer< klt_track_pad > pad_output_active_tracks;
  process_smart_pointer< timestamp_vec_pad > pad_output_timestamp_vector;
  process_smart_pointer< shot_break_flags_pad > pad_output_shot_break_flags;
  process_smart_pointer< mask_pad > pad_output_mask;

  // Configuration parameters
  config_block config;
  enum sp_mode{ VIDTK_LOAD, VIDTK_COMPUTE, VIDTK_DISABLED };
  sp_mode mode;

  bool tdw_enable;

  // CTOR
  stabilization_super_process_impl()
  : proc_timestamper( NULL ),
    proc_homog_reader( NULL ),
    proc_convert_to_grey( NULL ),
    proc_klt_pyramid( NULL ),
    proc_klt_homography( NULL ),
    proc_metadata_mask( NULL ),
    proc_shot_stitcher( NULL ),
    proc_data_write( NULL ),
    proc_pass( NULL ),

    // input pads
    pad_source_image( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_metadata( NULL ),

    // output pads
    pad_output_timestamp( NULL ),
    pad_output_image( NULL ),
    pad_output_metadata( NULL ),
    pad_output_src2ref_homog( NULL ),
    pad_output_active_tracks( NULL ),
    pad_output_timestamp_vector( NULL ),
    pad_output_shot_break_flags( NULL ),
    pad_output_mask( NULL ),
    mode( VIDTK_DISABLED ),
    tdw_enable(false)
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
    pad_source_timestamp = new timestamp_pad("source_timestamp");
    pad_source_metadata = new metadata_pad("source_metadata");

    pad_output_timestamp = new timestamp_pad("output_timestamp");
    pad_output_image = new image_pad("output_image");
    pad_output_metadata = new metadata_vec_pad("output_metadata");
    pad_output_src2ref_homog = new homog_pad("output_src2ref_homog");
    pad_output_active_tracks = new klt_track_pad("output_active_tracks");
    pad_output_timestamp_vector = new timestamp_vec_pad("output_timestamp_vec");
    pad_output_shot_break_flags = new shot_break_flags_pad("output_shot_break_flags");
    pad_output_mask = new mask_pad("output_mask");

    proc_timestamper = new vidtk::timestamp_reader_process<vxl_byte>( "timestamper" );
    config.add_subblock( proc_timestamper->params(), proc_timestamper->name() );

    proc_homog_reader = new homography_reader_process( "homog_reader" );
    config.add_subblock( proc_homog_reader->params(),
                         proc_homog_reader->name() );

    proc_convert_to_grey = new greyscale_process<PixType>( "rgb_to_grey" );
    config.add_subblock( proc_convert_to_grey->params(),
                         proc_convert_to_grey->name() );

    proc_klt_pyramid = new klt_pyramid_process( "klt_pyramid" );
    config.add_subblock( proc_klt_pyramid->params(),
                         proc_klt_pyramid->name() );

    proc_klt_homography = new klt_homography_super_process( "klt_homog_sp" );
    config.add_subblock( proc_klt_homography->params(),
                         proc_klt_homography->name() );

    proc_metadata_mask = new metadata_mask_super_process< PixType >( "md_mask_sp" );
    config.add_subblock( proc_metadata_mask->params(),
                         proc_metadata_mask->name() );

    proc_shot_stitcher = new shot_stitching_process< PixType >( "shot_stitcher" );
    config.add_subblock( proc_shot_stitcher->params(),
                         proc_shot_stitcher->name() );

    proc_data_write = new tagged_data_writer_process("tagged_data_writer");
    config.add_subblock( proc_data_write->params(), proc_data_write->name() );

    proc_pass = new stab_pass_thru_process<PixType>( "stab_pass_thru" );
    config.add_subblock( proc_pass->params(), proc_pass->name() );
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
    p->connect( proc_timestamper->timestamp_vector_port(), proc_pass->set_input_ts_vector_port() );
    p->connect( proc_timestamper->metadata_port(),         proc_pass->set_input_metadata_vector_port() );

    p->add( pad_output_image );
    p->add( pad_output_timestamp );
    p->add( pad_output_timestamp_vector );
    p->add( pad_output_metadata );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),           pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),       pad_output_timestamp->set_value_port() );
    p->connect( proc_pass->get_output_ts_vector_port(),       pad_output_timestamp_vector->set_value_port() );
    p->connect( proc_pass->get_output_metadata_vector_port(), pad_output_metadata->set_value_port() );
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

    // Add homog reader to create the src2ref homog
    p->add( proc_homog_reader );
    p->connect( proc_timestamper->timestamp_port(),   proc_homog_reader->set_timestamp_port() );

    p->add( proc_pass );
    p->connect( proc_timestamper->image_port(),            proc_pass->set_input_image_port() );
    p->connect( proc_timestamper->timestamp_port(),        proc_pass->set_input_timestamp_port() );
    p->connect( proc_timestamper->timestamp_vector_port(), proc_pass->set_input_ts_vector_port() );
    p->connect( proc_timestamper->metadata_port(),         proc_pass->set_input_metadata_vector_port() );
    p->connect( proc_homog_reader->image_to_world_vidtk_homography_image_port(), proc_pass->set_input_homography_port() );

    p->add( pad_output_image );
    p->add( pad_output_timestamp );
    p->add( pad_output_timestamp_vector );
    p->add( pad_output_metadata );
    p->add( pad_output_src2ref_homog );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),           pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),       pad_output_timestamp->set_value_port() );
    p->connect( proc_pass->get_output_metadata_vector_port(), pad_output_metadata->set_value_port() );
    p->connect( proc_pass->get_output_ts_vector_port(),       pad_output_timestamp_vector->set_value_port() );
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

    p->add( proc_convert_to_grey );
    p->connect( proc_timestamper->image_port(),       proc_convert_to_grey->set_image_port() );

    p->add( proc_klt_pyramid );
    p->connect( proc_convert_to_grey->copied_image_port(), proc_klt_pyramid->set_image_port() );

    p->add( proc_klt_homography );
    p->connect( proc_timestamper->timestamp_port(),           proc_klt_homography->set_timestamp_port() );
    p->connect( proc_klt_pyramid->image_pyramid_port(),       proc_klt_homography->set_image_pyramid_port() );
    p->connect( proc_klt_pyramid->image_pyramid_gradx_port(), proc_klt_homography->set_image_pyramid_gradx_port() );
    p->connect( proc_klt_pyramid->image_pyramid_grady_port(), proc_klt_homography->set_image_pyramid_grady_port() );

    p->add( proc_shot_stitcher );
    p->connect( proc_klt_homography->active_tracks_port(),               proc_shot_stitcher->input_klt_tracks_port() );
    p->connect( proc_timestamper->image_port(),                          proc_shot_stitcher->input_image_port() );
    p->connect( proc_timestamper->timestamp_port(),                      proc_shot_stitcher->input_timestamp_port() );
    p->connect( proc_timestamper->timestamp_vector_port(),               proc_shot_stitcher->input_timestamp_vector_port() );
    p->connect( proc_timestamper->metadata_port(),                       proc_shot_stitcher->input_metadata_vector_port() );
    p->connect( proc_klt_homography->src_to_ref_homography_port(),       proc_shot_stitcher->input_src2ref_h_port() );
    p->connect( proc_klt_homography->get_output_shot_break_flags_port(), proc_shot_stitcher->input_shot_break_flags_port() );

    p->add( proc_pass );

    // Test for masking support enabled
    bool masking_enabled (false);

    config.get( "masking_enabled", masking_enabled );
    if( masking_enabled )
    {
      p->add( proc_metadata_mask );
      p->connect( proc_convert_to_grey->copied_image_port(), proc_metadata_mask->set_source_image_port() );
      p->connect( proc_timestamper->timestamp_port(),        proc_metadata_mask->set_source_timestamp_port() );
      p->connect( proc_metadata_mask->get_mask_port(),       proc_klt_homography->set_mask_port() );

      p->add( pad_output_mask );
      p->connect( proc_metadata_mask->get_mask_port(),       proc_shot_stitcher->input_metadata_mask_port() );
      p->connect( proc_shot_stitcher->output_metadata_mask_port(), proc_pass->set_input_mask_port() );
      p->connect( proc_pass->get_output_mask_port(), pad_output_mask->set_value_port() );
    } // end masking enabled


    // connect to pass-thru process
    p->connect( proc_shot_stitcher->output_image_port(),            proc_pass->set_input_image_port() );
    p->connect( proc_shot_stitcher->output_klt_tracks_port(),       proc_pass->set_input_klt_tracks_port() );
    p->connect( proc_shot_stitcher->output_metadata_vector_port(),  proc_pass->set_input_metadata_vector_port() );
    p->connect( proc_shot_stitcher->output_shot_break_flags_port(), proc_pass->set_input_shot_break_flags_port() );
    p->connect( proc_shot_stitcher->output_src2ref_h_port(),        proc_pass->set_input_homography_port() );
    p->connect( proc_shot_stitcher->output_timestamp_port(),        proc_pass->set_input_timestamp_port() );
    p->connect( proc_shot_stitcher->output_timestamp_vector_port(), proc_pass->set_input_ts_vector_port() );

    // Connect to output pads
    p->add( pad_output_image );
    p->add( pad_output_active_tracks );
    p->add( pad_output_metadata );
    p->add( pad_output_shot_break_flags );
    p->add( pad_output_src2ref_homog );
    p->add( pad_output_timestamp );
    p->add( pad_output_timestamp_vector );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),            pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_klt_tracks_port(),       pad_output_active_tracks->set_value_port() );
    p->connect( proc_pass->get_output_metadata_vector_port(),  pad_output_metadata->set_value_port() );
    p->connect( proc_pass->get_output_shot_break_flags_port(), pad_output_shot_break_flags->set_value_port() );
    p->connect( proc_pass->get_output_homography_port(),       pad_output_src2ref_homog->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),        pad_output_timestamp->set_value_port() );
    p->connect( proc_pass->get_output_ts_vector_port(),        pad_output_timestamp_vector->set_value_port() );
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

    p->connect( proc_pass->get_output_image_port(),           proc_data_write->set_input_image_port() );
    proc_data_write->set_image_connected (true);

    p->connect( proc_pass->get_output_metadata_vector_port(), proc_data_write->set_input_video_metadata_vector_port() );
    proc_data_write->set_video_metadata_vector_connected (true);

    p->connect( proc_pass->get_output_homography_port(),      proc_data_write->set_input_src_to_ref_homography_port() );
    proc_data_write->set_src_to_ref_homography_connected (true);

    p->connect( proc_pass->get_output_shot_break_flags_port(), proc_data_write->set_input_shot_break_flags_port() );
    proc_data_write->set_shot_break_flags_connected (true);

    bool masking_enabled;
    config.get( "masking_enabled", masking_enabled );
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

    case VIDTK_DISABLED:
      setup_pipeline_disabled_mode(p);
      break;
    } // end switch

    // Attach data writer if specified.
    setup_pipeline_data_writer(p);
  }

}; // end class



template< class PixType>
stabilization_super_process<PixType>
::stabilization_super_process( vcl_string const& name )
  : super_process( name, "stabilization_super_process" ),
    impl_( new stabilization_super_process_impl<PixType> )
{
  impl_->create_process_configs();

  impl_->config.add_parameter( "run_async",
    "false",
    "Whether or not to run asynchronously" );

  impl_->config.add_parameter( "mode",
    "disabled",
    "Choose between load, compute, or disabled, i.e. load img2ref "
    "homographies from a file, compute new img2ref homographies, or disable "
    "this process and supply identity matrices by default." );

  impl_->config.add_parameter( "masking_enabled",
    "false",
    "Whether or not to run with masking support" );
}

template< class PixType>
stabilization_super_process<PixType>
::~stabilization_super_process()
{
  delete impl_;
}

template< class PixType>
config_block
stabilization_super_process<PixType>
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  return tmp_config;
}

template< class PixType>
bool
stabilization_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    bool run_async = false;
    blk.get( "run_async", run_async );

    vcl_string str = blk.get<vcl_string>( "mode" );
    if( str == "load" )
    {
      impl_->mode = impl_->VIDTK_LOAD;
    }
    else if( str == "compute" )
    {
      impl_->mode = impl_->VIDTK_COMPUTE;
    }
    else if( str == "disabled" )
    {
      impl_->mode = impl_->VIDTK_DISABLED;
    }
    else
    {
      LOG_ERROR( this->name() << ": Invalid stabilization mode \""<<str << "\"" );
      return false;
    }

    impl_->config.update( blk );

    impl_->config.get(impl_->proc_data_write->name() + ":enabled", impl_->tdw_enable);

    if (impl_->tdw_enable)
    {
      // Update parameters that have video name prefix.
      add_videoname_prefix( impl_->config, impl_->proc_data_write->name() + ":filename" );
    }

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

    impl_->config.set( impl_->proc_metadata_mask->name() + ":run_async", run_async );

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw unchecked_return_value( " unable to set pipeline parameters." );
    }

  }
  catch( unchecked_return_value& e)
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

template< class PixType >
timestamp const&
stabilization_super_process< PixType >
::source_timestamp() const
{
  return impl_->pad_output_timestamp->value();
}

template< class PixType >
vil_image_view< PixType > const&
stabilization_super_process< PixType >
::source_image() const
{
  return impl_->pad_output_image->value();
}

template< class PixType>
image_to_image_homography const &
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
video_metadata::vector_t const&
stabilization_super_process<PixType>
::output_metadata_vector() const
{
  return impl_->pad_output_metadata->value();
}


template< class PixType>
vcl_vector< klt_track_ptr > const&
stabilization_super_process<PixType>
::active_tracks() const
{
  return impl_->pad_output_active_tracks->value();
}


template< class PixType >
vidtk::timestamp::vector_t const&
stabilization_super_process< PixType >::
get_output_timestamp_vector() const
{
  // fetch from timestamper process
  return impl_->pad_output_timestamp_vector->value();
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
