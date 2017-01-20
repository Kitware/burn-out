/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "saliency_detector_super_process.h"

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <utilities/config_block.h>

#include <tracking_data/image_object.h>

#include <tracking_data/io/track_writer_process.h>

#include <video_io/image_list_writer_process.h>

#include <video_properties/border_detection_process.h>

#include <video_transforms/greyscale_process.h>
#include <video_transforms/world_smooth_image_process.h>
#include <video_transforms/deep_copy_image_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/mask_merge_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/mask_overlay_process.h>

#include <object_detectors/pixel_feature_extractor_super_process.h>
#include <object_detectors/scene_obstruction_detector_process.h>
#include <object_detectors/osd_recognizer_process.h>
#include <object_detectors/salient_region_classifier_process.h>
#include <object_detectors/three_frame_differencing_process.h>
#include <object_detectors/sg_background_model_process.h>

#include <classifier/hashed_image_classifier_process.h>

#include <logger/logger.h>


namespace vidtk
{

VIDTK_LOGGER( "saliency_detector_super_process" );

template< class PixType>
class saliency_detector_super_process_impl
{
public:

  // Pad Typedefs
  typedef vidtk::super_process_pad_impl< vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl< double > gsd_pad;
  typedef vidtk::super_process_pad_impl< timestamp > timestamp_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > fg_img_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< float > > diff_img_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > mask_pad;
  typedef vidtk::super_process_pad_impl< vgl_h_matrix_2d< double > > vgl_2d_pad;
  typedef vidtk::super_process_pad_impl< image_to_utm_homography > src2utm_pad;
  typedef vidtk::super_process_pad_impl< plane_to_image_homography > wld2src_pad;
  typedef vidtk::super_process_pad_impl< image_to_plane_homography > src2wld_pad;
  typedef vidtk::super_process_pad_impl< image_to_image_homography > src2ref_pad;
  typedef vidtk::super_process_pad_impl< image_to_plane_homography > img2pln_pad;
  typedef vidtk::super_process_pad_impl< shot_break_flags > shot_break_flags_pad;
  typedef vidtk::super_process_pad_impl< video_modality > modality_pad;

  // Configuration parameters
  config_block config;
  config_block default_config;
  config_block dependency_config;

  enum{ NONE, USE_INTERNAL_DETECTOR, USE_EXTERNAL_MASK } masking_mode;

  bool enable_sal_writer;
  bool enable_fg_writer;

  // Input Pads (dummy edge processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< src2utm_pad > pad_source_src2utm_homog;
  process_smart_pointer< wld2src_pad > pad_source_wld2src_homog;
  process_smart_pointer< src2wld_pad > pad_source_src2wld_homog;
  process_smart_pointer< src2ref_pad > pad_source_src2ref_homog;
  process_smart_pointer< gsd_pad > pad_source_gsd;
  process_smart_pointer< mask_pad > pad_source_mask;
  process_smart_pointer< shot_break_flags_pad > pad_source_shot_breaks;
  process_smart_pointer< img2pln_pad > pad_source_img2pln_homog;
  process_smart_pointer< modality_pad > pad_source_modality;

  // Pipeline Processes
  process_smart_pointer< greyscale_process<PixType> > proc_greyscale;
  process_smart_pointer< pixel_feature_extractor_super_process<PixType,vxl_byte> > proc_features;
  process_smart_pointer< scene_obstruction_detector_process<PixType> > proc_mask_detector;
  process_smart_pointer< osd_recognizer_process<PixType> > proc_mask_recognizer;
  process_smart_pointer< hashed_image_classifier_process<vxl_byte> > proc_classifier1;
  process_smart_pointer< salient_region_classifier_process<PixType> > proc_classifier2;
  process_smart_pointer< deep_copy_image_process<PixType> > proc_copy;
  process_smart_pointer< border_detection_process<PixType> > proc_border;
  process_smart_pointer< threshold_image_process<double> > proc_threshold;

  // Optional Data Writers
  process_smart_pointer< image_list_writer_process<float> > proc_sal_writer;
  process_smart_pointer< image_list_writer_process<bool> > proc_fg_writer;

  // Output Pads (dummy edge processes)
  process_smart_pointer< image_pad > pad_output_image;
  process_smart_pointer< timestamp_pad > pad_output_timestamp;
  process_smart_pointer< gsd_pad > pad_output_gsd;
  process_smart_pointer< src2utm_pad > pad_output_src2utm_homog;
  process_smart_pointer< wld2src_pad > pad_output_wld2src_homog;
  process_smart_pointer< src2wld_pad > pad_output_src2wld_homog;
  process_smart_pointer< src2ref_pad > pad_output_src2ref_homog;
  process_smart_pointer< fg_img_pad > pad_output_fg_image;
  process_smart_pointer< diff_img_pad > pad_output_sal_image;
  process_smart_pointer< shot_break_flags_pad > pad_output_shot_breaks;
  process_smart_pointer< img2pln_pad > pad_output_img2pln_homog;
  process_smart_pointer< modality_pad > pad_output_modality;

  // Default initializer
  saliency_detector_super_process_impl()
  : config(),
    default_config(),
    dependency_config(),
    masking_mode( NONE ),
    enable_sal_writer( false ),
    enable_fg_writer( false ),

    // Input Pads
    pad_source_image( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_src2utm_homog( NULL ),
    pad_source_wld2src_homog( NULL ),
    pad_source_src2wld_homog( NULL ),
    pad_source_src2ref_homog( NULL ),
    pad_source_gsd( NULL ),
    pad_source_mask( NULL ),
    pad_source_shot_breaks( NULL ),
    pad_source_img2pln_homog( NULL ),
    pad_source_modality( NULL ),

    // Processes
    proc_greyscale( NULL ),
    proc_features( NULL ),
    proc_mask_detector( NULL ),
    proc_mask_recognizer( NULL ),
    proc_classifier1( NULL ),
    proc_classifier2( NULL ),
    proc_copy( NULL ),
    proc_border( NULL ),
    proc_threshold( NULL ),
    proc_sal_writer( NULL ),
    proc_fg_writer( NULL ),

    // Output Pads
    pad_output_image( NULL ),
    pad_output_timestamp( NULL ),
    pad_output_gsd( NULL ),
    pad_output_src2utm_homog( NULL ),
    pad_output_wld2src_homog( NULL ),
    pad_output_src2wld_homog( NULL ),
    pad_output_src2ref_homog( NULL ),
    pad_output_fg_image( NULL ),
    pad_output_sal_image( NULL ),
    pad_output_shot_breaks( NULL ),
    pad_output_img2pln_homog( NULL ),
    pad_output_modality( NULL )
  {}

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    // Input Pads
    pad_source_image = new image_pad("source_image");
    pad_source_timestamp = new timestamp_pad("source_timestamp");
    pad_source_gsd = new gsd_pad("source_gsd");
    pad_source_src2utm_homog = new src2utm_pad("source_wld2utm_H");
    pad_source_wld2src_homog = new wld2src_pad("source_wld2src_H");
    pad_source_src2wld_homog = new src2wld_pad("source_src2wld_H");
    pad_source_src2ref_homog = new src2ref_pad("source_src2ref_H");
    pad_source_mask = new mask_pad("source_mask");
    pad_source_shot_breaks = new shot_break_flags_pad("source_shot_breaks");
    pad_source_img2pln_homog = new img2pln_pad("source_img2pln_homog");
    pad_source_modality = new modality_pad("source_video_modality");

    // Output Pads
    pad_output_image = new image_pad("output_image");
    pad_output_timestamp = new timestamp_pad("output_timestamp");
    pad_output_gsd = new gsd_pad("output_gsd");
    pad_output_src2utm_homog = new src2utm_pad("output_wld2utm_H");
    pad_output_wld2src_homog = new wld2src_pad("output_wld2src_H");
    pad_output_src2wld_homog = new src2wld_pad("output_src2wld_H");
    pad_output_src2ref_homog = new src2ref_pad("output_src2ref_H");
    pad_output_fg_image = new fg_img_pad("output_fg_img");
    pad_output_sal_image = new diff_img_pad("output_diff_img");
    pad_output_shot_breaks = new shot_break_flags_pad("output_shot_breaks");
    pad_output_img2pln_homog = new img2pln_pad("output_img2pln_homog");
    pad_output_modality = new modality_pad("output_video_modality");

    // Processes
    proc_greyscale = new greyscale_process<PixType>( "rgb_to_grey" );
    config.add_subblock( proc_greyscale->params(),
                         proc_greyscale->name() );

    proc_features = new pixel_feature_extractor_super_process<PixType,vxl_byte>( "feature_sp" );
    config.add_subblock( proc_features->params(),
                         proc_features->name() );

    proc_mask_detector = new scene_obstruction_detector_process<PixType>( "mask_detector" );
    config.add_subblock( proc_mask_detector->params(),
                         proc_mask_detector->name() );

    proc_mask_recognizer = new osd_recognizer_process<PixType>( "mask_recognizer" );
    config.add_subblock( proc_mask_recognizer->params(),
                         proc_mask_recognizer->name() );

    proc_classifier1 = new hashed_image_classifier_process<vxl_byte>( "initial_classifier" );
    config.add_subblock( proc_classifier1->params(),
                         proc_classifier1->name() );

    proc_classifier2 = new salient_region_classifier_process<PixType>( "final_classifier" );
    config.add_subblock( proc_classifier2->params(),
                         proc_classifier2->name() );

    proc_copy = new deep_copy_image_process<PixType>("deep_copy");
    config.add_subblock( proc_copy->params(),
                         proc_copy->name() );

    proc_border = new border_detection_process<PixType>("border_detector");
    config.add_subblock( proc_border->params(),
                         proc_border->name() );

    proc_threshold = new threshold_image_process<double>( "thresholder" );
    config.add_subblock( proc_threshold->params(),
                         proc_threshold->name() );

    proc_sal_writer = new image_list_writer_process<float>( "saliency_writer" );
    config.add_subblock( proc_sal_writer->params(),
                         proc_sal_writer->name() );

    proc_fg_writer = new image_list_writer_process<bool>( "fg_writer" );
    config.add_subblock( proc_fg_writer->params(),
                         proc_fg_writer->name() );

    // Over-riding the process default with this super-process defaults.
    default_config.add_parameter( proc_sal_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_fg_writer->name() + ":disabled", "true", "Default override" );
    config.update( default_config );

    // Over-ride the async/sync run mode of the pixel feature extractor
    // and hide the config parameter from the user.
    dependency_config.add_parameter( proc_features->name() + ":run_async", "false", "Force override" );
  }

  // Create sync or async pipeline
  template <class PIPELINE>
  void setup_pipeline( PIPELINE * p )
  {
    // Add Required Input Pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_gsd );
    p->add( pad_source_src2utm_homog );
    p->add( pad_source_wld2src_homog );
    p->add( pad_source_src2wld_homog );
    p->add( pad_source_src2ref_homog );
    p->add( pad_source_shot_breaks );
    p->add( pad_source_img2pln_homog );
    p->add( pad_source_modality );

    // Required Output Pads
    p->add( pad_output_image );
    p->add( pad_output_timestamp );
    p->add( pad_output_gsd );
    p->add( pad_output_src2utm_homog );
    p->add( pad_output_wld2src_homog );
    p->add( pad_output_src2wld_homog );
    p->add( pad_output_src2ref_homog );
    p->add( pad_output_fg_image );
    p->add( pad_output_sal_image );
    p->add( pad_output_shot_breaks );
    p->add( pad_output_img2pln_homog );
    p->add( pad_output_modality );

    // Add worker processes
    p->add( proc_greyscale );
    p->add( proc_features );
    p->add( proc_classifier1 );
    p->add( proc_classifier2 );
    p->add( proc_copy );
    p->add( proc_border );
    p->add( proc_threshold );

    // Connect RGB input to greyscale converter
    p->connect( pad_source_image->value_port(),
                proc_copy->set_source_image_port() );
    p->connect( proc_copy->image_port(),
                proc_greyscale->set_image_port() );

    // Connect RGB input image to everything else
    p->connect( proc_copy->image_port(),
                proc_classifier2->set_input_image_port() );
    p->connect( proc_copy->image_port(),
                proc_border->set_source_color_image_port() );

    // Connect feature extractor inputs
    p->connect( proc_greyscale->copied_image_port(),
                proc_features->set_source_grey_image_port() );
    p->connect( pad_source_image->value_port(),
                proc_features->set_source_color_image_port() );

    // Connect feature array to all classifiers
    p->connect( proc_features->feature_array_port(),
                proc_classifier1->set_pixel_features_port() );
    p->connect( proc_features->feature_array_port(),
                proc_classifier2->set_input_features_port() );

    // Connect border detector to all classifiers
    p->connect( proc_border->border_port(),
                proc_features->set_border_port() );
    p->connect( proc_border->border_port(),
                proc_classifier2->set_border_port() );

    // Connect optional scene obstructor detectors
    if( masking_mode == USE_INTERNAL_DETECTOR )
    {
      p->add( proc_mask_detector );
      p->add( proc_mask_recognizer );

      // Connect inputs to mask detectors
      p->connect( proc_copy->image_port(),
                  proc_mask_detector->set_color_image_port() );
      p->connect( proc_copy->image_port(),
                  proc_mask_recognizer->set_source_image_port() );
      p->connect( proc_features->feature_array_port(),
                  proc_mask_detector->set_input_features_port() );
      p->connect( proc_features->feature_array_port(),
                  proc_mask_recognizer->set_input_features_port() );
      p->connect( proc_border->border_port(),
                  proc_mask_detector->set_border_port() );
      p->connect( proc_border->border_port(),
                  proc_mask_recognizer->set_border_port() );
      p->connect( proc_features->var_image_port(),
                  proc_mask_detector->set_variance_image_port() );

      // Connect outputs from mask detectors
      p->connect( proc_mask_detector->classified_image_port(),
                  proc_mask_recognizer->set_classified_image_port() );
      p->connect( proc_mask_detector->mask_properties_port(),
                  proc_mask_recognizer->set_mask_properties_port() );
      p->connect( proc_mask_recognizer->mask_image_port(),
                  proc_classifier2->set_obstruction_mask_port() );
    }
    else if( masking_mode == USE_EXTERNAL_MASK )
    {
      p->add( pad_source_mask );

      p->connect( pad_source_mask->value_port(),
                  proc_classifier2->set_obstruction_mask_port() );
    }

    // Connect primary saliency classifier to recognizer
    p->connect( proc_classifier1->classified_image_port(),
                proc_classifier2->set_saliency_map_port() );
    p->connect( proc_classifier1->classified_image_port(),
                proc_threshold->set_source_image_port() );
    p->connect( proc_threshold->thresholded_image_port(),
                proc_classifier2->set_saliency_mask_port() );

    // Connect all output detections
    p->connect( proc_classifier2->foreground_image_port(),
                pad_output_fg_image->set_value_port() );
    p->connect( proc_classifier2->pixel_saliency_port(),
                pad_output_sal_image->set_value_port() );
    p->connect( proc_copy->image_port(),
                pad_output_image->set_value_port() );

    // Connect all optional writers
    if( enable_sal_writer )
    {
      p->add( proc_sal_writer );
      p->connect( pad_source_timestamp->value_port(),
                  proc_sal_writer->set_timestamp_port() );
      p->connect( proc_classifier2->pixel_saliency_port(),
                  proc_sal_writer->set_image_port() );
    }
    if( enable_fg_writer )
    {
      p->add( proc_fg_writer );
      p->connect( pad_source_timestamp->value_port(),
                  proc_fg_writer->set_timestamp_port() );
      p->connect( proc_classifier2->foreground_image_port(),
                  proc_fg_writer->set_image_port() );
    }

    // Connect all outputs
    p->connect( pad_source_gsd->value_port(),
                pad_output_gsd->set_value_port() );
    p->connect( pad_source_timestamp->value_port(),
                pad_output_timestamp->set_value_port() );
    p->connect( pad_source_wld2src_homog->value_port(),
                pad_output_wld2src_homog->set_value_port() );
    p->connect( pad_source_src2utm_homog->value_port(),
                pad_output_src2utm_homog->set_value_port() );
    p->connect( pad_source_src2wld_homog->value_port(),
                pad_output_src2wld_homog->set_value_port() );
    p->connect( pad_source_src2ref_homog->value_port(),
                pad_output_src2ref_homog->set_value_port() );
    p->connect( pad_source_shot_breaks->value_port(),
                pad_output_shot_breaks->set_value_port() );
    p->connect( pad_source_img2pln_homog->value_port(),
                pad_output_img2pln_homog->set_value_port() );
    p->connect( pad_source_modality->value_port(),
                pad_output_modality->set_value_port() );

  }
};

template< class PixType >
saliency_detector_super_process<PixType>
::saliency_detector_super_process( std::string const& _name )
  : super_process( _name, "saliency_detector_super_process" ),
    impl_( NULL )
{
  impl_ = new saliency_detector_super_process_impl<PixType>;

  impl_->create_process_configs();

  impl_->config.add_parameter( "run_async",
    "false",
    "Whether or not to run process asynchronously" );

  impl_->config.add_parameter( "masking_enabled",
    "false",
    "Should we enable masking in the pipeline?" );

  impl_->config.add_parameter( "masking_mode",
    "detect",
    "The masking mode, can be either \"detector\" or \"external\" depending "
    "on if we want to use an externally provided mask, or use the internal "
    "scene obstruction detector process" );
}

template< class PixType >
saliency_detector_super_process<PixType>
::~saliency_detector_super_process()
{
  delete impl_;
}

template< class PixType >
config_block
saliency_detector_super_process<PixType>
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  tmp_config.remove( impl_->dependency_config );
  return tmp_config;
}

template< class PixType >
bool
saliency_detector_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    impl_->config.update( blk );

    bool masking_enable = blk.get<bool>( "masking_enabled" );

    if( masking_enable )
    {
      std::string mode = blk.get<std::string>( "masking_mode" );

      if( mode == "detect" )
      {
        impl_->masking_mode = saliency_detector_super_process_impl<PixType>::USE_INTERNAL_DETECTOR;
      }
      else if( mode == "external" )
      {
        impl_->masking_mode = saliency_detector_super_process_impl<PixType>::USE_EXTERNAL_MASK;
      }
      else
      {
        throw config_block_parse_error( "Invalid masking mode: " + mode );
      }
    }
    else
    {
      impl_->masking_mode = saliency_detector_super_process_impl<PixType>::NONE;
    }

    impl_->enable_sal_writer = !blk.get<bool>( impl_->proc_sal_writer->name() + ":disabled" );
    impl_->enable_fg_writer = !blk.get<bool>( impl_->proc_fg_writer->name() + ":disabled" );

    bool run_async = blk.get<bool>( "run_async" );

    impl_->dependency_config.set( impl_->proc_features->name() + ":run_async", run_async );
    impl_->config.update( impl_->dependency_config );

    if( run_async )
    {
      async_pipeline* p = new async_pipeline(async_pipeline::ENABLE_STATUS_FORWARDING);

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else
    {
      sync_pipeline* p = new sync_pipeline;

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }

    if( !pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error( " unable to set pipeline parameters." );
    }
  }
  catch( config_block_parse_error const& e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: " << e.what() );
    return false;
  }

  return true;
}

template< class PixType >
bool
saliency_detector_super_process<PixType>
::initialize()
{
  return this->pipeline_->initialize();
}


// ----------------------------------------------------------------
/** Main process loop.
 *
 *
 */
template< class PixType >
process::step_status
saliency_detector_super_process<PixType>
::step2()
{
  return this->pipeline_->execute();
}


// ================================================================
// Process Input methods

template< class PixType >
void saliency_detector_super_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  impl_->pad_source_image->set_value( img );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_world_units_per_pixel( double const& gsd )
{
  impl_->pad_source_gsd->set_value( gsd );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_mask( vil_image_view<bool> const& mask )
{
  impl_->pad_source_mask->set_value( mask );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_src_to_ref_homography( image_to_image_homography const& H )
{
  impl_->pad_source_src2ref_homog->set_value( H );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_src_to_wld_homography( image_to_plane_homography const& H )
{
  impl_->pad_source_src2wld_homog->set_value( H );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_wld_to_src_homography( plane_to_image_homography const& H )
{
  impl_->pad_source_wld2src_homog->set_value( H );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_src_to_utm_homography( image_to_utm_homography const& H )
{
  impl_->pad_source_src2utm_homog->set_value( H );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_input_ref_to_wld_homography( image_to_plane_homography const& H )
{
  impl_->pad_source_img2pln_homog->set_value( H );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_input_video_modality( video_modality const& vm )
{
  impl_->pad_source_modality->set_value( vm );
}

template< class PixType >
void saliency_detector_super_process<PixType>
::set_input_shot_break_flags( shot_break_flags const& sb )
{
  impl_->pad_source_shot_breaks->set_value( sb );
}

// ================================================================
// Process output methods

template< class PixType >
vil_image_view<bool>
saliency_detector_super_process<PixType>
::fg_out() const
{
  return impl_->pad_output_fg_image->value();
}

template< class PixType >
vil_image_view<float>
saliency_detector_super_process<PixType>
::diff_out() const
{
  return impl_->pad_output_sal_image->value();
}

template< class PixType >
vil_image_view<bool>
saliency_detector_super_process<PixType>
::copied_fg_out() const
{
  return impl_->pad_output_fg_image->value();
}

template< class PixType >
vil_image_view<float>
saliency_detector_super_process<PixType>
::copied_diff_out() const
{
  return impl_->pad_output_sal_image->value();
}

template< class PixType >
double
saliency_detector_super_process<PixType>
::world_units_per_pixel() const
{
  return impl_->pad_output_gsd->value();
}

template< class PixType >
timestamp
saliency_detector_super_process<PixType>
::source_timestamp() const
{
  return impl_->pad_output_timestamp->value();
}

template< class PixType >
vil_image_view< PixType >
saliency_detector_super_process<PixType>
::source_image() const
{
  return impl_->pad_output_image->value();
}

template< class PixType >
image_to_image_homography
saliency_detector_super_process<PixType>
::src_to_ref_homography() const
{
  return impl_->pad_output_src2ref_homog->value();
}

template< class PixType >
image_to_plane_homography
saliency_detector_super_process<PixType>
::src_to_wld_homography() const
{
  return impl_->pad_output_src2wld_homog->value();
}

template< class PixType >
plane_to_image_homography
saliency_detector_super_process<PixType>
::wld_to_src_homography() const
{
  return impl_->pad_output_wld2src_homog->value();
}

template< class PixType >
image_to_utm_homography
saliency_detector_super_process<PixType>
::src_to_utm_homography() const
{
  return impl_->pad_output_src2utm_homog->value();
}

template< class PixType >
image_to_plane_homography
saliency_detector_super_process<PixType>
::get_output_ref_to_wld_homography() const
{
  return impl_->pad_output_img2pln_homog->value();
}

template< class PixType >
video_modality
saliency_detector_super_process<PixType>
::get_output_video_modality() const
{
  return impl_->pad_output_modality->value();
}

template< class PixType >
shot_break_flags
saliency_detector_super_process<PixType>
::get_output_shot_break_flags() const
{
  return impl_->pad_output_shot_breaks->value();
}

} // end namespace vidtk
