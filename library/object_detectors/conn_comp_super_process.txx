/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "conn_comp_super_process.h"

#include <pipeline_framework/sync_pipeline.h>
#include <pipeline_framework/async_pipeline.h>

#include <object_detectors/connected_component_process.h>
#include <object_detectors/conn_comp_pass_thru_process.h>
#include <object_detectors/project_to_world_process.h>
#include <object_detectors/filter_image_objects_process.h>
#include <object_detectors/transform_image_object_process.h>
#include <object_detectors/homography_to_scale_image_process.h>

#include <tracking_data/io/image_object_writer_process.h>

#include <video_properties/filter_image_process.h>
#include <video_properties/extract_image_resolution_process.h>

#include <video_transforms/convert_image_process.h>
#include <video_transforms/crop_image_process.h>
#include <video_transforms/floating_point_image_hash_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/uncrop_image_process.h>
#include <video_transforms/world_morphology_process.h>

#include <video_io/image_list_writer_process.h>

#include <utilities/extract_matrix_process.h>
#include <utilities/checked_bool.h>

#include <vxl_config.h>
#include <vil/vil_convert.h>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_conn_comp_super_process_txx__
VIDTK_LOGGER("conn_comp_super_process_txx");

namespace vidtk
{


// Main operating mode types of the conn component superprocess
enum mode_type { STANDARD, HYSTERESIS, INVALID };


// Implementation of the CC pipeline
template< class PixType >
class conn_comp_super_process_impl
{
public:

  typedef typename pixel_feature_array< PixType >::sptr_t feature_array_sptr;

  // Processes
  process_smart_pointer< filter_image_process > proc_filter_image;

  process_smart_pointer< homography_to_scale_image_process< unsigned > > proc_scale_img_gen;
  process_smart_pointer< image_list_writer_process< unsigned > > proc_depth_writer;

  process_smart_pointer< extract_image_resolution_process< PixType > > proc_extract_resolution;
  process_smart_pointer< convert_image_process< float, vxl_byte > > proc_convert;
  process_smart_pointer< extract_matrix_process< image_to_plane_homography > > proc_extract_matrix;

  process_smart_pointer< world_morphology_process< bool, unsigned > > proc_morph1;
  process_smart_pointer< world_morphology_process< bool, unsigned > > proc_morph2;
  process_smart_pointer< world_morphology_process< bool, unsigned > > proc_morph3;
  process_smart_pointer< world_morphology_process< PixType, unsigned > > proc_morph_grey1;
  process_smart_pointer< world_morphology_process< PixType, unsigned > > proc_morph_grey2;

  process_smart_pointer< threshold_image_process< PixType > > proc_hysteresis_high;
  process_smart_pointer< threshold_image_process< PixType > > proc_hysteresis_low;

  process_smart_pointer< connected_component_process > proc_conn_comp;
  process_smart_pointer< conn_comp_pass_thru_process< PixType > > proc_conn_comp_pass;
  process_smart_pointer< project_to_world_process > proc_project;
  process_smart_pointer< filter_image_objects_process< PixType > > proc_filter1;

  process_smart_pointer< image_object_writer_process > proc_output_objects;
  process_smart_pointer< uncrop_image_process< bool > > proc_morph_image_uncrop;
  process_smart_pointer< image_list_writer_process< bool > > proc_morph_image_writer;

  process_smart_pointer< image_list_writer_process< bool > > proc_th_fg_writer;
  process_smart_pointer< image_list_writer_process< PixType > > proc_morph_grey_img_writer;
  process_smart_pointer< filter_image_objects_process< PixType > > proc_th_filter_objs;

  process_smart_pointer< project_to_world_process > proc_crop_correct;

  process_smart_pointer< crop_image_process< double > > proc_crop_image;
  process_smart_pointer< floating_point_image_hash_process< double, unsigned > > proc_float_pt_img_hash;

  process_smart_pointer< transform_image_object_process< PixType > > proc_add_chip;


#define decl_input_pads(name, type) \
  process_smart_pointer< super_process_pad_impl< type > > input_##name##_pad;

  conn_comp_sp_inputs(decl_input_pads)
  conn_comp_sp_optional_inputs(decl_input_pads)

#undef decl_input_pads

#define decl_output_pads(name, type) \
  process_smart_pointer< super_process_pad_impl< type > > output_##name##_pad;

  conn_comp_sp_outputs(decl_output_pads)
  conn_comp_sp_optional_outputs(decl_output_pads)

#undef decl_output_pads

  // Config parameters
  enum mode_type mode;
  bool use_variable_morphology;
  bool use_gsd_scale_image;

  config_block config;
  config_block default_config;
  config_block dependency_config;

  conn_comp_super_process_impl()
    : proc_filter_image( NULL ),
      proc_scale_img_gen( NULL ),
      proc_depth_writer( NULL ),
      proc_extract_resolution( NULL ),
      proc_convert( NULL ),
      proc_extract_matrix( NULL ),
      proc_morph1( NULL ),
      proc_morph2( NULL ),
      proc_morph3( NULL ),
      proc_morph_grey1( NULL ),
      proc_morph_grey2( NULL ),
      proc_hysteresis_high( NULL ),
      proc_hysteresis_low( NULL ),
      proc_conn_comp( NULL ),
      proc_conn_comp_pass( NULL ),
      proc_project( NULL ),
      proc_filter1( NULL ),
      proc_output_objects( NULL ),
      proc_morph_image_uncrop( NULL ),
      proc_morph_image_writer( NULL ),
      proc_th_fg_writer( NULL ),
      proc_morph_grey_img_writer( NULL ),
      proc_th_filter_objs( NULL ),
      proc_crop_correct( NULL ),
      proc_crop_image( NULL ),
      proc_float_pt_img_hash( NULL ),
      proc_add_chip(NULL),
#define init_input_pads(name, type) \
  input_##name##_pad(NULL),

      conn_comp_sp_inputs(init_input_pads)
      conn_comp_sp_optional_inputs(init_input_pads)

#undef init_input_pads

#define init_output_pads(name, type) \
  output_##name##_pad(NULL),

      conn_comp_sp_outputs(init_output_pads)
      conn_comp_sp_optional_outputs(init_output_pads)

#undef init_output_pads
      mode(INVALID),
      use_variable_morphology( false ),
      use_gsd_scale_image( false ),
      config(),
      default_config(),
      dependency_config()
  {

    config.add_parameter( "run_async",
                          "true",
                          "Whether to run async or not." );

    config.add_parameter( "pipeline_edge_capacity",
                          boost::lexical_cast< std::string >(10 / sizeof(PixType)),
                          "Maximum size of the edge in an async pipeline." );

    config.add_parameter( "hysteresis:disabled",
                          "true",
                          "Enable hysteresis thresholding." );

    config.add_parameter( "gui_feedback_enabled",
                          "false",
                          "Should we enabled GUI feedback?" );

    config.add_parameter( "use_homography_based_morphology",
                          "false",
                          "True if we want to use variable morphology based upon "
                          "regional GSDs." );

    config.add_parameter( "variable_morphology_resolution",
                          "2",
                          "When performing variable homography based on a GSD "
                          "image or a homography, the sub-pixel resolution required "
                          "for sizing morphology kernels. Unit is the number of sub-pixel "
                          "bins desired. See homography to scale image documentation." );

    config.add_parameter( "use_gsd_scale_image",
                          "false",
                          "True if we want to use scaled image based upon "
                          "regional GSDs." );
  }

  void create_process_configs()
  {
#define create_input_pads(name, type) \
  input_##name##_pad = new super_process_pad_impl<type>("input_" #name);

    conn_comp_sp_inputs(create_input_pads)
    conn_comp_sp_optional_inputs(create_input_pads)

#undef create_input_pads

#define create_output_pads(name, type) \
  output_##name##_pad = new super_process_pad_impl<type>("output_" #name);

    conn_comp_sp_outputs(create_output_pads)
    conn_comp_sp_optional_outputs(create_output_pads)

#undef create_output_pads

    proc_filter_image = new filter_image_process( "filter_image" );
    config.add_subblock( proc_filter_image->params(), proc_filter_image->name() );

    proc_crop_correct = new project_to_world_process( "correct_for_cropping" );
    config.add_subblock( proc_crop_correct->params(), proc_crop_correct->name() );

    proc_morph1 = new world_morphology_process< bool >( "morph1" );
    config.add_subblock( proc_morph1->params(), proc_morph1->name() );

    proc_morph2  = new world_morphology_process< bool >( "morph2" );
    config.add_subblock( proc_morph2->params(), proc_morph2->name() );

    proc_morph3  = new world_morphology_process< bool >( "morph3" );
    config.add_subblock( proc_morph3->params(), proc_morph3->name() );

    proc_scale_img_gen = new homography_to_scale_image_process< unsigned >( "gsd_image_gen" );
    config.add_subblock( proc_scale_img_gen->params(), proc_scale_img_gen->name() );

    proc_morph_grey1 = new world_morphology_process< PixType >( "morph_grey1" );
    config.add_subblock( proc_morph_grey1->params(), proc_morph_grey1->name() );

    proc_morph_grey2  = new world_morphology_process< PixType >( "morph_grey2" );
    config.add_subblock( proc_morph_grey2->params(), proc_morph_grey2->name() );

    proc_hysteresis_high  = new threshold_image_process< PixType >( "hysteresis_high" );
    config.add_subblock( proc_hysteresis_high->params(), proc_hysteresis_high->name() );

    proc_hysteresis_low  = new threshold_image_process< PixType >( "hysteresis_low" );
    config.add_subblock( proc_hysteresis_low->params(), proc_hysteresis_low->name() );

    proc_conn_comp = new connected_component_process( "conn_comp" );
    config.add_subblock( proc_conn_comp->params(), proc_conn_comp->name() );

    proc_conn_comp_pass = new conn_comp_pass_thru_process< PixType >( "conn_comp_pass" );
    config.add_subblock( proc_conn_comp_pass->params(), proc_conn_comp_pass->name() );

    proc_project = new project_to_world_process( "project" );
    config.add_subblock( proc_project->params(), proc_project->name() );

    proc_filter1 = new filter_image_objects_process< PixType >( "filter1" );
    config.add_subblock( proc_filter1->params(), proc_filter1->name() );

    proc_output_objects = new image_object_writer_process( "output_objects" );
    config.add_subblock( proc_output_objects->params(), proc_output_objects->name() );

    proc_morph_image_uncrop = new uncrop_image_process<bool>( "fg_image_uncrop" );
    config.add_subblock( proc_morph_image_uncrop->params(), proc_morph_image_uncrop->name() );

    proc_morph_image_writer = new image_list_writer_process<bool>( "fg_image_writer" );
    config.add_subblock( proc_morph_image_writer->params(), proc_morph_image_writer->name() );

    proc_th_fg_writer = new image_list_writer_process<bool>( "th_fg_writer" );
    config.add_subblock( proc_th_fg_writer->params(), proc_th_fg_writer->name() );

    proc_depth_writer = new image_list_writer_process<unsigned>( "depth_img_writer" );
    config.add_subblock( proc_depth_writer->params(), proc_depth_writer->name() );

    proc_extract_resolution = new extract_image_resolution_process<PixType>( "extract_resolution" );
    config.add_subblock( proc_extract_resolution->params(),
                         proc_extract_resolution->name() );

    proc_convert = new convert_image_process<PixType, vxl_byte>( "convert_image" );
    config.add_subblock( proc_convert->params(),
                         proc_convert->name() );

    proc_extract_matrix = new extract_matrix_process<image_to_plane_homography>( "extract_matrix" );
    config.add_subblock( proc_extract_matrix->params(),
                         proc_extract_matrix->name() );

    proc_morph_grey_img_writer = new image_list_writer_process<vxl_byte>( "morph_grey_img_writer" );
    config.add_subblock( proc_morph_grey_img_writer->params(),
                         proc_morph_grey_img_writer->name() );

    proc_th_filter_objs = new filter_image_objects_process< PixType >( "th_filter_objs" );
    config.add_subblock( proc_th_filter_objs->params(), proc_th_filter_objs->name() );

    proc_crop_image = new crop_image_process<double>( "crop_gsd_image" );
    config.add_subblock( proc_crop_image->params(), proc_crop_image->name() );

    proc_float_pt_img_hash = new floating_point_image_hash_process< double, unsigned >( "float_pt_img_hash" );
    config.add_subblock( proc_float_pt_img_hash->params(), proc_float_pt_img_hash->name() );

    proc_add_chip = new transform_image_object_process< PixType >( "add_image_chip",
                                                                   new add_image_clip_to_image_object< PixType >() );
    config.add_subblock( proc_add_chip->params(), proc_add_chip->name() );

    // Add superprocess subprocess defaults
    default_config.add_parameter( proc_filter_image->name() + ":disabled", "false", "Default override" );
    default_config.add_parameter( proc_th_fg_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_depth_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_morph_image_uncrop->name() + ":disabled", "true",
        "Uncropping not performed when disabled." );
    default_config.add_parameter( proc_morph_image_uncrop->name() + ":background_value", "false",
        "Value to fill in new area generated by the uncropping process. Fills "
        "in area with false values by default." );
    default_config.add_parameter( proc_morph_image_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_morph_image_writer->name() + ":skip_unset_images", "true", "Default override" );
    default_config.add_parameter( proc_morph_grey_img_writer->name() + ":disabled", "true", "Default override" );

    // Add superprocess subprocess default settings which may vary based on the mode
    dependency_config.add_parameter( proc_th_filter_objs->name() + ":filter_binary_image", "false", "Force override" );
    dependency_config.add_parameter( proc_scale_img_gen->name() + ":mode", "GSD_IMAGE", "Force override" );
    dependency_config.add_parameter( proc_morph1->name() + ":mode", "CONSTANT_GSD", "Force override" );
    dependency_config.add_parameter( proc_morph2->name() + ":mode", "CONSTANT_GSD", "Force override" );
    dependency_config.add_parameter( proc_morph3->name() + ":mode", "CONSTANT_GSD", "Force override" );
    dependency_config.add_parameter( proc_morph_grey1->name() + ":mode", "CONSTANT_GSD", "Force override" );
    dependency_config.add_parameter( proc_morph_grey2->name() + ":mode", "CONSTANT_GSD", "Force override" );
    dependency_config.add_parameter( proc_scale_img_gen->name() + ":hashed_max_gsd", "0", "Force override" );
    dependency_config.add_parameter( proc_scale_img_gen->name() + ":hashed_gsd_scale_factor", "0", "Force override");
    dependency_config.add_parameter( proc_morph1->name() + ":hashed_gsd_scale_factor", "0", "Force override" );
    dependency_config.add_parameter( proc_morph2->name() + ":hashed_gsd_scale_factor", "0", "Force override" );
    dependency_config.add_parameter( proc_morph3->name() + ":hashed_gsd_scale_factor", "0", "Force override" );
    dependency_config.add_parameter( proc_morph_grey1->name() + ":hashed_gsd_scale_factor", "0", "Force override" );
    dependency_config.add_parameter( proc_morph_grey2->name() + ":hashed_gsd_scale_factor", "0", "Force override" );
    dependency_config.add_parameter( proc_float_pt_img_hash->name() + ":max_input_value", "0", "Force override");
    dependency_config.add_parameter( proc_float_pt_img_hash->name() + ":scale_factor", "0", "Force override");

    std::string loc_type = config.get<std::string>(  proc_conn_comp->name() + ":location_type" );
    dependency_config.add_parameter( proc_project->name() + ":location_type", loc_type, "Force override");

    config.update( default_config );
  }


  // ----------------------------------------------------------------
  template <typename Pipeline>
  void setup_pipeline( Pipeline* p )
  {

#define add_input_pads(name, type) \
  p->add( input_##name##_pad );

    conn_comp_sp_inputs(add_input_pads)

#undef add_input_pads

    p->add( proc_conn_comp_pass );

#define add_output_pads(name, type) \
  p->add( output_##name##_pad );

    conn_comp_sp_outputs(add_output_pads)

#undef add_output_pads

#define connect_pass_thru_input(name, type)     \
  p->connect( input_##name##_pad->value_port(), \
              proc_conn_comp_pass->set_input_##name##_port() );

    conn_comp_sp_pass_thrus(connect_pass_thru_input)

#undef connect_pass_thru_input

    p->add( proc_filter_image);

    p->add( proc_conn_comp );
    p->connect( input_world_units_per_pixel_pad->value_port(),   proc_conn_comp->set_world_units_per_pixel_port() );
    p->connect( input_diff_image_pad->value_port(),              proc_conn_comp->set_heatmap_image_port() );
    p->connect( proc_filter_image->isgood_port(),                proc_conn_comp->set_is_fg_good_port() );

    p->add( proc_crop_correct );
    p->add( proc_project );

    // Add scale image generator if either option is set
    if( use_variable_morphology || use_gsd_scale_image )
    {
      p->add( proc_scale_img_gen );
      p->connect( proc_scale_img_gen->gsd_image_port(),   proc_project->set_gsd_image_port() );
    }

    switch( this->mode)
    {

      case STANDARD:
      {
        p->connect( input_fg_image_pad->value_port(),              proc_filter_image->set_source_image_port() );
        p->connect( input_fg_image_pad->value_port(),              proc_conn_comp_pass->set_input_fg_image_port() );

        p->add( proc_morph1 );
        p->connect( input_world_units_per_pixel_pad->value_port(), proc_morph1->set_world_units_per_pixel_port() );
        p->connect( input_fg_image_pad->value_port(),              proc_morph1->set_source_image_port() );
        p->connect( proc_filter_image->isgood_port(),              proc_morph1->set_is_fg_good_port() );

        p->add( proc_morph2 );
        p->connect( input_world_units_per_pixel_pad->value_port(), proc_morph2->set_world_units_per_pixel_port() );
        p->connect( proc_morph1->copied_image_port(),              proc_morph2->set_source_image_port() );

        p->add( proc_morph3 );
        p->connect( input_world_units_per_pixel_pad->value_port(), proc_morph3->set_world_units_per_pixel_port() );
        p->connect( proc_morph2->copied_image_port(),              proc_morph3->set_source_image_port() );
        p->connect( proc_morph3->copied_image_port(),              proc_conn_comp->set_fg_image_port() );

        if( use_variable_morphology || use_gsd_scale_image )
        {
          p->add( proc_extract_resolution );
          p->connect( input_source_image_pad->value_port(),          proc_extract_resolution->set_image_port() );
          p->connect( proc_extract_resolution->resolution_port(),    proc_scale_img_gen->set_resolution_port() );
          p->connect( input_src_to_wld_homography_pad->value_port(), proc_scale_img_gen->set_image_to_world_homography_port() );
          p->connect( proc_filter_image->isgood_port(),              proc_scale_img_gen->set_is_valid_flag_port() );
        }

        if( use_variable_morphology )
        {
          p->add( proc_float_pt_img_hash );
          p->add( proc_crop_image );
          p->connect( proc_scale_img_gen->copied_gsd_image_port(),  proc_crop_image->set_source_image_port() );
          p->connect( proc_crop_image->cropped_image_port(),        proc_float_pt_img_hash->set_input_image_port() );
          p->connect( proc_float_pt_img_hash->hashed_image_port(),  proc_morph1->set_hashed_gsd_image_port() );
          p->connect( proc_float_pt_img_hash->hashed_image_port(),  proc_morph2->set_hashed_gsd_image_port() );
          p->connect( proc_float_pt_img_hash->hashed_image_port(),  proc_morph3->set_hashed_gsd_image_port() );

          bool depth_writer_disabled = config.get<bool>(proc_depth_writer->name() + ":disabled");
          if (!depth_writer_disabled)
          {
            p->add( proc_depth_writer );
            p->connect( input_source_timestamp_pad->value_port(),    proc_depth_writer->set_timestamp_port() );
            p->connect( proc_scale_img_gen->copied_hashed_gsd_image_port(), proc_depth_writer->set_image_port() );
          }
        }

        bool morph_image_writer_disabled = config.get<bool>(proc_morph_image_writer->name() + ":disabled");
        if (!morph_image_writer_disabled)
        {
          p->add( proc_morph_image_writer );

          p->connect( input_source_timestamp_pad->value_port(), proc_morph_image_writer->set_timestamp_port() );

          bool morph_image_uncrop_disabled = config.get<bool>(proc_morph_image_uncrop->name() + ":disabled");

          if (!morph_image_uncrop_disabled)
          {
            p->add( proc_morph_image_uncrop );
            p->connect( proc_morph3->copied_image_port(),                proc_morph_image_uncrop->set_source_image_port() );
            p->connect( proc_morph_image_uncrop->uncropped_image_port(), proc_morph_image_writer->set_image_port() );
          }
          else
          {
            p->connect( proc_morph3->copied_image_port(), proc_morph_image_writer->set_image_port() );
          }
        }

        p->connect( proc_conn_comp->objects_port(),   proc_crop_correct->set_source_objects_port() );
        p->connect( proc_morph3->copied_image_port(), proc_conn_comp_pass->set_input_morph_image_port() );
        break;

      } // STANDARD

      case HYSTERESIS:
      {

        p->add( proc_convert );
        p->connect( input_diff_image_pad->value_port(), proc_convert->set_image_port() );

        p->add( proc_morph_grey1 );
        p->connect( proc_convert->converted_image_port(),          proc_morph_grey1->set_source_image_port() );
        p->connect( input_world_units_per_pixel_pad->value_port(), proc_morph_grey1->set_world_units_per_pixel_port() );

        p->add( proc_morph_grey2 );
        p->connect( input_world_units_per_pixel_pad->value_port(), proc_morph_grey2->set_world_units_per_pixel_port() );
        p->connect( proc_morph_grey1->copied_image_port(),         proc_morph_grey2->set_source_image_port() );

        bool morph_grey_image_writer_disabled = config.get<bool>(proc_morph_grey_img_writer->name() + ":disabled");
        if (!morph_grey_image_writer_disabled)
        {
          p->add( proc_morph_grey_img_writer );
          p->connect( input_source_timestamp_pad->value_port(), proc_morph_grey_img_writer->set_timestamp_port() );
          p->connect( proc_morph_grey2->copied_image_port(),    proc_morph_grey_img_writer->set_image_port() );
        }

        if( use_variable_morphology )
        {
          p->add( proc_float_pt_img_hash );
          p->add( proc_crop_image );
          p->connect( proc_scale_img_gen->copied_gsd_image_port(), proc_crop_image->set_source_image_port());
          p->connect( proc_crop_image->cropped_image_port(),       proc_float_pt_img_hash->set_input_image_port() );
          p->connect( proc_float_pt_img_hash->hashed_image_port(), proc_morph_grey1->set_hashed_gsd_image_port() );
          p->connect( proc_float_pt_img_hash->hashed_image_port(), proc_morph_grey2->set_hashed_gsd_image_port() );
        }

        p->add( proc_hysteresis_low );
        p->connect( proc_morph_grey2->copied_image_port(),         proc_hysteresis_low->set_source_image_port() );
        p->connect( proc_hysteresis_low->thresholded_image_port(), proc_filter_image->set_source_image_port() );
        p->connect( proc_hysteresis_low->thresholded_image_port(), proc_conn_comp->set_fg_image_port() );
        p->connect( proc_hysteresis_low->thresholded_image_port(), proc_conn_comp_pass->set_input_fg_image_port() );

        // TODO: proc_th_fg_writer should be replaced by proc_morph_image_uncrop --> proc_morph_image_writer
        bool th_fg_writer_disabled = config.get<bool>(proc_th_fg_writer->name() + ":disabled");
        if (!th_fg_writer_disabled)
        {
          p->add( proc_th_fg_writer );
          p->connect( input_source_timestamp_pad->value_port(),      proc_th_fg_writer->set_timestamp_port() );
          p->connect( proc_hysteresis_low->thresholded_image_port(), proc_th_fg_writer->set_image_port() );
        }

        p->add( proc_hysteresis_high );
        p->connect( proc_morph_grey2->copied_image_port(), proc_hysteresis_high->set_source_image_port() );

        // proc_th_filter_objs *merges* the output of high and low thresholding
        // For each detection above the low threshold, we will check if there is support of
        // at least one pixel above the high threshold.
        p->add( proc_th_filter_objs );
        p->connect( proc_hysteresis_high->thresholded_image_port(), proc_th_filter_objs->set_binary_image_port() );
        p->connect( proc_conn_comp->objects_port(),                 proc_th_filter_objs->set_source_objects_port() );
        p->connect( input_shot_break_flags_pad->value_port(),       proc_th_filter_objs->set_shot_break_flags_port() );
        p->connect( proc_th_filter_objs->objects_port(),            proc_crop_correct->set_source_objects_port() );
        p->connect( proc_morph_grey2->copied_image_port(),          proc_conn_comp_pass->set_input_morph_grey_image_port() );

        break;
      } // HYSTERESIS

      case INVALID:
        LOG_ERROR("MODE WAS NOT SET");
        break;

      default:
        //by code paths this should never happen, but just in case
        LOG_ERROR("Invalid enum value, expecting Hysteresis or Standard");

    } // switch(mode)


    p->add( proc_extract_matrix );
    p->connect( input_src_to_wld_homography_pad->value_port(),  proc_extract_matrix->set_homography_port() );

    p->connect( proc_extract_matrix->matrix_port(),   proc_project->set_image_to_world_homography_port() );
    p->connect( proc_crop_correct->objects_port(),    proc_project->set_source_objects_port() );

    p->add( proc_filter1 );
    p->connect( input_source_image_pad->value_port(),     proc_filter1->set_source_image_port() );
    p->connect( proc_project->objects_port(),             proc_filter1->set_source_objects_port() );
    p->connect( input_shot_break_flags_pad->value_port(), proc_filter1->set_shot_break_flags_port() );

    p->add( proc_add_chip );
    p->connect( input_source_image_pad->value_port(), proc_add_chip->set_image_port() );
    p->connect( proc_filter1->objects_port(),         proc_add_chip->set_objects_port() );

    p->connect( proc_add_chip->objects_port(),     proc_conn_comp_pass->set_input_output_objects_port() );
    p->connect( proc_project->objects_port(),      proc_conn_comp_pass->set_input_projected_objects_port() );

    p->connect( input_source_timestamp_pad->value_port(),      proc_conn_comp_pass->set_input_source_timestamp_port() );
    p->connect( input_source_image_pad->value_port(),          proc_conn_comp_pass->set_input_source_image_port() );
    p->connect( input_src_to_ref_homography_pad->value_port(), proc_conn_comp_pass->set_input_src_to_ref_homography_port() );
    p->connect( input_src_to_wld_homography_pad->value_port(), proc_conn_comp_pass->set_input_src_to_wld_homography_port() );
    p->connect( input_wld_to_src_homography_pad->value_port(), proc_conn_comp_pass->set_input_wld_to_src_homography_port() );
    p->connect( input_src_to_utm_homography_pad->value_port(), proc_conn_comp_pass->set_input_src_to_utm_homography_port() );
    p->connect( input_world_units_per_pixel_pad->value_port(), proc_conn_comp_pass->set_input_world_units_per_pixel_port() );
    p->connect( input_diff_image_pad->value_port(),            proc_conn_comp_pass->set_input_diff_image_port() );

    // Only pass gui feedback related information if required
    if( config.get<bool>("gui_feedback_enabled") )
    {
      p->add( input_gui_feedback_pad );
      p->connect( input_gui_feedback_pad->value_port(),     proc_conn_comp_pass->set_input_gui_feedback_port() );

      p->add( output_gui_feedback_pad );
      p->connect( proc_conn_comp_pass->get_output_gui_feedback_port(),  output_gui_feedback_pad->set_value_port() );

      p->add( input_pixel_feature_array_pad );
      p->connect( input_pixel_feature_array_pad->value_port(), proc_conn_comp_pass->set_input_pixel_feature_array_port() );

      p->add( output_pixel_feature_array_pad );
      p->connect( proc_conn_comp_pass->get_output_pixel_feature_array_port(), output_pixel_feature_array_pad->set_value_port() );
    }

#define connect_output_pads(name, type)                        \
  p->connect( proc_conn_comp_pass->get_output_##name##_port(), \
              output_##name##_pad->set_value_port() );

    conn_comp_sp_outputs(connect_output_pads)

#undef connect_output_pads

  }
}; // conn_comp_super_process_impl


template< class PixType >
conn_comp_super_process<PixType>
::conn_comp_super_process( std::string const& _name )
  : super_process( _name, "conn_comp_super_process" ),
    impl_( new conn_comp_super_process_impl<PixType> )
{
  impl_->create_process_configs();
}

template< class PixType >
conn_comp_super_process<PixType>
::~conn_comp_super_process()
{
  delete impl_;
}


template< class PixType >
config_block
conn_comp_super_process<PixType>
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  tmp_config.remove( impl_->dependency_config );
  return tmp_config;
}


template< class PixType >
bool
conn_comp_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    // Get settings relating to operating mode
    double var_morphology_resolution;

    impl_->config.update( blk );
    bool hysteresis_disabled = impl_->config.template get<bool>( "hysteresis:disabled" );
    if( !hysteresis_disabled )
    {
      impl_->mode = HYSTERESIS;

      if ( vil_pixel_format_of( PixType() ) != VIL_PIXEL_FORMAT_BYTE )
      {
        LOG_WARN( this->name() << ": Hysteresis mode is suspected to only work "
            "for vxl_byte at the moment. Please inspect conn_comp_super_process "
            "before usage." );
      }

      //Ensure th_filter_objs has binary filtering set
      impl_->dependency_config.set( impl_->proc_th_filter_objs->name()
                                    + ":filter_binary_image", "true" );
    }
    else
    {
      impl_->mode = STANDARD;
    }

    // Should we perform variable morphology based on the provided world homography?
    impl_->use_variable_morphology = impl_->config.template get<bool>( "use_homography_based_morphology" );

    if( impl_->use_variable_morphology )
    {
      var_morphology_resolution = impl_->config.template get<double>( "variable_morphology_resolution" );
      optimize_variable_morphology_system( var_morphology_resolution );
    }

    impl_->use_gsd_scale_image = impl_->config.template get<bool>( "use_gsd_scale_image" );

    bool run_async = impl_->config.template get<bool>( "run_async" );

    if( run_async )
    {
      unsigned edge_capacity = blk.get<unsigned>( "pipeline_edge_capacity" );
      async_pipeline * p = new async_pipeline(async_pipeline::ENABLE_STATUS_FORWARDING,
                                              edge_capacity);
      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else
    {
      sync_pipeline* p =  new sync_pipeline;
      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    impl_->config.update( impl_->dependency_config );

    if( ! pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error("Pipeline could not set parameters");
    }

  }
  catch( config_block_parse_error const & e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );
    return false;
  }

  return true;
}


template< class PixType >
bool
conn_comp_super_process<PixType>
::initialize()
{
  return pipeline_->initialize();
}


// Function which configures the most efficient communication settings
// between the process which generates a hashed per-pixel GSD image,
// and the twin variable morphology operators which utlize the depth map.
// Optimizes memory usage based upon the settings of the morphology processes
// which will be used (depends on if in regular/hysteresis mode).
template< class PixType >
void
conn_comp_super_process<PixType>
::optimize_variable_morphology_system( double const& subpix_resolution )
{
  double optimal_scale_factor = 0, max_radii_world_units = 0;

  // First retrieve the properties relating to the morphology operators
  double radii_world_units[4], max_radii_pixels[4];
  if( impl_->mode == STANDARD )
  {
    // Get for "morph" series of processes
    radii_world_units[0] = impl_->config.template get<double>( impl_->proc_morph1->name() + ":opening_radius" );
    radii_world_units[1] = impl_->config.template get<double>( impl_->proc_morph1->name() + ":closing_radius" );
    radii_world_units[2] = impl_->config.template get<double>( impl_->proc_morph2->name() + ":opening_radius" );
    radii_world_units[3] = impl_->config.template get<double>( impl_->proc_morph2->name() + ":closing_radius" );
    max_radii_pixels[0] = impl_->config.template get<double>( impl_->proc_morph1->name() + ":max_image_opening_radius" );
    max_radii_pixels[1] = impl_->config.template get<double>( impl_->proc_morph1->name() + ":max_image_closing_radius" );
    max_radii_pixels[2] = impl_->config.template get<double>( impl_->proc_morph2->name() + ":max_image_opening_radius" );
    max_radii_pixels[3] = impl_->config.template get<double>( impl_->proc_morph2->name() + ":max_image_closing_radius" );
  }
  else
  {
    // Get for "grey" series of processes
    radii_world_units[0] = impl_->config.template get<double>( impl_->proc_morph_grey1->name() + ":opening_radius" );
    radii_world_units[1] = impl_->config.template get<double>( impl_->proc_morph_grey1->name() + ":closing_radius" );
    radii_world_units[2] = impl_->config.template get<double>( impl_->proc_morph_grey2->name() + ":opening_radius" );
    radii_world_units[3] = impl_->config.template get<double>( impl_->proc_morph_grey2->name() + ":closing_radius" );
    max_radii_pixels[0] = impl_->config.template get<double>( impl_->proc_morph_grey1->name() + ":max_image_opening_radius" );
    max_radii_pixels[1] = impl_->config.template get<double>( impl_->proc_morph_grey1->name() + ":max_image_closing_radius" );
    max_radii_pixels[2] = impl_->config.template get<double>( impl_->proc_morph_grey2->name() + ":max_image_opening_radius" );
    max_radii_pixels[3] = impl_->config.template get<double>( impl_->proc_morph_grey2->name() + ":max_image_closing_radius" );
  }

  // Calculate the optimal hashed GSD image scale factor and max GSD required in the hash
  for( unsigned i = 0; i < 4; i++ )
  {
    if( radii_world_units[i] > 0 )
    {
      if( radii_world_units[i] > max_radii_world_units )
      {
        max_radii_world_units = radii_world_units[i];
      }

      double resolution_req = 1;

      if( max_radii_pixels[i] > 1 )
      {
        resolution_req = ( max_radii_pixels[i]*max_radii_pixels[i] - max_radii_pixels[i] ) *
                         ( subpix_resolution / radii_world_units[i] );
      }

      if( resolution_req > optimal_scale_factor )
      {
        optimal_scale_factor = resolution_req;
      }
    }
  }

  // Make sure the possible range of the hashed image isn't too large, such that we will not
  // use an excess amount of memory in our optimized schema. This should never be an issue
  // unless the 2 closing/opening radii are different by several orders of magnitudes.
  if( optimal_scale_factor * max_radii_world_units > 1e5 )
  {
    LOG_ERROR( this->name() << ": could not meet resolution requirement for optimized "
                               "variable morphology system, truncating." );
    optimal_scale_factor = 1e5 / max_radii_world_units;
  }

  std::string hash_factor = boost::lexical_cast< std::string >( optimal_scale_factor );
  std::string max_gsd_required = boost::lexical_cast< std::string >( max_radii_world_units );

  // Force morphology processes and scale image generator to communicate using hashed
  // GSD images for efficiency with the same scale factor (see world_morphology class documentation)
  impl_->dependency_config.set( impl_->proc_scale_img_gen->name() + ":mode", "GSD_IMAGE" );
  impl_->dependency_config.set( impl_->proc_scale_img_gen->name() + ":hashed_max_gsd", max_gsd_required );
  impl_->dependency_config.set( impl_->proc_scale_img_gen->name() + ":hashed_gsd_scale_factor", hash_factor );

  impl_->dependency_config.set( impl_->proc_float_pt_img_hash->name() + ":max_input_value", max_gsd_required );
  impl_->dependency_config.set( impl_->proc_float_pt_img_hash->name() + ":scale_factor", hash_factor);

  if( impl_->mode == STANDARD )
  {
    impl_->dependency_config.set( impl_->proc_morph1->name() + ":mode", "HASHED_GSD_IMAGE" );
    impl_->dependency_config.set( impl_->proc_morph2->name() + ":mode", "HASHED_GSD_IMAGE" );
    impl_->dependency_config.set( impl_->proc_morph1->name() + ":hashed_gsd_scale_factor", hash_factor );
    impl_->dependency_config.set( impl_->proc_morph2->name() + ":hashed_gsd_scale_factor", hash_factor );
  }
  else
  {
    impl_->dependency_config.set( impl_->proc_morph_grey1->name() + ":mode", "HASHED_GSD_IMAGE" );
    impl_->dependency_config.set( impl_->proc_morph_grey2->name() + ":mode", "HASHED_GSD_IMAGE" );
    impl_->dependency_config.set( impl_->proc_morph_grey1->name() + ":hashed_gsd_scale_factor", hash_factor );
    impl_->dependency_config.set( impl_->proc_morph_grey2->name() + ":hashed_gsd_scale_factor", hash_factor );
  }
}


// ----------------------------------------------------------------
/** Main process loop
 *
 *
 */
template< class PixType >
process::step_status
conn_comp_super_process<PixType>
::step2()
{
  step_status status =  pipeline_->execute();

  return status;
}


// ----------------------------------------------------------------
// -- INPUTS --
#define port_input_pads(name, type)              \
  template <class PixType>                       \
  void                                           \
  conn_comp_super_process<PixType>               \
  ::set_##name(type const& value)                \
  {                                              \
    impl_->input_##name##_pad->set_value(value); \
  }

  conn_comp_sp_inputs(port_input_pads)
  conn_comp_sp_optional_inputs(port_input_pads)

#undef port_input_pads

// ----------------------------------------------------------------
// -- OUTPUTS --
#define port_output_pads(name, type)            \
  template <class PixType>                      \
  type                                          \
  conn_comp_super_process<PixType>              \
  ::name() const                                \
  {                                             \
    return impl_->output_##name##_pad->value(); \
  }

  conn_comp_sp_outputs(port_output_pads)
  conn_comp_sp_optional_outputs(port_output_pads)

#undef port_output_pads

} // end namespace vidtk
