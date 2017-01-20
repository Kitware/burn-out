/*ckwg +5
 * Copyright 2014-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "motion_and_saliency_detector_pipeline.h"

#include <utilities/videoname_prefix.h>

#include <vul/vul_awk.h>
#include <vul/vul_sprintf.h>


namespace vidtk
{

template< class PixType >
motion_and_saliency_detector_pipeline< PixType >
::motion_and_saliency_detector_pipeline( std::string const& n )
  : detector_implementation_base< PixType >( n ),
    proc_diff_sp( NULL ),
    proc_conn_comp1_sp( NULL ),
    proc_conn_comp2_sp( NULL ),
    proc_merge( NULL ),
    proc_set_obj_type_1( NULL ),
    proc_set_obj_type_2( NULL ),
    proc_set_proj_obj_type( NULL )
{
  //
  // Minimize the amount of work done here since an object of this
  // type will be created during parameter collection. So you have to
  // create enough so that get_params() can be called.
  //
  detector_implementation_base< PixType >::create_base_pads();

  proc_diff_sp = new diff_super_process< PixType >( "diff_sp" );
  proc_conn_comp1_sp = new conn_comp_super_process< PixType >( "conn_comp_sp" );
  proc_conn_comp2_sp = new conn_comp_super_process< PixType >( "conn_comp_2_sp" );
  proc_merge = new merge_image_object_process( "merge" );

    // Create separate functors so the source information can be set based on the config if needed
  this->m_obj_type_1 = boost::make_shared<set_type_functor > (vidtk::image_object::MOTION, this->name() + "_1");
  this->m_obj_type_2 = boost::make_shared<set_type_functor > (vidtk::image_object::MOTION, this->name() + "_2");
  this->m_proj_obj_type = boost::make_shared<set_type_functor > (vidtk::image_object::MOTION, this->name());

  proc_set_obj_type_1 = new set_object_type_process( "set_motion_type_1", this->m_obj_type_1 );
  proc_set_obj_type_2 = new set_object_type_process( "set_motion_type_2", this->m_obj_type_2 );
  proc_set_proj_obj_type = new set_object_type_process( "set_motion_type", this->m_proj_obj_type );
}


template< class PixType >
motion_and_saliency_detector_pipeline< PixType >
::~motion_and_saliency_detector_pipeline()
{
}


// ----------------------------------------------------------------
// Create process objects
template< class PixType >
config_block
motion_and_saliency_detector_pipeline< PixType >
::params()
{
  // Get parameters from our processes
  this->m_config.add_subblock( proc_diff_sp->params(),       proc_diff_sp->name() );
  this->m_config.add_subblock( proc_conn_comp1_sp->params(), proc_conn_comp1_sp->name() );
  this->m_config.add_subblock( proc_conn_comp2_sp->params(), proc_conn_comp2_sp->name() );
  this->m_config.add_subblock( proc_merge->params(),         proc_merge->name() );
  this->m_config.add_subblock( proc_set_obj_type_1->params(),  proc_set_obj_type_1->name() );
  this->m_config.add_subblock( proc_set_obj_type_2->params(),  proc_set_obj_type_2->name() );
  this->m_config.add_subblock( proc_set_proj_obj_type->params(), proc_set_proj_obj_type->name() );

  // Set parameters for this implementation. These parameters will be
  // reflected to all contained processes.
  this->m_config.add_parameter(
    "masking_enabled",
    "false",
    "Whether or not to run with metadata burnin masking support" );

  this->m_config.add_parameter(
    "run_async",
    "true",
    "Whether or not to run asynchronously" );

  this->m_config.add_parameter(
    "location_type",
    "centroid",
    "Location of the target for tracking: bottom or centroid.");

  this->m_config.add_parameter(
    "gui_feedback_enabled",
    "false",
    "Whether we should enable gui feedback support in the pipeline" );

  this->m_config.add_parameter(
    "pipeline_edge_capacity",
    boost::lexical_cast<std::string>( 10 / sizeof( PixType ) ),
    "Maximum size of the edge in an async pipeline." );

  this->m_config.add_parameter(
    "max_workable_gsd",
    "1.25",
    "Threshold (in meters-per-pixel) on GSD above which we don't try to "
    "detect or track objects until the end of the current shot." );

  this->m_config.add_parameter(
    "image_object_source",
    "MOTION",
    "Type code to use when marking origin of image objects produced by this detector. "
    "Allowable types are " + this->m_detector_source.list() );

  return this->m_config;
}


// ----------------------------------------------------------------
// Create process objects
template< class PixType >
bool
motion_and_saliency_detector_pipeline< PixType >
::set_params( config_block const& blk )
{
  // Note that this is called as part of the video mode change restart
  // processing. Which means that there may be a pipeline already
  // active when we are called. The old pipeline should be
  // automatically deleted.

  // Save updated config.
  this->m_config = blk;

  config_block dependency_config;

  std::string masking_enabled;
  std::string run_async;
  std::string location_type;
  std::string ul, lr;

  try
  {
    // extract options that we will be needing here
    m_gui_feedback_enabled = blk.get< bool >( "gui_feedback_enabled" );
    m_max_workable_gsd     = blk.get< double >( "max_workable_gsd" );

    masking_enabled        = blk.get< std::string >( "masking_enabled" );
    run_async              = blk.get< std::string >( "run_async" );
    location_type          = blk.get< std::string >( "location_type" );
    ul                     = blk.get< std::string >( proc_diff_sp->name() + ":source_crop:upper_left" );
    lr                     = blk.get< std::string >( proc_diff_sp->name() + ":source_crop:lower_right" );

    int source_code = blk.get< int >( this->m_detector_source, "image_object_source" );
    this->m_obj_type_1->set_source( static_cast< vidtk::image_object::source_code >(source_code), this->name() );
    this->m_obj_type_2->set_source( static_cast< vidtk::image_object::source_code >(source_code), this->name() );
    this->m_proj_obj_type->set_source( static_cast< vidtk::image_object::source_code >(source_code), this->name() );

    // Force config down to contained processes
    dependency_config.add_parameter( proc_diff_sp->name() + ":gui_feedback_enabled",
                                     boost::lexical_cast< std::string > ( m_gui_feedback_enabled ),
                                     "Forced override" );
    dependency_config.add_parameter( proc_diff_sp->name() + ":masking_enabled",
                                     masking_enabled,
                                     "Forced override" );
    dependency_config.add_parameter( proc_diff_sp->name() + ":run_async",
                                     run_async,
                                     "Forced override" );
    dependency_config.add_parameter( proc_diff_sp->name() + ":pixel_feature_infusion",
                                     "dual_output",
                                     "Forced override" );

    dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":run_async", run_async, "Forced override" );
    dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":run_async", run_async, "Forced override" );

    dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":conn_comp:location_type", location_type, "Forced override" );
    dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":conn_comp:location_type", location_type, "Forced override" );

    dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":gui_feedback_enabled",
                           boost::lexical_cast< std::string > ( m_gui_feedback_enabled ),
                                     "Forced override" );
    dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":gui_feedback_enabled",
                           boost::lexical_cast< std::string > ( m_gui_feedback_enabled ),
                                     "Forced override" );

    // Force crop in conn-comp to be the same as diff
    dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":crop_gsd_image:upper_left", ul, "Forced override" );
    dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":crop_gsd_image:upper_left", ul, "Forced override" );

    dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":crop_gsd_image:lower_right", lr, "Forced override" );
    dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":crop_gsd_image:lower_right", lr, "Forced override" );

    { // Reflect cropping setting into appropriate uncrop processes in children
      std::stringstream crop_ul_ss;
      crop_ul_ss << ul;
      vul_awk crop_ul_awk( crop_ul_ss );

      // conn_comp_sp::fg_image_uncrop
      dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":fg_image_uncrop:horizontal_padding",
                                       crop_ul_awk[0],
                                       "Forced override" );
      dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":fg_image_uncrop:horizontal_padding",
                                       crop_ul_awk[0],
                                       "Forced override" );

      dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":fg_image_uncrop:vertical_padding",
                                       crop_ul_awk[1],
                                       "Forced override" );
      dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":fg_image_uncrop:vertical_padding",
                                       crop_ul_awk[1],
                                       "Forced override" );

      dependency_config.add_parameter( proc_conn_comp1_sp->name() + ":fg_image_uncrop:disabled",
                                       blk.get<std::string>( proc_conn_comp1_sp->name() + ":fg_image_writer:disabled" ),
                                       "Forced override" );
      dependency_config.add_parameter( proc_conn_comp2_sp->name() + ":fg_image_uncrop:disabled",
                                       blk.get<std::string>( proc_conn_comp2_sp->name() + ":fg_image_writer:disabled" ),
                                       "Forced override" );
    }

    // This is to not even try to track if the GSD is unexpectedly large.
    // There is probably something wrong with the metadata.

    // There is an assumption that we are using the motion detector.
    // This is not always the case. We need to reevaluate what this is trying to do.
    double gsd = this->pad_source_world_units_per_pixel->value();

    if( gsd > m_max_workable_gsd )
    {
      try
      {
        dependency_config.add_parameter( proc_diff_sp->name() + ":disabled", "true",
                                         "Forced override" );

        LOG_WARN( this->name()  << ": Disabled detection and tracking because GSD value ("
                  << gsd << ") "
                  << " > workable value (" << m_max_workable_gsd << ")." );
      }
      catch ( config_block_parse_error const& e )
      {
        LOG_WARN( this->name()  << ":Could not disable detection and tracking for this shot: "
                  << e.what() );
      }
    }

    // Force these values into our config so they will be seen by the contained processes
    this->m_config.update( dependency_config );

    // Update parameters that have video name prefix.
    // Note updating is done in the returned copy of the config, not in the saved copy.
    videoname_prefix::instance()->add_videoname_prefix( this->m_config, proc_diff_sp->name()
                                                        + ":diff_fg_out:pattern" );
    videoname_prefix::instance()->add_videoname_prefix( this->m_config, proc_diff_sp->name()
                                                        + ":diff_float_out:pattern" );
    videoname_prefix::instance()->add_videoname_prefix( this->m_config, proc_diff_sp->name()
                                                        + ":diff_fg_zscore_out:pattern" );

    // ================================
    // now is the time to instantiate the pipeline
    bool opt_run_async = blk.get< bool >( "run_async" );

    if( opt_run_async )
    {
      unsigned edge_capacity = blk.get< unsigned >( "pipeline_edge_capacity" );
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
    if( ! this->pipeline()->set_params( this->m_config ) )
    {
      throw config_block_parse_error( " unable to set pipeline parameters." );
    }
  }
  catch( config_block_parse_error const& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  return true;
} // set_params


// ----------------------------------------------------------------
// Create sync or async pipeline
template< class PixType >
template< class Pipeline >
void
motion_and_saliency_detector_pipeline< PixType >
::setup_pipeline( Pipeline * p )
{
  this->m_pipeline = p; // save generic pipeline pointer

  this->setup_base_pipeline( p );

  p->add( proc_diff_sp );
  p->add( proc_conn_comp1_sp );
  p->add( proc_conn_comp2_sp );
  p->add( proc_set_obj_type_1 );
  p->add( proc_set_obj_type_2 );
  p->add( proc_set_proj_obj_type );
  p->add( proc_merge );

  // input pads to diff_sp
  connect_pad2port( timestamp,             proc_diff_sp, set_source_timestamp );
  connect_pad2port( image,                 proc_diff_sp, set_source_image );
  connect_pad2port( src_to_ref_homography, proc_diff_sp, set_src_to_ref_homography );
  connect_pad2port( wld_to_src_homography, proc_diff_sp, set_wld_to_src_homography );
  connect_pad2port( src_to_utm_homography, proc_diff_sp, set_src_to_utm_homography );
  connect_pad2port( wld_to_utm_homography, proc_diff_sp, set_wld_to_utm_homography );
  connect_pad2port( src_to_wld_homography, proc_diff_sp, set_src_to_wld_homography );
  connect_pad2port( world_units_per_pixel, proc_diff_sp, set_world_units_per_pixel );
  connect_pad2port( shot_break_flags,      proc_diff_sp, set_input_shot_break_flags );
  connect_pad2port( mask_image,            proc_diff_sp, set_mask );
  connect_pad2port( video_modality,        proc_diff_sp, set_input_video_modality );
  connect_pad2port( ref_to_wld_homography, proc_diff_sp, set_input_ref_to_wld_homography );
  connect_pad2port( gui_feedback,          proc_diff_sp, set_input_gui_feedback );

  // diff_sp to conn_comp 1
  connect_ports( proc_diff_sp, source_timestamp,                 proc_conn_comp1_sp, set_source_timestamp );
  connect_ports( proc_diff_sp, source_image,                     proc_conn_comp1_sp, set_source_image );
  connect_ports( proc_diff_sp, copied_fg_out,                    proc_conn_comp1_sp, set_fg_image );
  connect_ports( proc_diff_sp, world_units_per_pixel,            proc_conn_comp1_sp, set_world_units_per_pixel );
  connect_ports( proc_diff_sp, src_to_ref_homography,            proc_conn_comp1_sp, set_src_to_ref_homography );
  connect_ports( proc_diff_sp, src_to_wld_homography,            proc_conn_comp1_sp, set_src_to_wld_homography );
  connect_ports( proc_diff_sp, wld_to_src_homography,            proc_conn_comp1_sp, set_wld_to_src_homography );
  connect_ports( proc_diff_sp, src_to_utm_homography,            proc_conn_comp1_sp, set_src_to_utm_homography );
  connect_ports( proc_diff_sp, wld_to_utm_homography,            proc_conn_comp1_sp, set_wld_to_utm_homography );
  connect_ports( proc_diff_sp, copied_diff_out,                  proc_conn_comp1_sp, set_diff_image );
  connect_ports( proc_diff_sp, get_output_ref_to_wld_homography, proc_conn_comp1_sp, set_ref_to_wld_homography );
  connect_ports( proc_diff_sp, get_output_video_modality,        proc_conn_comp1_sp, set_video_modality );
  connect_ports( proc_diff_sp, get_output_shot_break_flags,      proc_conn_comp1_sp, set_shot_break_flags );
  connect_ports( proc_diff_sp, get_output_gui_feedback,          proc_conn_comp1_sp, set_gui_feedback );
  connect_ports( proc_diff_sp, get_output_feature_array,         proc_conn_comp1_sp, set_pixel_feature_array );

  // diff_sp to conn_comp 2
  connect_ports( proc_diff_sp, source_timestamp,                 proc_conn_comp2_sp, set_source_timestamp );
  connect_ports( proc_diff_sp, source_image,                     proc_conn_comp2_sp, set_source_image );
  connect_ports( proc_diff_sp, fg2_out,                          proc_conn_comp2_sp, set_fg_image );
  connect_ports( proc_diff_sp, world_units_per_pixel,            proc_conn_comp2_sp, set_world_units_per_pixel );
  connect_ports( proc_diff_sp, src_to_ref_homography,            proc_conn_comp2_sp, set_src_to_ref_homography );
  connect_ports( proc_diff_sp, src_to_wld_homography,            proc_conn_comp2_sp, set_src_to_wld_homography );
  connect_ports( proc_diff_sp, wld_to_src_homography,            proc_conn_comp2_sp, set_wld_to_src_homography );
  connect_ports( proc_diff_sp, src_to_utm_homography,            proc_conn_comp2_sp, set_src_to_utm_homography );
  connect_ports( proc_diff_sp, wld_to_utm_homography,            proc_conn_comp2_sp, set_wld_to_utm_homography );
  connect_ports( proc_diff_sp, copied_diff_out,                  proc_conn_comp2_sp, set_diff_image );
  connect_ports( proc_diff_sp, get_output_ref_to_wld_homography, proc_conn_comp2_sp, set_ref_to_wld_homography );
  connect_ports( proc_diff_sp, get_output_video_modality,        proc_conn_comp2_sp, set_video_modality );
  connect_ports( proc_diff_sp, get_output_shot_break_flags,      proc_conn_comp2_sp, set_shot_break_flags );
  connect_ports( proc_diff_sp, get_output_gui_feedback,          proc_conn_comp2_sp, set_gui_feedback );
  connect_ports( proc_diff_sp, get_output_feature_array,         proc_conn_comp2_sp, set_pixel_feature_array );

  // Connect to process that sets type
  connect_ports( proc_conn_comp1_sp, output_objects,           proc_set_obj_type_1,    input );
  connect_ports( proc_conn_comp2_sp, output_objects,           proc_set_obj_type_2,    input );
  connect_ports( proc_conn_comp1_sp, projected_objects,        proc_set_proj_obj_type, input );

  // merge object vectors from conn comps
  connect_ports( proc_set_obj_type_1, output,                  proc_merge,  add_vector_0 );
  connect_ports( proc_set_obj_type_2, output,                  proc_merge,  add_vector_1 );

  // conn_comp to output_pads
  connect_port2pad( proc_conn_comp1_sp, source_timestamp,      timestamp );
  connect_port2pad( proc_conn_comp1_sp, source_image,          image );
  connect_port2pad( proc_conn_comp1_sp, src_to_wld_homography, src_to_wld_homography );
  connect_port2pad( proc_conn_comp1_sp, wld_to_src_homography, wld_to_src_homography );
  connect_port2pad( proc_conn_comp1_sp, src_to_utm_homography, src_to_utm_homography );
  connect_port2pad( proc_conn_comp1_sp, wld_to_utm_homography, wld_to_utm_homography );
  connect_port2pad( proc_conn_comp1_sp, src_to_ref_homography, src_to_ref_homography );
  connect_port2pad( proc_conn_comp1_sp, fg_image,              fg_image );
  connect_port2pad( proc_conn_comp1_sp, diff_image,            diff_image );
  connect_port2pad( proc_conn_comp1_sp, morph_image,           morph_image );
  connect_port2pad( proc_conn_comp1_sp, morph_grey_image,      morph_grey_image );
  connect_port2pad( proc_conn_comp1_sp, world_units_per_pixel, world_units_per_pixel );
  connect_port2pad( proc_conn_comp1_sp, ref_to_wld_homography, ref_to_wld_homography );
  connect_port2pad( proc_conn_comp1_sp, video_modality,        video_modality );
  connect_port2pad( proc_conn_comp1_sp, shot_break_flags,      shot_break_flags );
  connect_port2pad( proc_conn_comp1_sp, gui_feedback,          gui_feedback );
  connect_port2pad( proc_conn_comp1_sp, pixel_feature_array,   pixel_features );

  connect_port2pad( proc_merge,             v_all,    image_objects );
  connect_port2pad( proc_set_proj_obj_type, output,   projected_objects );
}

} // end namespace vidtk
