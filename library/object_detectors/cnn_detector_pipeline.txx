/*ckwg +5
 * Copyright 2015-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "cnn_detector_pipeline.h"

#include <utilities/videoname_prefix.h>
#include <boost/lexical_cast.hpp>
#include <logger/logger.h>

#include <boost/make_shared.hpp>

namespace vidtk
{

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __cnn_detector_pipeline_txx__
VIDTK_LOGGER ("cnn_detector_pipeline");

// ----------------------------------------------------------------
template< class PixType >
cnn_detector_pipeline< PixType >
::cnn_detector_pipeline( std::string const& n )
  : detector_implementation_base< PixType >( n ),
    proc_cnn( NULL ),
    proc_pass( NULL ),
    proc_set_obj_type( NULL )
{
  //
  // Minimize the amount of work done here since an object of this
  // type will be created during parameter collection. So you have to
  // create enough so that get_params() can be called.
  //
  detector_implementation_base< PixType >::create_base_pads();

  // Create separate functors so the source information can be set based on the config if needed
  this->m_obj_type = boost::make_shared< set_type_functor >( vidtk::image_object::APPEARANCE, this->name() );

  proc_set_obj_type = new set_object_type_process( "set_motion_type", this->m_obj_type );

  // create ocv/cnn processes
  proc_cnn = new cnn_detector_process< PixType >( "cnn_detector" );
  proc_pass = new detector_pass_thru_process< PixType >( "pass_thru" );
}


template< class PixType >
cnn_detector_pipeline< PixType >
::~cnn_detector_pipeline()
{
}


// ----------------------------------------------------------------
template< class PixType >
config_block
cnn_detector_pipeline< PixType >
::params()
{

  // Collect config from processes
  this->m_config.add_subblock( proc_set_obj_type->params(), proc_set_obj_type->name() );

  this->m_config.add_subblock( proc_cnn->params(),         proc_cnn->name() );

  // Set parameters for this implementation. These parameters will be
  // reflected to all contained processes.
  this->m_config.add_parameter(
    "run_async",
    "false",
    "Whether or not to run asynchronously." );

  this->m_config.add_parameter(
    "pipeline_edge_capacity",
    boost::lexical_cast<std::string>(10 / sizeof(PixType)),
    "Maximum size of the edge in an async pipeline." );

  this->m_config.add_parameter(
    "image_object_source",
    "APPEARANCE",
    "Type code to use when marking origin of image objects produced by this detector. "
    "Allowable types are " + this->m_detector_source.list() );

  this->m_config.add_parameter(
    "location_type",
    "centroid",
    "Location of the target for tracking: bottom or centroid." );

  this->m_config.add_parameter(
    "gui_feedback_enabled",
    "false",
    "Whether we should enable gui feedback support in the pipeline." );

  this->m_config.add_parameter(
    "masking_enabled",
    "false",
    "Whether or not to run with metadata burnin masking support." );

  return this->m_config;
}


// ----------------------------------------------------------------
template < class PixType >
bool cnn_detector_pipeline< PixType >
::set_params( config_block const& blk )
{
  // Note that this is called as part of the video mode change restart
  // processing. Which means that there may be a pipeline already
  // active when we are called. The old pipeline should be
  // automatically deleted.

  // Save updated config.
  this->m_config = blk;

  try
  {
    int source_code = blk.get< int >( this->m_detector_source, "image_object_source" );
    this->m_obj_type->set_source( static_cast< vidtk::image_object::source_code >(source_code), this->name() );

    // ================================
    // now is the time to instantiate the pipeline
    bool opt_run_async = blk.get< bool > ( "run_async" );
    if ( opt_run_async )
    {
      unsigned edge_capacity = blk.get< unsigned > ( "pipeline_edge_capacity" );
      LOG_INFO( "Starting async object detector: " <<  this->name() );
      vidtk::async_pipeline* p = new async_pipeline( async_pipeline::ENABLE_STATUS_FORWARDING, edge_capacity );

      setup_pipeline( p );
    }
    else
    {
      LOG_INFO( "Starting sync object detector: " << this->name() );
      vidtk::sync_pipeline* p = new sync_pipeline;

      setup_pipeline( p );
    }

    // Send updated parameters to pipeline
    if ( ! this->pipeline()->set_params( this->m_config ) )
    {
      throw config_block_parse_error( " unable to set pipeline parameters." );
    }
  }
  catch ( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  return true;
} // set_params


// ----------------------------------------------------------------
// Setup sync or async pipeline
template< class PixType >
template< class Pipeline >
void
cnn_detector_pipeline< PixType >
::setup_pipeline( Pipeline * p )
{
  this->m_pipeline = p; // save generic pipeline pointer

  // Add processes to pipeline
  p->add( proc_cnn );
  p->add( proc_pass );
  p->add( proc_set_obj_type );

// Promote these macros into impl base if useful
#define connect_pad2port(PAD, PROC, PORT) \
  p->connect( this->pad_source_ ## PAD->value_port(), PROC->PORT ## _port());

#define connect_ports(PROC1, PORT1, PROC2, PORT2) \
  p->connect( PROC1-> PORT1 ## _port(), PROC2-> PORT2 ## _port() )

#define connect_port2pad(PROC, PORT, PAD) \
  p->connect( PROC->PORT ## _port(), this->pad_output_ ## PAD ->set_value_port() )

// special connect for pass thru
#define connect_pass_thru( PAD )                                        \
  p->connect( this->pad_source_ ## PAD->value_port(), proc_pass->set_input_ ## PAD ## _port() ); \
  p->connect( proc_pass->get_output_ ## PAD ## _port(), this->pad_output_ ## PAD ->set_value_port() )


  // connect ports
  connect_pad2port( image,                           proc_cnn, set_source_image );
  connect_pad2port( world_units_per_pixel,           proc_cnn, set_source_scale );

  // These just pass through
  connect_pass_thru( image );
  connect_pass_thru( timestamp );
  connect_pass_thru( src_to_ref_homography );
  connect_pass_thru( wld_to_src_homography );
  connect_pass_thru( src_to_utm_homography );
  connect_pass_thru( wld_to_utm_homography );
  connect_pass_thru( src_to_wld_homography );
  connect_pass_thru( world_units_per_pixel );
  connect_pass_thru( shot_break_flags );
  connect_pass_thru( video_modality );
  connect_pass_thru( ref_to_wld_homography );
  connect_pass_thru( gui_feedback );

  // Connect up empty array for async mode
  connect_port2pad( proc_pass, get_output_pixel_features,        pixel_features );

  // Connect process that sets object type
  connect_ports( proc_cnn, detections,                 proc_set_obj_type, input );

  // Duplicate output on two ports
  connect_port2pad( proc_set_obj_type, output,                    image_objects );
  connect_port2pad( proc_set_obj_type, output,                projected_objects );
}

} // end namespace vidtk
