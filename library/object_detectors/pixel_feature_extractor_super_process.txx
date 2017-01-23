/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "pixel_feature_extractor_super_process.h"

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <utilities/config_block.h>

#include <video_io/image_list_writer_process.h>

#include <video_transforms/aligned_edge_detection_process.h>
#include <video_transforms/color_commonality_filter_process.h>
#include <video_transforms/convert_color_space_process.h>
#include <video_transforms/frame_averaging_process.h>
#include <video_transforms/high_pass_filter_process.h>
#include <video_transforms/integral_image_process.h>
#include <video_transforms/image_consolidator_process.h>
#include <video_transforms/mask_merge_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/video_enhancement_process.h>
#include <video_transforms/floating_point_image_hash_process.h>
#include <video_transforms/kmeans_segmentation_process.h>

#include <object_detectors/blob_pixel_feature_extraction_process.h>

#include <logger/logger.h>

#include <boost/lexical_cast.hpp>

namespace vidtk
{


VIDTK_LOGGER( "pixel_feature_extractor_super_process" );


// Total number of toggleable features in this super process
const static unsigned feature_count = 12;


template < class InputType, class OutputType >
class pixel_feature_extractor_super_process_impl
{
public:

  // Helper Typedefs
  typedef std::vector< vil_image_view< OutputType > > feature_array;
  typedef vidtk::super_process_pad_impl< vil_image_view< InputType > > image_pad;
  typedef vidtk::super_process_pad_impl< timestamp > timestamp_pad;
  typedef vidtk::super_process_pad_impl< feature_array > feature_array_pad;
  typedef vidtk::super_process_pad_impl< double > gsd_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< double > > var_image_pad;
  typedef vidtk::super_process_pad_impl< image_border > border_pad;

  // Pipeline Processes
  process_smart_pointer< frame_averaging_process< OutputType > > proc_avg;
  process_smart_pointer< high_pass_filter_process< OutputType > > proc_hp1;
  process_smart_pointer< high_pass_filter_process< OutputType > > proc_hp2;
  process_smart_pointer< integral_image_process< OutputType, int > > proc_integral;
  process_smart_pointer< aligned_edge_detection_process< OutputType > > proc_canny;
  process_smart_pointer< convert_color_space_process< OutputType > > proc_color_convert;
  process_smart_pointer< color_commonality_filter_process< OutputType, OutputType > > proc_cc1;
  process_smart_pointer< color_commonality_filter_process< OutputType, OutputType > > proc_cc2;
  process_smart_pointer< kmeans_segmentation_process< OutputType, OutputType > > proc_kmeans;
  process_smart_pointer< threshold_image_process< OutputType > > proc_thresh;
  process_smart_pointer< blob_pixel_feature_extraction_process< OutputType > > proc_blob_feat;
  process_smart_pointer< video_enhancement_process< OutputType > > proc_enhance;
  process_smart_pointer< image_consolidator_process< OutputType > > proc_merge;
  process_smart_pointer< floating_point_image_hash_process< double, OutputType > > proc_hash1;

  // Input Pads (dummy processes)
  process_smart_pointer< image_pad > pad_source_color_image;
  process_smart_pointer< image_pad > pad_source_grey_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< border_pad > pad_source_border;
  process_smart_pointer< gsd_pad > pad_source_gsd;
  process_smart_pointer< feature_array_pad > pad_source_features;

  // Output Pads (dummy processes)
  process_smart_pointer< feature_array_pad > pad_output_features;
  process_smart_pointer< var_image_pad > pad_output_var_image;

  // Configuration Parameters
  config_block config;
  config_block default_config;
  bool disabled;
  bool single_channel_only;

  // An ordered array of which features should be enabled. Note: this array
  // has a one to one correspondance of all toggleable features and is in
  // the same order as the below list of process parameters.
  bool feature_enable_flags[ feature_count ];

  // Are certain intermediate output processes required?
  bool color_pad_required;
  bool grey_pad_required;
  bool gsd_pad_required;
  bool cc1_filter_required;

  // Defaults
  pixel_feature_extractor_super_process_impl()
  : proc_avg( NULL ),
    proc_hp1( NULL ),
    proc_hp2( NULL ),
    proc_integral( NULL ),
    proc_canny( NULL ),
    proc_color_convert( NULL ),
    proc_cc1( NULL ),
    proc_cc2( NULL ),
    proc_kmeans( NULL ),
    proc_thresh( NULL ),
    proc_blob_feat( NULL ),
    proc_enhance( NULL ),
    proc_merge( NULL ),
    proc_hash1( NULL ),
    pad_source_color_image( NULL ),
    pad_source_grey_image( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_border( NULL ),
    pad_source_gsd( NULL ),
    pad_source_features( NULL ),
    pad_output_features( NULL ),
    pad_output_var_image( NULL ),
    disabled( false ),
    single_channel_only( false ),
    color_pad_required( false ),
    grey_pad_required( false ),
    gsd_pad_required( false ),
    cc1_filter_required( false )
  {
  }

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    pad_source_color_image = new image_pad( "source_rgb_image" );
    pad_source_grey_image = new image_pad( "source_grey_image" );
    pad_source_border = new border_pad( "source_border" );
    pad_source_gsd = new gsd_pad( "source_gsd" );
    pad_source_timestamp = new timestamp_pad( "source_timestamp" );
    pad_source_features = new feature_array_pad( "source_array" );

    pad_output_features = new feature_array_pad( "output_array" );
    pad_output_var_image = new var_image_pad( "output_var_image" );


    proc_avg = new frame_averaging_process< OutputType >( "averager" );
    config.add_subblock( proc_avg->params(),
                         proc_avg->name() );

    proc_hp1 = new high_pass_filter_process< OutputType >( "high_pass_1" );
    config.add_subblock( proc_hp1->params(),
                         proc_hp1->name() );

    proc_hp2 = new high_pass_filter_process< OutputType >( "high_pass_2" );
    config.add_subblock( proc_hp2->params(),
                         proc_hp2->name() );

    proc_integral = new integral_image_process< OutputType, int >( "integrator" );
    config.add_subblock( proc_integral->params(),
                         proc_integral->name() );

    proc_canny = new aligned_edge_detection_process< OutputType >( "canny_edge" );
    config.add_subblock( proc_canny->params(),
                         proc_canny->name() );

    proc_color_convert = new convert_color_space_process< OutputType >( "color_convert" );
    config.add_subblock( proc_color_convert->params(),
                         proc_color_convert->name() );

    proc_cc1 = new color_commonality_filter_process< OutputType, OutputType >( "cc_filter_1" );
    config.add_subblock( proc_cc1->params(),
                         proc_cc1->name() );

    proc_cc2 = new color_commonality_filter_process< OutputType, OutputType >( "cc_filter_2" );
    config.add_subblock( proc_cc2->params(),
                         proc_cc2->name() );

    proc_kmeans = new kmeans_segmentation_process< OutputType, OutputType >( "kmeans_filter" );
    config.add_subblock( proc_kmeans->params(),
                         proc_kmeans->name() );

    proc_thresh = new threshold_image_process< OutputType >( "cc_thresholder" );
    config.add_subblock( proc_thresh->params(),
                         proc_thresh->name() );

    proc_blob_feat = new blob_pixel_feature_extraction_process< OutputType >( "cc_blob_features" );
    config.add_subblock( proc_blob_feat->params(),
                         proc_blob_feat->name() );

    proc_enhance = new video_enhancement_process< OutputType >( "enhancer" );
    config.add_subblock( proc_enhance->params(),
                         proc_enhance->name() );

    proc_merge = new image_consolidator_process< OutputType >( "merger" );
    config.add_subblock( proc_merge->params(),
                         proc_merge->name() );

    proc_hash1 = new floating_point_image_hash_process< double, OutputType >( "hasher1" );
    config.add_subblock( proc_hash1->params(),
                         proc_hash1->name() );

    // Over-riding the process default with this super-process defaults.
    default_config.add_parameter( proc_avg->name() + ":type", "window", "Default override" );
    default_config.add_parameter( proc_avg->name() + ":window_size", "25", "Default override" );
    default_config.add_parameter( proc_avg->name() + ":compute_variance", "true", "Default override" );
    default_config.add_parameter( proc_hp1->name() + ":mode", "BIDIR", "Default override" );
    default_config.add_parameter( proc_hp1->name() + ":box_kernel_width", "27", "Default override" );
    default_config.add_parameter( proc_hp1->name() + ":box_kernel_height", "27", "Default override" );
    default_config.add_parameter( proc_hp1->name() + ":box_interlaced", "false", "Default override" );
    default_config.add_parameter( proc_hp1->name() + ":output_net_only", "true", "Default override" );
    default_config.add_parameter( proc_hp2->name() + ":mode", "BOX_WORLD", "Default override" );
    default_config.add_parameter( proc_hp2->name() + ":box_kernel_dim_world", "13", "Default override" );
    default_config.add_parameter( proc_hp2->name() + ":min_kernel_dim_pixels", "5", "Default override" );
    default_config.add_parameter( proc_hp2->name() + ":max_kernel_dim_pixels", "201", "Default override" );
    default_config.add_parameter( proc_cc1->name() + ":smooth_image", "false", "Default override" );
    default_config.add_parameter( proc_cc1->name() + ":output_scale", "255", "Default override" );
    default_config.add_parameter( proc_cc2->name() + ":grid_image", "true", "Default override" );
    default_config.add_parameter( proc_cc2->name() + ":color_hist_resolution_per_chan", "16", "Default override" );
    default_config.add_parameter( proc_hash1->name() + ":scale_factor", "0.05", "Default override" );
    default_config.add_parameter( proc_hash1->name() + ":max_input_value", "5080", "Default override" );
    default_config.add_parameter( proc_kmeans->name() + ":sampling_points", "600", "Default override" );
    default_config.add_parameter( proc_kmeans->name() + ":clusters", "5", "Default override" );
    default_config.add_parameter( proc_thresh->name() + ":type", "percentiles", "Default override" );
    default_config.add_parameter( proc_thresh->name() + ":percentiles", "0.01,0.07,0.16", "Default override" );
    default_config.add_parameter( proc_color_convert->name() + ":output_color_space", "HSV", "Default override" );

    config.update( default_config );
  }

  template <class PIPELINE>
  void setup_pipeline( PIPELINE * p )
  {
    // Add input and output pads
    p->add( pad_source_border );
    p->add( pad_source_features );
    p->add( pad_output_features );

    if( grey_pad_required )
    {
      p->add( pad_source_grey_image );
    }

    if( color_pad_required )
    {
      p->add( pad_source_color_image );
    }

    if( gsd_pad_required )
    {
      p->add( pad_source_gsd );
    }

    // Add shared worker processes
    p->add( proc_merge );

    if( cc1_filter_required )
    {
      p->add( proc_cc1 );
    }

    // Connect required merge process ports
    p->connect( pad_source_features->value_port(),
                proc_merge->set_input_list_port() );
    p->connect( proc_merge->output_list_port(),
                pad_output_features->set_value_port() );

    // No need to go any further if disabled
    if( disabled )
    {
      return;
    }

    // A pointer to the port assumed to contain the color image, if one is
    // available. If it is not, fallback to the grayscale image.
    image_pad* color_image_source;

    if( single_channel_only )
    {
      color_image_source = pad_source_grey_image.ptr();
    }
    else
    {
      color_image_source = pad_source_color_image.ptr();
    }

    // Add feature specific processes
    if( feature_enable_flags[0] )
    {
      p->connect( color_image_source->value_port(),
                  proc_merge->set_image_1_port() );
    }
    if( feature_enable_flags[1] )
    {
      p->connect( pad_source_grey_image->value_port(),
                  proc_merge->set_image_2_port() );
    }
    if( feature_enable_flags[2] )
    {
      p->connect( color_image_source->value_port(),
                  proc_cc1->set_source_image_port() );
      p->connect( proc_cc1->filtered_image_port(),
                  proc_merge->set_image_3_port() );
    }
    if( feature_enable_flags[3] )
    {
      p->add( proc_cc2 );

      p->connect( color_image_source->value_port(),
                  proc_cc2->set_source_image_port() );
      p->connect( proc_cc2->filtered_image_port(),
                  proc_merge->set_image_4_port() );
    }
    if( feature_enable_flags[4] )
    {
      p->add( proc_thresh );
      p->add( proc_blob_feat );

      p->connect( proc_cc1->filtered_image_port(),
                  proc_thresh->set_source_image_port() );
      p->connect( proc_thresh->thresholded_image_port(),
                  proc_blob_feat->set_source_image_port() );
      p->connect( proc_blob_feat->feature_image_port(),
                  proc_merge->set_image_5_port() );
    }
    if( feature_enable_flags[5] )
    {
      p->add( proc_enhance );

      p->connect( color_image_source->value_port(),
                  proc_enhance->set_source_image_port() );
      p->connect( proc_enhance->copied_output_image_port(),
                  proc_merge->set_image_6_port() );
    }
    if( feature_enable_flags[6] )
    {
      p->add( proc_hp1 );

      p->connect( pad_source_gsd->value_port(),
                  proc_hp1->set_source_gsd_port() );
      p->connect( pad_source_grey_image->value_port(),
                  proc_hp1->set_source_image_port() );
      p->connect( pad_source_border->value_port(),
                  proc_hp1->set_source_border_port() );
      p->connect( proc_hp1->filtered_image_port(),
                  proc_merge->set_image_7_port() );
    }
    if( feature_enable_flags[7] )
    {
      p->add( proc_hp2 );

      p->connect( pad_source_gsd->value_port(),
                  proc_hp2->set_source_gsd_port() );
      p->connect( pad_source_grey_image->value_port(),
                  proc_hp2->set_source_image_port() );
      p->connect( pad_source_border->value_port(),
                  proc_hp2->set_source_border_port() );
      p->connect( proc_hp2->filtered_image_port(),
                  proc_merge->set_image_8_port() );

    }
    if( feature_enable_flags[8] )
    {
      p->add( proc_avg );
      p->add( proc_hash1 );
      p->add( pad_output_var_image );

      p->connect( pad_source_grey_image->value_port(),
                  proc_avg->set_source_image_port() );
      p->connect( proc_avg->variance_image_port(),
                  proc_hash1->set_input_image_port() );
      p->connect( proc_hash1->hashed_image_port(),
                  proc_merge->set_image_9_port() );
      p->connect( proc_avg->variance_image_port(),
                  pad_output_var_image->set_value_port() );
    }
    if( feature_enable_flags[9] )
    {
      p->add( proc_canny );

      p->connect( pad_source_grey_image->value_port(),
                  proc_canny->set_source_image_port() );
      p->connect( pad_source_border->value_port(),
                  proc_canny->set_source_border_port() );
      p->connect( proc_canny->combined_edge_image_port(),
                  proc_merge->set_image_10_port() );
    }
    if( feature_enable_flags[10] )
    {
      p->add( proc_kmeans );

      p->connect( pad_source_grey_image->value_port(),
                  proc_kmeans->set_source_image_port() );
      p->connect( pad_source_border->value_port(),
                  proc_kmeans->set_source_border_port() );
      p->connect( proc_kmeans->segmented_image_port(),
                  proc_merge->set_image_11_port() );
    }
    if( feature_enable_flags[11] )
    {
      p->add( proc_color_convert );

      p->connect( color_image_source->value_port(),
                  proc_color_convert->set_source_image_port() );
      p->connect( proc_color_convert->converted_image_port(),
                  proc_merge->set_image_12_port() );
    }
  }
};

template < class InputType, class OutputType >
pixel_feature_extractor_super_process<InputType,OutputType>
::pixel_feature_extractor_super_process( std::string const& _name )
  : super_process( _name, "pixel_feature_extractor_super_process" ),
    impl_( NULL )
{
  impl_ = new pixel_feature_extractor_super_process_impl<InputType,OutputType>;

  impl_->create_process_configs();

  // Settings related to which features should be enabled
  impl_->config.add_parameter(
    "enable_raw_color_image",
    "false",
    "Should we report the raw color image as a feature?" );
  impl_->config.add_parameter(
    "enable_raw_grey_image",
    "false",
    "Should we report the raw greyscale image as a feature?" );
  impl_->config.add_parameter(
    "enable_color_commonality_filter_1",
    "false",
    "Enable the first color commonality filter." );
  impl_->config.add_parameter(
    "enable_color_commonality_filter_2",
    "false",
    "Enable the second color commonality filter." );
  impl_->config.add_parameter(
    "enable_cc_blob_filter",
    "false",
    "Enable the color commmonality segmented blob feature filter." );
  impl_->config.add_parameter(
    "enable_color_normalization_filter",
    "false",
    "Enable the first color normalization filter." );
  impl_->config.add_parameter(
    "enable_high_pass_filter_1",
    "false",
    "Enable the first high pass filter." );
  impl_->config.add_parameter(
    "enable_high_pass_filter_2",
    "false",
    "Enable the second high pass filter." );
  impl_->config.add_parameter(
    "enable_variance_filter",
    "false",
    "Enable the per-pixel intensity variance filter." );
  impl_->config.add_parameter(
    "enable_edge_filter",
    "false",
    "Enable the edge detection filter." );
  impl_->config.add_parameter(
    "enable_kmeans_filter",
    "false",
    "Enable the kmeans segmentation commonality filter." );
  impl_->config.add_parameter(
    "enable_color_conversion_filter",
    "false",
    "Enable the alternative color space conversion filter." );
  impl_->config.add_parameter(
    "feature_code",
    "",
    "An alternative method to set which features are enabled, as a single "
    "variable as opposed to having to set multiple boolean expressions." );

  // Other settings
  impl_->config.add_parameter(
    "disabled",
    "false",
    "Should we force this process to not generate any outputs?" );
  impl_->config.add_parameter(
    "run_async",
    "true",
    "Whether or not to run asynchronously" );
  impl_->config.add_parameter(
    "single_channel_only",
    "false",
    "Force all filters to run on a single channel image instead of color" );

}

template < class InputType, class OutputType >
pixel_feature_extractor_super_process<InputType,OutputType>
::~pixel_feature_extractor_super_process()
{
  delete impl_;
}

template < class InputType, class OutputType >
config_block
pixel_feature_extractor_super_process<InputType,OutputType>
::params() const
{
  return impl_->config;
}

template < class InputType, class OutputType >
bool
pixel_feature_extractor_super_process<InputType,OutputType>
::set_params( config_block const& blk )
{
  try
  {
    // Load operating mode properties
    bool run_async = blk.get<bool>( "run_async" );
    impl_->single_channel_only = blk.get<bool>( "single_channel_only" );

    // Update internal params
    impl_->config.update( blk );

    // Create a feature code, which has a bit for each possible feature
    unsigned feature_code = 0x0000;

    std::string feature_code_str = blk.get<std::string>("feature_code");

    if( feature_code_str.empty() )
    {
      if( blk.get<bool>("enable_raw_color_image") )
        feature_code |= 0x0001;
      if( blk.get<bool>("enable_raw_grey_image") )
        feature_code |= 0x0002;
      if( blk.get<bool>("enable_color_commonality_filter_1") )
        feature_code |= 0x0004;
      if( blk.get<bool>("enable_color_commonality_filter_2") )
        feature_code |= 0x0008;
      if( blk.get<bool>("enable_cc_blob_filter") )
        feature_code |= 0x0010;
      if( blk.get<bool>("enable_color_normalization_filter") )
        feature_code |= 0x0020;
      if( blk.get<bool>("enable_high_pass_filter_1") )
        feature_code |= 0x0040;
      if( blk.get<bool>("enable_high_pass_filter_2") )
        feature_code |= 0x0080;
      if( blk.get<bool>("enable_variance_filter") )
        feature_code |= 0x0100;
      if( blk.get<bool>("enable_edge_filter") )
        feature_code |= 0x0200;
      if( blk.get<bool>("enable_kmeans_filter") )
        feature_code |= 0x0400;
      if( blk.get<bool>("enable_color_conversion_filter") )
        feature_code |= 0x0800;
    }
    else
    {
      feature_code = boost::lexical_cast<unsigned>( feature_code_str );
    }

    // If there are no features to extract or the flag is set, disable process
    impl_->disabled = ( feature_code == 0 || blk.get<bool>( "disabled" ) );

    if( !impl_->disabled )
    {
      impl_->color_pad_required = ( feature_code & 0x083D ) != 0;
      impl_->grey_pad_required = ( feature_code & 0x07C3 ) != 0;
      impl_->gsd_pad_required = ( feature_code & 0x00C0 ) != 0;
      impl_->cc1_filter_required = ( feature_code & 0x0014 ) != 0;

      for( unsigned i = 0; i < feature_count; ++i )
      {
        impl_->feature_enable_flags[i] = ( feature_code & ( 0x01 << i ) ) != 0;
      }
    }

    // Configure internal pipelines, run in sync mode if disabled
    if( run_async && !impl_->disabled )
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

    // Set parameters
    if( !pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error( "Unable to set pipeline parameters." );
    }
  }
  catch( const config_block_parse_error& e )
  {
    LOG_ERROR( this->name() << ": set_params failed: " << e.what() );
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
template < class InputType, class OutputType >
bool
pixel_feature_extractor_super_process<InputType,OutputType>
::initialize()
{
  return this->pipeline_->initialize();
}


// ----------------------------------------------------------------
template < class InputType, class OutputType >
process::step_status
pixel_feature_extractor_super_process<InputType,OutputType>
::step2()
{
  return this->pipeline_->execute();
}


// ----------------------------------------------------------------
template < class InputType, class OutputType >
void
pixel_feature_extractor_super_process<InputType,OutputType>
::set_source_color_image( vil_image_view< InputType > const& img )
{
  impl_->pad_source_color_image->set_value( img );
}

template < class InputType, class OutputType >
void
pixel_feature_extractor_super_process<InputType,OutputType>
::set_source_gsd( double gsd )
{
  impl_->pad_source_gsd->set_value( gsd );
}

template < class InputType, class OutputType >
void
pixel_feature_extractor_super_process<InputType,OutputType>
::set_source_grey_image( vil_image_view< InputType > const& img )
{
  impl_->pad_source_grey_image->set_value( img );
}

template< class InputType, class OutputType>
void
pixel_feature_extractor_super_process<InputType,OutputType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}

template < class InputType, class OutputType >
void
pixel_feature_extractor_super_process<InputType,OutputType>
::set_border( image_border const& border )
{
  impl_->pad_source_border->set_value( border );
}

template < class InputType, class OutputType >
void
pixel_feature_extractor_super_process<InputType,OutputType>
::set_auxiliary_features( feature_array_t const& features )
{
  impl_->pad_source_features->set_value( features );
}

template < class InputType, class OutputType >
std::vector< vil_image_view< OutputType > >
pixel_feature_extractor_super_process<InputType,OutputType>
::feature_array() const
{
  return impl_->pad_output_features->value();
}

template < class InputType, class OutputType >
vil_image_view< double >
pixel_feature_extractor_super_process<InputType,OutputType>
::var_image() const
{
  return impl_->pad_output_var_image->value();
}


} // end namespace vidtk
