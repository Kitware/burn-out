/*ckwg +5
 * Copyright 2011-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "metadata_mask_super_process.h"

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <utilities/config_block.h>
#include <utilities/videoname_prefix.h>

#include <video_transforms/greyscale_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/mask_image_process.h>
#include <video_transforms/mask_merge_process.h>
#include <video_transforms/mask_select_process.h>
#include <video_transforms/mask_overlay_process.h>
#include <video_transforms/convert_image_process.h>

#include <object_detectors/simple_burnin_filter_process.h>
#include <object_detectors/moving_burnin_detector_process.h>
#include <object_detectors/pixel_feature_extractor_super_process.h>
#include <object_detectors/scene_obstruction_detector_process.h>
#include <object_detectors/osd_recognizer_process.h>
#include <object_detectors/osd_mask_refinement_process.h>
#include <object_detectors/metadata_text_parser_process.h>

#ifdef USE_CAFFE
#include <object_detectors/cnn_detector_process.h>
#endif

#include <video_io/mask_reader_process.h>
#include <video_io/image_list_writer_process.h>

#include <video_properties/border_detection_process.h>

#include <logger/logger.h>

#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_metadata_mask_super_process_txx__
VIDTK_LOGGER( "metadata_mask_super_process_txx" );


namespace vidtk
{

template < class PixType >
class metadata_mask_super_process_impl
{
public:
  typedef vidtk::super_process_pad_impl< vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl< timestamp > timestamp_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > mask_pad;
  typedef vidtk::super_process_pad_impl< image_border > border_pad;
  typedef vidtk::super_process_pad_impl< video_metadata::vector_t > metadata_pad;
  typedef vidtk::super_process_pad_impl< std::string > detected_type_pad;

  // Pipeline processes for the manual mask approach
  process_smart_pointer< mask_reader_process > proc_mask_reader;
  process_smart_pointer< mask_select_process > proc_mask_select;

  // Pipeline processes for the pixel classifier approach
  process_smart_pointer< greyscale_process< PixType > > proc_greyscale;
  process_smart_pointer< pixel_feature_extractor_super_process< PixType, vxl_byte > > proc_features;
  process_smart_pointer< scene_obstruction_detector_process< PixType > > proc_mask_detector;
  process_smart_pointer< osd_recognizer_process< PixType > > proc_mask_recognizer;
  process_smart_pointer< osd_mask_refinement_process< PixType > > proc_mask_refiner1;
  process_smart_pointer< osd_mask_refinement_process< PixType > > proc_mask_refiner2;

  // Pipeline processes for the cnn classifier approach
#ifdef USE_CAFFE
  process_smart_pointer< cnn_detector_process< PixType > > proc_cnn_detector;
  process_smart_pointer< convert_image_process< float, double > > proc_converter;
#endif

  // Pipeline processes for secondary template matching
  process_smart_pointer< simple_burnin_filter_process< PixType > > proc_mdb_filter;
  process_smart_pointer< moving_burnin_detector_process > proc_burnin_detect1;
  process_smart_pointer< moving_burnin_detector_process > proc_burnin_detect2;
  process_smart_pointer< metadata_text_parser_process< PixType > > proc_text_parser;

  // Pipeline processes shared across both approaches
  process_smart_pointer< border_detection_process< PixType > > proc_border_det;
  process_smart_pointer< mask_merge_process > proc_mask_merge1;
  process_smart_pointer< mask_merge_process > proc_mask_merge2;

  // Pipeline processes for displaying masks and intermediary outputs
  process_smart_pointer< mask_overlay_process > proc_mask_overlay;
  process_smart_pointer< image_list_writer_process< PixType > > proc_overlay_writer;
  process_smart_pointer< image_list_writer_process< PixType > > proc_filter_writer;
  process_smart_pointer< image_list_writer_process< bool > > proc_mask_writer;

  // Input pads (dummy processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< metadata_pad > pad_source_metadata;

  // Output pads (dummy processes)
  process_smart_pointer< image_pad > pad_output_image;
  process_smart_pointer< mask_pad > pad_output_mask;
  process_smart_pointer< mask_pad > pad_output_mask_no_border;
  process_smart_pointer< border_pad > pad_output_border;
  process_smart_pointer< timestamp_pad > pad_output_timestamp;
  process_smart_pointer< metadata_pad > pad_output_metadata;
  process_smart_pointer< detected_type_pad > pad_output_type;

  // Method for detecting obstructions
  enum detection_mode_t {
    MANUAL_MASK,
    PIXEL_CLASS,
    CNN_CLASS,
    BORDER_DET_ONLY
  } detection_mode;

  // Configuration parameters
  config_block config;
  config_block default_config;
  bool allow_text_parsing;
  bool allow_template_matching;
  bool use_border_in_osd_detection;
  unsigned default_edge_capacity;
  unsigned refiner_edge_capacity;

  // Default constructor
  metadata_mask_super_process_impl()
  : proc_mask_reader( NULL ),
    proc_mask_select( NULL ),
    proc_greyscale( NULL ),
    proc_features( NULL ),
    proc_mask_detector( NULL ),
    proc_mask_recognizer( NULL ),
    proc_mdb_filter( NULL ),
    proc_burnin_detect1( NULL ),
    proc_burnin_detect2( NULL ),
    proc_text_parser( NULL ),
    proc_border_det( NULL ),
    proc_mask_merge1( NULL ),
    proc_mask_merge2( NULL ),
    proc_mask_overlay( NULL ),
    proc_overlay_writer( NULL ),
    proc_filter_writer( NULL ),
    proc_mask_writer( NULL ),
    pad_source_image( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_metadata( NULL ),
    pad_output_image( NULL ),
    pad_output_mask( NULL ),
    pad_output_mask_no_border( NULL ),
    pad_output_border( NULL ),
    pad_output_timestamp( NULL ),
    pad_output_metadata( NULL ),
    detection_mode( MANUAL_MASK ),
    config(),
    default_config(),
    allow_text_parsing( false ),
    allow_template_matching( false ),
    use_border_in_osd_detection( true ),
    default_edge_capacity( 10 ),
    refiner_edge_capacity( 10 )
  {
  }

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    pad_source_image = new image_pad( "source_image" );
    pad_source_timestamp = new timestamp_pad( "source_timestamp" );
    pad_source_metadata = new metadata_pad( "source_metadata" );

    pad_output_image = new image_pad( "output_image" );
    pad_output_mask = new mask_pad( "output_mask" );
    pad_output_mask_no_border = new mask_pad( "output_borderless_mask" );
    pad_output_border = new border_pad( "output_border" );
    pad_output_timestamp = new timestamp_pad( "output_timestamp" );
    pad_output_metadata = new metadata_pad( "output_metadata" );
    pad_output_type = new detected_type_pad( "output_type" );

    proc_mask_reader = new mask_reader_process( "mask_reader" );
    config.add_subblock( proc_mask_reader->params(), proc_mask_reader->name() );

    proc_mask_select = new mask_select_process( "mask_select" );
    config.add_subblock( proc_mask_select->params(), proc_mask_select->name() );

    proc_greyscale = new greyscale_process< PixType >( "rgb_to_grey" );
    config.add_subblock( proc_greyscale->params(), proc_greyscale->name() );

    proc_features = new pixel_feature_extractor_super_process< PixType, vxl_byte >( "feature_sp" );
    config.add_subblock( proc_features->params(), proc_features->name() );

    proc_mask_detector = new scene_obstruction_detector_process< PixType >( "mask_detector" );
    config.add_subblock( proc_mask_detector->params(), proc_mask_detector->name() );

    proc_mask_recognizer = new osd_recognizer_process< PixType >( "mask_recognizer" );
    config.add_subblock( proc_mask_recognizer->params(), proc_mask_recognizer->name() );

    proc_mask_refiner1 = new osd_mask_refinement_process< PixType >( "mask_refiner1" );
    config.add_subblock( proc_mask_refiner1->params(), proc_mask_refiner1->name() );

    proc_mask_refiner2 = new osd_mask_refinement_process< PixType >( "mask_refiner2" );
    config.add_subblock( proc_mask_refiner2->params(), proc_mask_refiner2->name() );

#ifdef USE_CAFFE
    proc_cnn_detector = new cnn_detector_process< PixType >( "cnn_detector" );
    config.add_subblock( proc_cnn_detector->params(), proc_cnn_detector->name() );

    proc_converter = new convert_image_process< float, double >( "detection_converter" );
    config.add_subblock( proc_converter->params(), proc_converter->name() );
#endif

    proc_mdb_filter = new simple_burnin_filter_process< PixType >( "mdb_filter" );
    config.add_subblock( proc_mdb_filter->params(), proc_mdb_filter->name() );

    proc_burnin_detect1 = new moving_burnin_detector_process( "burnin_detect1" );
    config.add_subblock( proc_burnin_detect1->params(), proc_burnin_detect1->name() );

    proc_burnin_detect2 = new moving_burnin_detector_process( "burnin_detect2" );
    config.add_subblock( proc_burnin_detect2->params(), proc_burnin_detect2->name() );

    proc_text_parser = new metadata_text_parser_process< PixType >( "text_parser" );
    config.add_subblock( proc_text_parser->params(), proc_text_parser->name() );

    proc_border_det = new border_detection_process< PixType >( "border_detection" );
    config.add_subblock( proc_border_det->params(), proc_border_det->name() );

    proc_mask_merge1 = new mask_merge_process( "burnin_mask_merge" );
    config.add_subblock( proc_mask_merge1->params(), proc_mask_merge1->name() );

    proc_mask_merge2 = new mask_merge_process( "final_mask_merge" );
    config.add_subblock( proc_mask_merge2->params(), proc_mask_merge2->name() );

    proc_mask_overlay = new mask_overlay_process( "mask_overlay" );
    config.add_subblock( proc_mask_overlay->params(), proc_mask_overlay->name() );

    proc_overlay_writer = new image_list_writer_process< vxl_byte >( "overlay_writer" );
    config.add_subblock( proc_overlay_writer->params(), proc_overlay_writer->name() );

    proc_filter_writer = new image_list_writer_process< vxl_byte >( "filter_writer" );
    config.add_subblock( proc_filter_writer->params(), proc_filter_writer->name() );

    proc_mask_writer = new image_list_writer_process< bool >( "mask_writer" );
    config.add_subblock( proc_mask_writer->params(), proc_mask_writer->name() );

    // Over-riding the process default with this super-process defaults.
    default_config.add_parameter( proc_mask_refiner1->name() + ":refiner_index", "1", "Default override" );
    default_config.add_parameter( proc_mask_refiner2->name() + ":refiner_index", "2", "Default override" );
    default_config.add_parameter( proc_overlay_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_overlay_writer->name() + ":pattern", "overlay-%2$04d.png", "Default override" );
    default_config.add_parameter( proc_filter_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_filter_writer->name() + ":pattern", "filter-%2$04d.png", "Default override" );
    default_config.add_parameter( proc_filter_writer->name() + ":skip_unset_images", "true", "Default override" );
    default_config.add_parameter( proc_mask_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_mask_writer->name() + ":pattern", "mask-%2$04d.png", "Default override" );

    // Update actual config block.
    config.update( default_config );
  }

  template < class Pipeline >
  void setup_pipeline( Pipeline * p )
  {
    p->add( pad_source_image );
    p->add( pad_source_metadata );
    p->add( pad_source_timestamp );

    p->add( pad_output_image );
    p->add( pad_output_mask );
    p->add( pad_output_timestamp );
    p->add( pad_output_metadata );

    // Pre-compute different enabled/disabled flags
    bool overlay_writer_disabled = config.get< bool >( proc_overlay_writer->name() + ":disabled" );
    bool filter_writer_disabled = config.get< bool >( proc_filter_writer->name() + ":disabled" );
    bool mask_writer_disabled = config.get< bool >( proc_mask_writer->name() + ":disabled" );
    bool template_matching_option_enabled = ( detection_mode != BORDER_DET_ONLY && allow_template_matching );

    // For now, the image and timestamp are simply a passed through this super process. Later on
    // it may be desirable to add a lag at the beginning of a video for improved performance.
    p->set_edge_capacity( refiner_edge_capacity );
    p->connect( pad_source_image->value_port(),
                pad_output_image->set_value_port() );
    p->connect( pad_source_timestamp->value_port(),
                pad_output_timestamp->set_value_port() );
    p->set_edge_capacity( default_edge_capacity );

    // Add capability for running template matching to the pipeline, if enabled.
    if( template_matching_option_enabled )
    {
      p->add( proc_mdb_filter );
      p->add( proc_burnin_detect1 );
      p->add( proc_burnin_detect2 );
      p->add( proc_mask_merge1 );
      p->add( proc_mask_merge2 );

       /*
        *    src
        *     |
        *     v
        *  mdb_filter
        */
      p->set_edge_capacity( refiner_edge_capacity );
      p->connect( pad_source_image->value_port(),
                  proc_mdb_filter->set_source_image_port() );
      p->set_edge_capacity( default_edge_capacity );

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
        *   merge <-----
        *     |
        *     v
        *   merge
        */
      p->connect( proc_burnin_detect1->metadata_mask_port(),
                  proc_mask_merge1->set_mask_a_port() );
      p->connect( proc_burnin_detect2->metadata_mask_port(),
                  proc_mask_merge1->set_mask_b_port() );
      p->connect( proc_mask_merge1->mask_port(),
                  proc_mask_merge2->set_mask_a_port() );
    }

    // Manual mask with template matching specific setup
    if( detection_mode == MANUAL_MASK )
    {
      p->add( proc_mask_reader );
      p->add( proc_mask_select );

       /*
        *  mdb_filter   reader
        *     |           |
        *     v           |
        *  mask_select <---
        */
      p->connect( proc_mdb_filter->metadata_image_port(),
                  proc_mask_select->set_data_image_port() );
      p->connect( proc_mask_reader->mask_image_port(),
                  proc_mask_select->set_mask_port() );

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
                  proc_mask_merge2->set_mask_b_port() );
      p->connect( proc_mask_merge2->mask_port(),
                  pad_output_mask->set_value_port() );
    }

    // Pixel classifier pipeline specific setup
    if( detection_mode == PIXEL_CLASS || detection_mode == CNN_CLASS )
    {
      p->add( proc_greyscale );
      p->add( proc_border_det );
      p->add( proc_features );

      if( detection_mode == PIXEL_CLASS )
      {
        p->add( proc_mask_detector );
      }
#ifdef USE_CAFFE
      else
      {
        p->add( proc_cnn_detector );
        p->add( proc_converter );
      }
#endif

      p->add( proc_mask_recognizer );
      p->add( proc_mask_refiner1 );
      p->add( proc_mask_refiner2 );
      p->add( pad_output_mask_no_border );
      p->add( pad_output_border );
      p->add( pad_output_type );

      // Connect RGB input image to required processes
      p->connect( pad_source_image->value_port(),
                  proc_greyscale->set_image_port() );
      p->connect( pad_source_image->value_port(),
                  proc_features->set_source_color_image_port() );
      p->connect( pad_source_image->value_port(),
                  proc_border_det->set_source_color_image_port() );
      p->connect( pad_source_image->value_port(),
                  proc_mask_recognizer->set_source_image_port() );

      if( detection_mode == PIXEL_CLASS )
      {
        p->connect( pad_source_image->value_port(),
                    proc_mask_detector->set_color_image_port() );
      }
#ifdef USE_CAFFE
      else
      {
        p->connect( pad_source_image->value_port(),
                    proc_cnn_detector->set_source_image_port() );
        p->connect( proc_cnn_detector->heatmap_image_port(),
                    proc_converter->set_image_port() );
      }
#endif

      // Connect greyscale image to required processes
      p->connect( proc_greyscale->copied_image_port(),
                  proc_features->set_source_grey_image_port() );
      p->connect( proc_greyscale->copied_image_port(),
                  proc_border_det->set_source_gray_image_port() );

      // Connect border detector to all required processes
      if( use_border_in_osd_detection )
      {
        p->connect( proc_border_det->border_port(),
                    proc_features->set_border_port() );
        p->connect( proc_border_det->border_port(),
                    proc_mask_recognizer->set_border_port() );

        if( detection_mode == PIXEL_CLASS )
        {
          p->connect( proc_border_det->border_port(),
                      proc_mask_detector->set_border_port() );
        }
      }
      else
      {
        p->set_edge_capacity( refiner_edge_capacity );
        p->connect( proc_border_det->border_port(),
                    proc_features->set_border_port() );
        p->connect( proc_border_det->border_port(),
                    proc_mask_merge2->set_border_port() );
        p->set_edge_capacity( default_edge_capacity );
      }

      // Connect feature array to all mask classifiers
      if( detection_mode == PIXEL_CLASS )
      {
        p->connect( proc_features->feature_array_port(),
                    proc_mask_detector->set_input_features_port() );
        p->connect( proc_features->var_image_port(),
                    proc_mask_detector->set_variance_image_port() );
      }
      p->connect( proc_features->feature_array_port(),
                  proc_mask_recognizer->set_input_features_port() );

      // Connect mask detector to the mask recognizer
      if( detection_mode == PIXEL_CLASS )
      {
        p->connect( proc_mask_detector->classified_image_port(),
                    proc_mask_recognizer->set_classified_image_port() );
        p->connect( proc_mask_detector->mask_properties_port(),
                    proc_mask_recognizer->set_mask_properties_port() );
      }
#ifdef USE_CAFFE
      else
      {
        p->connect( proc_converter->converted_image_port(),
                    proc_mask_recognizer->set_classified_image_port() );
      }
#endif

      // Connect recognizer to traditional template matchers
      p->set_edge_capacity( refiner_edge_capacity );
      p->connect( proc_mask_recognizer->output_port(),
                  proc_mdb_filter->set_burnin_properties_port() );
      p->connect( proc_mask_recognizer->dynamic_instruction_set_1_port(),
                  proc_burnin_detect1->set_parameter_file_port() );
      p->connect( proc_mask_recognizer->dynamic_instruction_set_2_port(),
                  proc_burnin_detect2->set_parameter_file_port() );
      p->set_edge_capacity( default_edge_capacity );

      // Connect recognizer to refiners
      p->connect( proc_mask_recognizer->output_port(),
                  proc_mask_refiner1->set_osd_info_port() );
      p->connect( proc_mask_recognizer->mask_image_port(),
                  proc_mask_refiner1->set_mask_image_port() );
      p->connect( proc_mdb_filter->metadata_image_port(),
                  proc_mask_refiner1->set_new_feature_port() );
      p->connect( proc_mask_refiner1->mask_image_port(),
                  proc_mask_refiner2->set_mask_image_port() );
      p->connect( proc_mask_refiner1->osd_info_port(),
                  proc_mask_refiner2->set_osd_info_port() );

      // Set output mask pads
      p->connect( proc_mask_refiner2->mask_image_port(),
                  proc_mask_merge2->set_mask_b_port() );
      p->connect( proc_mask_merge2->mask_port(),
                  pad_output_mask->set_value_port() );
      p->connect( proc_mask_merge2->borderless_mask_port(),
                  pad_output_mask_no_border->set_value_port() );

      // Connect output border pad and output type pad
      p->set_edge_capacity( refiner_edge_capacity );
      p->connect( proc_border_det->border_port(),
                  pad_output_border->set_value_port() );
      p->connect( proc_mask_recognizer->detected_type_port(),
                  pad_output_type->set_value_port() );
      p->set_edge_capacity( default_edge_capacity );
    }

    // Border detection only option setup
    if( detection_mode == BORDER_DET_ONLY )
    {
      p->add( proc_border_det );
      p->add( pad_output_border );

      // Connect border detector directly to the output mask
      p->connect( pad_source_image->value_port(),
                  proc_border_det->set_source_color_image_port() );
      p->connect( proc_border_det->border_mask_port(),
                  pad_output_mask->set_value_port() );

      // Set output border pad
      p->connect( proc_border_det->border_port(),
                  pad_output_border->set_value_port() );
    }

    // Enable text detection process, only if necessary
    if( allow_text_parsing )
    {
      p->add( proc_text_parser );

      // Connect required text parser inputs and outputs
      p->connect( pad_source_metadata->value_port(),
                  proc_text_parser->set_metadata_port() );
      p->connect( proc_mdb_filter->metadata_image_port(),
                  proc_text_parser->set_image_port() );
      p->connect( proc_text_parser->output_metadata_port(),
                  pad_output_metadata->set_value_port() );

      // Connect detected border to text parser if enabled
      if( detection_mode == PIXEL_CLASS ||
          detection_mode == BORDER_DET_ONLY ||
          detection_mode == CNN_CLASS )
      {
        p->connect( proc_border_det->border_port(),
                    proc_text_parser->set_border_port() );
      }

      // Connect target widths to text parser if enabled
      if( template_matching_option_enabled )
      {
        p->connect( proc_burnin_detect1->target_pixel_widths_port(),
                    proc_text_parser->set_target_widths_port() );
      }

      // Connect action commands from recognized template
      if( detection_mode == PIXEL_CLASS || detection_mode == CNN_CLASS )
      {
        p->connect( proc_mask_recognizer->text_instructions_port(),
                    proc_text_parser->set_instructions_port() );
      }
    }
    else
    {
      // Pass metadata through unchanged
      p->set_edge_capacity( refiner_edge_capacity );
      p->connect( pad_source_metadata->value_port(),
                  pad_output_metadata->set_value_port() );
      p->set_edge_capacity( default_edge_capacity );
    }

    // Connect optional overlay writer
    if( !overlay_writer_disabled )
    {
      p->add( proc_mask_overlay );
      p->add( proc_overlay_writer );

      p->set_edge_capacity( refiner_edge_capacity );
      p->connect( pad_source_image->value_port(),
                  proc_mask_overlay->set_source_image_port() );

      if( detection_mode == BORDER_DET_ONLY )
      {
        p->connect( proc_border_det->border_mask_port(),
                    proc_mask_overlay->set_mask_port() );
      }
      else
      {
        p->connect( proc_mask_merge2->mask_port(),
                    proc_mask_overlay->set_mask_port() );
      }

      p->connect( proc_mask_overlay->image_port(),
                  proc_overlay_writer->set_image_port() );
      p->connect( pad_source_timestamp->value_port(),
                  proc_overlay_writer->set_timestamp_port() );
      p->set_edge_capacity( default_edge_capacity );
    }

    // Connect optional mask writer
    if( !mask_writer_disabled )
    {
      p->add( proc_mask_writer );

      if( detection_mode == BORDER_DET_ONLY )
      {
        p->connect( proc_border_det->border_mask_port(),
                    proc_mask_writer->set_image_port() );
      }
      else
      {
        p->set_edge_capacity( refiner_edge_capacity );
        p->connect( proc_mask_merge2->mask_port(),
                    proc_mask_writer->set_image_port() );
        p->set_edge_capacity( default_edge_capacity );
      }

      p->connect( pad_source_timestamp->value_port(),
                  proc_mask_writer->set_timestamp_port() );
    }

    // Connect optional filter writer
    if( template_matching_option_enabled && !filter_writer_disabled )
    {
      p->add( proc_filter_writer );

      p->connect( proc_mdb_filter->metadata_image_port(),
                  proc_filter_writer->set_image_port() );
      p->connect( pad_source_timestamp->value_port(),
                  proc_filter_writer->set_timestamp_port() );
    }
  }
};

template < class PixType >
metadata_mask_super_process<PixType>
::metadata_mask_super_process( std::string const& _name )
  : super_process( _name, "metadata_mask_super_process" ),
    impl_( NULL )
{
  impl_ = new metadata_mask_super_process_impl<PixType>;

  impl_->create_process_configs();

  impl_->config.add_parameter( "run_async",
    "true",
    "Whether or not to run asynchronously" );

  impl_->config.add_parameter( "pipeline_edge_capacity",
    boost::lexical_cast< std::string >( 10 / sizeof( PixType ) ),
    "Maximum size of the edge in an async pipeline." );

  impl_->config.add_parameter( "detection_mode",
    "manual_mask",
    "Operating mode of the super process. Can be one of the following values: "
    "\"manual_mask\", \"pixel_classifier\", \"cnn_classifier\" or "
    "\"border_detect_only\" for different mask generation variants." );

  impl_->config.add_parameter( "allow_text_parsing",
    "false",
    "Should parsing text in the input imagery be allowed?" );

  impl_->config.add_parameter( "allow_template_matching",
    "true",
    "Should template matching in the input imagery be allowed?" );

  impl_->config.add_parameter( "use_border_in_osd_detection",
    "true",
    "Should the detected image border be used for on-screen display detection,"
    "i.e., should we look for OSD components in the entire image, or just in "
    "the area encapsulated by the image border." );

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

  config_enum_option det_mode;
  det_mode.add( "manual_mask",        impl_t::MANUAL_MASK )
          .add( "pixel_classifier",   impl_t::PIXEL_CLASS )
          .add( "cnn_classifier",     impl_t::CNN_CLASS )
          .add( "border_detect_only", impl_t::BORDER_DET_ONLY )
    ;

  try
  {
    bool run_async = blk.get< bool >( "run_async" );

    impl_->detection_mode = blk.get< typename impl_t::detection_mode_t >( det_mode, "detection_mode" );

    impl_->allow_text_parsing = blk.get< bool >( "allow_text_parsing" );
    impl_->allow_template_matching = blk.get< bool >( "allow_template_matching" );
    impl_->use_border_in_osd_detection = blk.get< bool >( "use_border_in_osd_detection" );
    impl_->default_edge_capacity = blk.get< unsigned >( "pipeline_edge_capacity" );

    // Compute required edge lengths for internal buffers
    unsigned refine_delay1 = blk.get< unsigned >(
      impl_->proc_mask_detector->name() + ":reset_buffer_length" );
    unsigned refine_delay2 = blk.get< unsigned >(
      impl_->proc_mask_refiner1->name() + ":reset_buffer_length" );
    unsigned refine_delay3 = blk.get< unsigned >(
      impl_->proc_mask_refiner2->name() + ":reset_buffer_length" );

    impl_->refiner_edge_capacity = std::max( impl_->default_edge_capacity,
      std::max( refine_delay1, std::max( refine_delay2, refine_delay3 ) ) );

    impl_->config.update( blk );

    if( run_async )
    {
      async_pipeline* p = new async_pipeline( async_pipeline::ENABLE_STATUS_FORWARDING,
                                              impl_->default_edge_capacity );
      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else if( refine_delay1 > 0 || refine_delay2 > 0 )
    {
      throw config_block_parse_error( "Cannot use a mask refinement delay in sync mode" );
    }
    else
    {
      sync_pipeline* p = new sync_pipeline;

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }

    videoname_prefix::instance()->add_videoname_prefix(
      impl_->config, impl_->proc_overlay_writer->name() + ":pattern" );
    videoname_prefix::instance()->add_videoname_prefix(
      impl_->config, impl_->proc_filter_writer->name() + ":pattern" );
    videoname_prefix::instance()->add_videoname_prefix(
      impl_->config, impl_->proc_mask_writer->name() + ":pattern" );

    if( !pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error( "Unable to set pipeline parameters." );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );
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
::set_image( vil_image_view< PixType > const& img )
{
  impl_->pad_source_image->set_value( img );
}

template < class PixType >
void
metadata_mask_super_process<PixType>
::set_timestamp( vidtk::timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}

template < class PixType >
void
metadata_mask_super_process<PixType>
::set_metadata( vidtk::video_metadata const& md )
{
  impl_->pad_source_metadata->set_value( video_metadata::vector_t( 1, md ) );
}

template < class PixType >
void
metadata_mask_super_process<PixType>
::set_metadata_vector( vidtk::video_metadata::vector_t const& md )
{
  impl_->pad_source_metadata->set_value( md );
}

template < class PixType >
vil_image_view< PixType >
metadata_mask_super_process<PixType>
::image() const
{
  return impl_->pad_output_image->value();
}

template < class PixType >
vil_image_view< bool >
metadata_mask_super_process<PixType>
::mask() const
{
  return impl_->pad_output_mask->value();
}

template < class PixType >
std::string
metadata_mask_super_process<PixType>
::detected_type() const
{
  return impl_->pad_output_type->value();
}

template < class PixType >
vil_image_view< bool >
metadata_mask_super_process<PixType>
::borderless_mask() const
{
  return impl_->pad_output_mask_no_border->value();
}

template < class PixType >
image_border
metadata_mask_super_process<PixType>
::border() const
{
  return impl_->pad_output_border->value();
}

template < class PixType >
vidtk::timestamp
metadata_mask_super_process<PixType>
::timestamp() const
{
  return impl_->pad_output_timestamp->value();
}

template < class PixType >
video_metadata
metadata_mask_super_process<PixType>
::metadata() const
{
  if( impl_->pad_output_metadata->value().empty() )
  {
    return video_metadata();
  }

  return ( impl_->pad_output_metadata->value() )[ 0 ];
}

template < class PixType >
double
metadata_mask_super_process<PixType>
::world_units_per_pixel() const
{
  return 1.0;
}

template < class PixType >
video_metadata::vector_t
metadata_mask_super_process<PixType>
::metadata_vector() const
{
  return impl_->pad_output_metadata->value();
}


} // end namespace vidtk
