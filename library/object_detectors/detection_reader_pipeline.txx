/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "detection_reader_pipeline.h"

namespace vidtk
{

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __detection_reader_pipeline_txx__
VIDTK_LOGGER ("detection_reader_pipeline");

template< class PixType >
detection_reader_pipeline< PixType >
::detection_reader_pipeline( std::string const& n )
  : detector_implementation_base< PixType >( n ),
    proc_obj_reader( NULL ),
    proc_pass( NULL)
{
  //
  // Minimize the amount of work done here since an object of this
  // type will be created during parameter collection. So you have to
  // create enough so that params() can be called.
  //
  detector_implementation_base< PixType >::create_base_pads();

  this->proc_obj_reader = new image_object_reader_process( "detection_reader" );
  this->proc_pass = new detector_pass_thru_process<PixType>("image_pass_thru");
}


template< class PixType >
detection_reader_pipeline< PixType >
::~detection_reader_pipeline()
{
}


// ----------------------------------------------------------------
template< class PixType >
config_block
detection_reader_pipeline< PixType >
::params()
{
  this->m_config.add_subblock( proc_obj_reader->params(),  proc_obj_reader->name() );

  this->m_config.add_parameter(
    "run_async",
    "true",
    "Whether or not to run asynchronously" );

  this->m_config.add_parameter(
    "pipeline_edge_capacity",
    boost::lexical_cast<std::string>(10 / sizeof(PixType)),
    "Maximum size of the edge in an async pipeline." );

  this->m_config.add_parameter(
    "gui_feedback_enabled",
    "false",
    "This option is not used by the object_reader" );

  this->m_config.add_parameter(
    "masking_enabled",
    "false",
    "This option is not used by the object_reader" );

  this->m_config.add_parameter(
    "location_type",
    "",
    "This option is not used by the object_reader" );

  return this->m_config;
}


// ----------------------------------------------------------------
template < class PixType >
bool detection_reader_pipeline< PixType >
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
    // Currently this pipeline is so simple, we are using sync only
    vidtk::sync_pipeline* p = new sync_pipeline;

    setup_pipeline( p );

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
template <class PixType >
template< class Pipeline >
void
detection_reader_pipeline< PixType >
::setup_pipeline( Pipeline * p )
{
  this->create_base_pads();
  this->setup_base_pipeline( p );

  p->add( proc_obj_reader );
  p->add( proc_pass );

  connect_pad2port( mask_image, proc_pass, set_input_mask_image );

  connect_pad2port( timestamp, proc_obj_reader, set_timestamp );
  connect_ports( this->proc_obj_reader, timestamp, this->proc_pass, set_input_timestamp);
  connect_port2pad( proc_pass, get_output_timestamp, timestamp );

  connect_ports( this->proc_obj_reader, objects, this->proc_pass, set_input_image_objects);
  connect_port2pad( this->proc_pass, get_output_image_objects, image_objects);


  connect_pass_thru( image);
  connect_pass_thru( src_to_ref_homography);
  connect_pass_thru( src_to_wld_homography );
  connect_pass_thru( src_to_utm_homography );
  connect_pass_thru( wld_to_src_homography );
  connect_pass_thru( wld_to_utm_homography );
  connect_pass_thru( ref_to_wld_homography );
  connect_pass_thru( world_units_per_pixel);
  connect_pass_thru( video_modality );
  connect_pass_thru( shot_break_flags );
  connect_pass_thru( gui_feedback);

  connect_port2pad( this->proc_pass, get_output_projected_objects, projected_objects );
  connect_port2pad( this->proc_pass, get_output_fg_image, fg_image );
  connect_port2pad( this->proc_pass, get_output_morph_image, morph_image );
  connect_port2pad( this->proc_pass, get_output_morph_grey_image, morph_grey_image );
  connect_port2pad( this->proc_pass, get_output_diff_image, diff_image );
  connect_port2pad( this->proc_pass, get_output_pixel_features, pixel_features );
}

} // end namespace
