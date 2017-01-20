/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline_framework/async_pipeline.h>
#include <pipeline_framework/sync_pipeline.h>

#include <object_detectors/diff_super_process.h>
#include <object_detectors/diff_buffer_process.h>
#include <object_detectors/diff_pass_thru_process.h>
#include <object_detectors/sg_background_model_process.h>
#include <object_detectors/three_frame_differencing_process.h>
#include <object_detectors/diff_pixel_feature_infusion_process.h>
#include <object_detectors/pixel_feature_extractor_super_process.h>

#ifdef USE_BRL
#include <object_detectors/gmm_background_model_process.h>
#endif

#include <utilities/config_block.h>
#include <utilities/transform_vidtk_homography_process.h>

#include <video_io/image_list_writer_process.h>
#include <video_io/image_list_writer_process.txx>

#include <video_transforms/world_smooth_image_process.h>
#include <video_transforms/clamp_image_pixel_values_process.h>
#include <video_transforms/mask_image_process.h>
#include <video_transforms/mask_merge_process.h>
#include <video_transforms/uncrop_image_process.h>
#include <video_transforms/greyscale_process.h>
#include <video_transforms/warp_image_process.h>
#include <video_transforms/video_enhancement_process.h>
#include <video_transforms/crop_image_process.h>
#include <video_transforms/deep_copy_image_process.h>
#include <video_transforms/threshold_image_process.h>
#include <video_transforms/floating_point_image_hash_process.h>

#include <video_properties/border_detection_process.h>

#include <tracking_data/image_object.h>

#include <boost/lexical_cast.hpp>

#include <sstream>
#include <vul/vul_awk.h>

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_diff_super_process_txx__
VIDTK_LOGGER("diff_super_process_txx");


namespace vidtk
{

template< class PixType>
class diff_super_process_impl
{
public:
  // General Typedefs
  typedef typename pixel_feature_array<PixType>::sptr_t feature_array_sptr;

  // Pad Typedefs
  typedef vidtk::super_process_pad_impl< vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl< double > gsd_pad;
  typedef vidtk::super_process_pad_impl< timestamp > timestamp_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > fg_img_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< float > > diff_img_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > mask_pad;
  typedef vidtk::super_process_pad_impl< vgl_h_matrix_2d< double > > vgl_2d_pad;
  typedef vidtk::super_process_pad_impl< image_to_utm_homography >   src2utm_pad;
  typedef vidtk::super_process_pad_impl< plane_to_utm_homography >   wld2utm_pad;
  typedef vidtk::super_process_pad_impl< plane_to_image_homography > wld2src_pad;
  typedef vidtk::super_process_pad_impl< image_to_plane_homography > src2wld_pad;
  typedef vidtk::super_process_pad_impl< image_to_image_homography > src2ref_pad;
  typedef vidtk::super_process_pad_impl< image_to_plane_homography > img2pln_pad;
  typedef vidtk::super_process_pad_impl< shot_break_flags > shot_break_flags_pad;
  typedef vidtk::super_process_pad_impl< video_modality > modality_pad;
  typedef vidtk::super_process_pad_impl< gui_frame_info > gui_feedback_pad;
  typedef vidtk::super_process_pad_impl< feature_array_sptr > feature_array_pad;

  // Configuration parameters
  config_block config;
  config_block default_config;
  config_block dependency_config;

  enum mod_mode_t { VIDTK_MOD_IMAGE_DIFF, VIDTK_MOD_GMM_BBGM, VIDTK_MOD_GMM_VIDTK };
  mod_mode_t mod_mode;

  enum infusion_mode_t { NONE, HEATMAP_ONLY, REPLACE, DUAL_OUTPUT };
  infusion_mode_t infusion_mode;

  bool run_async;
  bool masking_enabled;
  bool gui_feedback_enabled;
  bool compute_unprocessed_mask;
  bool disabled;
  bool video_enhancement_disabled;
  bool diff_out1_enabled;
  bool diff_out2_enabled;
  bool diff_out3_enabled;

  // Input Pads (dummy edge processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< src2utm_pad > pad_source_src2utm_homog;
  process_smart_pointer< wld2utm_pad > pad_source_wld2utm_homog;
  process_smart_pointer< wld2src_pad > pad_source_wld2src_homog;
  process_smart_pointer< src2wld_pad > pad_source_src2wld_homog;
  process_smart_pointer< src2ref_pad > pad_source_src2ref_homog;
  process_smart_pointer< gsd_pad > pad_source_gsd;
  process_smart_pointer< mask_pad > pad_source_mask;
  process_smart_pointer< shot_break_flags_pad > pad_source_shot_breaks;
  process_smart_pointer< img2pln_pad > pad_source_img2pln_homog;
  process_smart_pointer< modality_pad > pad_source_modality;
  process_smart_pointer< gui_feedback_pad > pad_source_gui_feedback;

  // Pipeline Processes
  process_smart_pointer< border_detection_process<PixType> > proc_border;
  process_smart_pointer< world_smooth_image_process<PixType> > proc_smooth_image;
  process_smart_pointer< clamp_image_pixel_values_process< PixType > > proc_clamper;
  process_smart_pointer< transform_image_homography_process > proc_trans_for_cropping;
  process_smart_pointer< three_frame_differencing_process<PixType,float> > proc_fg_process;
  process_smart_pointer< sg_background_model_process<PixType> > proc_gmm_process;
  process_smart_pointer< diff_buffer_process<PixType> > proc_cropped_buffer;
  process_smart_pointer< crop_image_process<PixType> > proc_image_crop;
  process_smart_pointer< warp_image_process<PixType> > proc_warp_1;
  process_smart_pointer< warp_image_process<PixType> > proc_warp_2;
  process_smart_pointer< mask_image_process > proc_mask_fg;
  process_smart_pointer< crop_image_process<bool> > proc_mask_crop;
  process_smart_pointer< mask_merge_process > proc_unprocessed_mask_merge;
  process_smart_pointer< image_list_writer_process<bool> > proc_unprocessed_mask_writer;
  process_smart_pointer< uncrop_image_process<bool> > proc_uncrop_unprocessed;
  process_smart_pointer< greyscale_process<PixType> > proc_greyscale;
  process_smart_pointer< diff_pass_thru_process<PixType> > proc_pass;
  process_smart_pointer< video_enhancement_process<PixType> > proc_video_enhancement;
  process_smart_pointer< pixel_feature_extractor_super_process<PixType,PixType> > proc_features;
  process_smart_pointer< diff_pixel_feature_infusion_process<PixType> > proc_infusion;
  process_smart_pointer< threshold_image_process<float> > proc_threshold;
  process_smart_pointer< floating_point_image_hash_process<float,vxl_byte> > proc_hash;
#ifdef USE_BRL
  process_smart_pointer< gmm_background_model_process<PixType> > proc_bbgm_process;
#endif

  // Debug Processes
  process_smart_pointer< image_list_writer_process<bool> > proc_diff_out1;
  process_smart_pointer< image_list_writer_process<float> > proc_diff_out2;
  process_smart_pointer< image_list_writer_process<PixType> > proc_diff_out3;

  // Output Pads (dummy edge processes)
  process_smart_pointer< image_pad > pad_output_image;
  process_smart_pointer< timestamp_pad > pad_output_timestamp;
  process_smart_pointer< gsd_pad > pad_output_gsd;
  process_smart_pointer< src2utm_pad > pad_output_src2utm_homog;
  process_smart_pointer< wld2utm_pad > pad_output_wld2utm_homog;
  process_smart_pointer< wld2src_pad > pad_output_wld2src_homog;
  process_smart_pointer< src2wld_pad > pad_output_src2wld_homog;
  process_smart_pointer< src2ref_pad > pad_output_src2ref_homog;
  process_smart_pointer< fg_img_pad > pad_output_fg_image;
  process_smart_pointer< diff_img_pad > pad_output_diff_image;
  process_smart_pointer< fg_img_pad > pad_output_fg2_image;
  process_smart_pointer< diff_img_pad > pad_output_diff2_image;
  process_smart_pointer< shot_break_flags_pad > pad_output_shot_breaks;
  process_smart_pointer< img2pln_pad > pad_output_img2pln_homog;
  process_smart_pointer< modality_pad > pad_output_modality;
  process_smart_pointer< gui_feedback_pad > pad_output_gui_feedback;
  process_smart_pointer< feature_array_pad > pad_output_feature_array;

  // Cached data for disabled mode
  vil_image_view< bool > disabled_fg_mask;
  vil_image_view< float > disabled_diff_image;

  int input_image_i_;
  int input_image_j_;

  // Default initializer
  diff_super_process_impl()
  : config(),
    default_config(),
    dependency_config(),
    mod_mode( VIDTK_MOD_IMAGE_DIFF ),
    infusion_mode( NONE ),
    masking_enabled( false ),
    gui_feedback_enabled( false ),
    compute_unprocessed_mask( false ),
    disabled( false ),
    video_enhancement_disabled( true ),
    diff_out1_enabled( false ),
    diff_out2_enabled( false ),
    diff_out3_enabled( false ),

    // Input Pads
    pad_source_image( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_src2utm_homog( NULL ),
    pad_source_wld2utm_homog( NULL ),
    pad_source_wld2src_homog( NULL ),
    pad_source_src2wld_homog( NULL ),
    pad_source_src2ref_homog( NULL ),
    pad_source_gsd( NULL ),
    pad_source_mask( NULL ),
    pad_source_shot_breaks( NULL ),
    pad_source_img2pln_homog( NULL ),
    pad_source_modality( NULL ),
    pad_source_gui_feedback( NULL ),

    // Processes
    proc_border( NULL ),
    proc_smooth_image( NULL ),
    proc_clamper( NULL ),
    proc_trans_for_cropping( NULL ),
    proc_fg_process( NULL ),
    proc_gmm_process( NULL ),
    proc_cropped_buffer( NULL ),
    proc_image_crop( NULL ),
    proc_warp_1( NULL ),
    proc_warp_2( NULL ),
    proc_mask_fg( NULL ),
    proc_mask_crop( NULL ),
    proc_unprocessed_mask_merge( NULL ),
    proc_unprocessed_mask_writer( NULL ),
    proc_uncrop_unprocessed( NULL ),
    proc_greyscale( NULL ),
    proc_pass( NULL ),
    proc_video_enhancement( NULL ),
    proc_features( NULL ),
    proc_infusion( NULL ),
    proc_threshold( NULL ),
    proc_hash( NULL ),
#ifdef USE_BRL
    proc_bbgm_process( NULL ),
#endif

    proc_diff_out1( NULL ),
    proc_diff_out2( NULL ),
    proc_diff_out3( NULL ),

    // Output Pads
    pad_output_image( NULL ),
    pad_output_timestamp( NULL ),
    pad_output_gsd( NULL ),
    pad_output_src2utm_homog( NULL ),
    pad_output_wld2utm_homog( NULL ),
    pad_output_wld2src_homog( NULL ),
    pad_output_src2wld_homog( NULL ),
    pad_output_src2ref_homog( NULL ),
    pad_output_fg_image( NULL ),
    pad_output_diff_image( NULL ),
    pad_output_shot_breaks( NULL ),
    pad_output_img2pln_homog( NULL ),
    pad_output_modality( NULL ),
    pad_output_gui_feedback( NULL ),

    input_image_i_(0),
    input_image_j_(0)
  {}

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    // Input Pads
    pad_source_image = new image_pad("source_image");
    pad_source_timestamp = new timestamp_pad("source_timestamp");
    pad_source_gsd = new gsd_pad("source_gsd");
    pad_source_src2utm_homog = new src2utm_pad("source_src2utm_H");
    pad_source_wld2utm_homog = new wld2utm_pad("source_wld2utm_H");
    pad_source_wld2src_homog = new wld2src_pad("source_wld2src_H");
    pad_source_src2wld_homog = new src2wld_pad("source_src2wld_H");
    pad_source_src2ref_homog = new src2ref_pad("source_src2ref_H");
    pad_source_mask = new mask_pad("source_mask");
    pad_source_shot_breaks = new shot_break_flags_pad("source_shot_breaks");
    pad_source_img2pln_homog = new img2pln_pad("source_img2pln_homog");
    pad_source_modality = new modality_pad("source_video_modality");
    pad_source_gui_feedback = new gui_feedback_pad("source_gui_feedback");

    // Output Pads
    pad_output_image = new image_pad("output_image");
    pad_output_timestamp = new timestamp_pad("output_timestamp");
    pad_output_gsd = new gsd_pad("output_gsd");
    pad_output_src2utm_homog = new src2utm_pad("output_src2utm_H");
    pad_output_wld2utm_homog = new wld2utm_pad("output_wld2utm_H");
    pad_output_wld2src_homog = new wld2src_pad("output_wld2src_H");
    pad_output_src2wld_homog = new src2wld_pad("output_src2wld_H");
    pad_output_src2ref_homog = new src2ref_pad("output_src2ref_H");
    pad_output_fg_image = new fg_img_pad("output_fg_img");
    pad_output_diff_image = new diff_img_pad("output_diff_img");
    pad_output_fg2_image = new fg_img_pad("output_fg_img");
    pad_output_diff2_image = new diff_img_pad("output_diff_img");
    pad_output_shot_breaks = new shot_break_flags_pad("output_shot_breaks");
    pad_output_img2pln_homog = new img2pln_pad("output_img2pln_homog");
    pad_output_modality = new modality_pad("output_video_modality");
    pad_output_gui_feedback = new gui_feedback_pad("output_gui_feedback");
    pad_output_feature_array = new feature_array_pad("output_feature_array");

    // Processes
    proc_border = new border_detection_process<PixType>( "border_detect" );
    config.add_subblock( proc_border->params(), proc_border->name() );

    proc_smooth_image = new world_smooth_image_process<PixType>( "smooth_image" );
    config.add_subblock( proc_smooth_image->params(), proc_smooth_image->name() );

    proc_clamper = new clamp_image_pixel_values_process<PixType>( "pixel_value_clamper" );
    config.add_subblock( proc_clamper->params(), proc_clamper->name() );

    proc_trans_for_cropping = new transform_image_homography_process( "trans_for_cropping" );
    config.add_subblock( proc_trans_for_cropping->params(), proc_trans_for_cropping->name() );

    proc_fg_process = new three_frame_differencing_process<PixType,float>( "image_diff" );
    config.add_subblock( proc_fg_process->params(), proc_fg_process->name() );

    proc_gmm_process = new sg_background_model_process<PixType>( "gmm" );
    config.add_subblock( proc_gmm_process->params(), proc_gmm_process->name() );

#ifdef USE_BRL
    proc_bbgm_process = new gmm_background_model_process<PixType>( "bbgm_gmm" );
    config.add_subblock( proc_bbgm_process->params(), proc_bbgm_process->name() );
#endif

    proc_video_enhancement = new video_enhancement_process<PixType>( "video_enhancement" );
    config.add_subblock( proc_video_enhancement->params(), proc_video_enhancement->name() );

    proc_diff_out1 = new image_list_writer_process<bool>( "diff_fg_out" );
    config.add_subblock( proc_diff_out1->params(), proc_diff_out1->name() );

    proc_diff_out2 = new image_list_writer_process<float>( "diff_float_out" );
    config.add_subblock( proc_diff_out2->params(), proc_diff_out2->name() );

    proc_diff_out3 = new image_list_writer_process<PixType>( "diff_input_out" );
    config.add_subblock( proc_diff_out3->params(), proc_diff_out3->name() );

    proc_cropped_buffer = new diff_buffer_process<PixType>( "cropped_image_buffer" );
    config.add_subblock( proc_cropped_buffer->params(), proc_cropped_buffer->name() );

    proc_image_crop = new crop_image_process<PixType>( "source_crop" );
    config.add_subblock( proc_image_crop->params(), proc_image_crop->name() );

    proc_mask_crop = new crop_image_process<bool>( "mask_crop" );
    config.add_subblock( proc_mask_crop->params(), proc_mask_crop->name() );

    proc_warp_1 = new warp_image_process<PixType>( "warp_image_1" );
    config.add_subblock( proc_warp_1->params(), proc_warp_1->name() );

    proc_warp_2 = new warp_image_process<PixType>( "warp_image_2" );
    config.add_subblock( proc_warp_2->params(), proc_warp_2->name() );

    proc_mask_fg = new mask_image_process( "mask_fg_image" );
    config.add_subblock( proc_mask_fg->params(), proc_mask_fg->name() );

    proc_unprocessed_mask_merge = new mask_merge_process( "unprocessed_mask_merge" );
    config.add_subblock( proc_unprocessed_mask_merge->params(),
                         proc_unprocessed_mask_merge->name() );

    proc_unprocessed_mask_writer = new image_list_writer_process<bool>( "unprocessed_mask_writer" );
    config.add_subblock( proc_unprocessed_mask_writer->params(),
                         proc_unprocessed_mask_writer->name() );

    proc_uncrop_unprocessed = new uncrop_image_process<bool>( "uncrop_unprocessed_mask" );
    config.add_subblock( proc_uncrop_unprocessed->params(),
                         proc_uncrop_unprocessed->name() );

    proc_greyscale = new greyscale_process<PixType>( "rgb_to_grey" );
    config.add_subblock( proc_greyscale->params(), proc_greyscale->name() );

    proc_features = new pixel_feature_extractor_super_process<PixType,vxl_byte>( "feature_extractor" );
    config.add_subblock( proc_features->params(), proc_features->name() );

    proc_infusion = new diff_pixel_feature_infusion_process<vxl_byte>( "feature_infuser" );
    config.add_subblock( proc_infusion->params(), proc_infusion->name() );

    proc_threshold = new threshold_image_process<float>( "infusion_threshold" );
    config.add_subblock( proc_threshold->params(), proc_threshold->name() );

    proc_hash = new floating_point_image_hash_process<float,vxl_byte>( "motion_hasher" );
    config.add_subblock( proc_hash->params(), proc_hash->name() );

    proc_pass = new diff_pass_thru_process<PixType>( "diff_pass_thru" );
    config.add_subblock( proc_pass->params(), proc_pass->name() );


    // Over-riding the process default with this super-process defaults.
    default_config.add_parameter( proc_diff_out1->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_diff_out2->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_diff_out3->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_video_enhancement->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_uncrop_unprocessed->name() + ":background_value", "true", "Default override" );
    default_config.add_parameter( proc_unprocessed_mask_writer->name() + ":disabled", "true", "Default override" );
    default_config.add_parameter( proc_hash->name() + ":scale_factor", "1.0", "Default override" );
    default_config.add_parameter( proc_hash->name() + ":max_input_value", "255", "Default override" );
    default_config.add_parameter( proc_border->name() + ":side_dilation", "0", "Default override" );
    default_config.add_parameter( proc_border->name() + ":max_border_width", "50", "Default override" );

    // Update config
    config.update( default_config );

    // Following default values will over-write the process defaults.
    dependency_config.add_parameter( proc_features->name() + ":run_async", "true", "Force override" );
    dependency_config.add_parameter( proc_cropped_buffer->name() + ":spacing", "2", "Force override" );
    dependency_config.add_parameter( proc_cropped_buffer->name() + ":check_shot_break_flags", "false", "Force override" );
    dependency_config.add_parameter( proc_mask_crop->name() + ":upper_left", "0 0", "Force override" );
    dependency_config.add_parameter( proc_mask_crop->name() + ":lower_right", "0 0", "Force override" );
    dependency_config.add_parameter( proc_infusion->name() + ":motion_image_crop", "0 0", "Force override" );
    dependency_config.add_parameter( proc_uncrop_unprocessed->name() + ":disabled", "false", "Force override" );
    dependency_config.add_parameter( proc_uncrop_unprocessed->name() + ":horizontal_padding", "0", "Force override" );
    dependency_config.add_parameter( proc_uncrop_unprocessed->name() + ":vertical_padding", "0", "Force override" );
    dependency_config.add_parameter( proc_fg_process->name() + ":compute_unprocessed_mask", "false", "Force override" );
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
    p->add( pad_source_wld2utm_homog );
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
    p->add( pad_output_wld2utm_homog );
    p->add( pad_output_wld2src_homog );
    p->add( pad_output_src2wld_homog );
    p->add( pad_output_src2ref_homog );
    p->add( pad_output_fg_image );
    p->add( pad_output_diff_image );
    p->add( pad_output_shot_breaks );
    p->add( pad_output_img2pln_homog );
    p->add( pad_output_modality );

    // If this super-process is disabled, we will pass along all the input
    // data as it is. In addition, we'll produce difference and foreground
    // images that represent no motion detection, i.e. zero-valued images.

    // NOTE: Currently we are forcing sync pipeline in disabled mode. Using
    // only for convenience of data flow as the I/O routines won't have to
    // change in sync/async modes.
    if( disabled )
    {
      p->connect( pad_source_image->value_port(),         pad_output_image->set_value_port() );
      p->connect( pad_source_gsd->value_port(),           pad_output_gsd->set_value_port() );
      p->connect( pad_source_timestamp->value_port(),     pad_output_timestamp->set_value_port() );
      p->connect( pad_source_wld2src_homog->value_port(), pad_output_wld2src_homog->set_value_port() );
      p->connect( pad_source_src2utm_homog->value_port(), pad_output_src2utm_homog->set_value_port() );
      p->connect( pad_source_wld2utm_homog->value_port(), pad_output_wld2utm_homog->set_value_port() );
      p->connect( pad_source_src2wld_homog->value_port(), pad_output_src2wld_homog->set_value_port() );
      p->connect( pad_source_src2ref_homog->value_port(), pad_output_src2ref_homog->set_value_port() );
      p->connect( pad_source_shot_breaks->value_port(),   pad_output_shot_breaks->set_value_port() );
      p->connect( pad_source_img2pln_homog->value_port(), pad_output_img2pln_homog->set_value_port() );
      p->connect( pad_source_modality->value_port(),      pad_output_modality->set_value_port() );

      // Following two calls will be made in step2()
      //    pad_output_fg_image->set_value()
      //    pad_output_diff_image->set_value()

      // We don't want to add any other processes in the disabled mode.
      return;
    }

    // Add Processes
    p->add( proc_image_crop );
    p->add( proc_smooth_image );
    p->add( proc_trans_for_cropping );
    p->add( proc_clamper );
    p->add( proc_mask_fg );
    p->add( proc_pass );

    // Add optional awb/illumination normalization process
    if( !video_enhancement_disabled )
    {
      p->add( proc_video_enhancement );
    }

    // Set up input connections, convert input to grayscale image if not
    // using the gaussian mixture model approach (for efficiency)
    if( mod_mode == VIDTK_MOD_IMAGE_DIFF )
    {
      p->add( proc_greyscale );
      p->connect( pad_source_image->value_port(),
                  proc_greyscale->set_image_port() );

      // If using awb connect greyscale -> enhance_process -> clamper
      if( video_enhancement_disabled )
      {
        p->connect( proc_greyscale->copied_image_port(),
                    proc_clamper->set_source_image_port() );
      }
      else
      {
        p->connect( proc_greyscale->copied_image_port(),
                    proc_video_enhancement->set_source_image_port() );
        p->connect( proc_video_enhancement->copied_output_image_port(),
                    proc_clamper->set_source_image_port() );
      }
    }
    else
    {
      // If using awb connect input pad -> enhance_process -> clamper
      if( video_enhancement_disabled )
      {
        p->connect( pad_source_image->value_port(),
                    proc_clamper->set_source_image_port() );
      }
      else
      {
        p->connect( pad_source_image->value_port(),
                    proc_video_enhancement->set_source_image_port() );
        p->connect( proc_video_enhancement->copied_output_image_port(),
                    proc_clamper->set_source_image_port() );
      }
    }

    // Input Pad Connections
    p->connect( pad_source_gsd->value_port(),
                proc_smooth_image->set_world_units_per_pixel_port() );
    p->connect( pad_source_src2ref_homog->value_port(),
                proc_trans_for_cropping->set_source_homography_port() );

    // Clamp/Smooth/Crop Connections
    p->connect( proc_clamper->image_port(),
                proc_smooth_image->set_source_image_port() );
    p->connect( proc_smooth_image->copied_image_port(),
                proc_image_crop->set_source_image_port() );

    // Setup input mask related connections if enabled
    if( masking_enabled )
    {
      p->add( pad_source_mask );
      p->add( proc_mask_crop );
      p->connect( pad_source_mask->value_port(),
                  proc_mask_crop->set_source_image_port() );
      p->connect( proc_mask_crop->cropped_image_port(),
                  proc_mask_fg->set_mask_image_port() );
    }

    // If using gaussian mixture model
    if( mod_mode == VIDTK_MOD_GMM_BBGM )
    {
#ifdef USE_BRL
      p->add( proc_bbgm_process );

      p->connect( proc_image_crop->cropped_image_port(),
                  proc_bbgm_process->set_source_image_port() );
      p->connect( proc_trans_for_cropping->bare_homography_port(),
                  proc_bbgm_process->set_homography_port() );
      p->connect( proc_bbgm_process->fg_image_port(),
                  proc_mask_fg->set_source_image_port() );
#else
      LOG_ERROR("mod_mod = gmm_bbgm, but BRL support not compiled in.");
#endif
    }
    else if( mod_mode == VIDTK_MOD_GMM_VIDTK )
    {
      p->add( proc_gmm_process );

      p->connect( proc_image_crop->cropped_image_port(),
                  proc_gmm_process->set_source_image_port() );
      p->connect( proc_trans_for_cropping->bare_homography_port(),
                  proc_gmm_process->set_homography_port() );
      p->connect( proc_gmm_process->fg_image_port(),
                  proc_mask_fg->set_source_image_port() );
    }
    else
    {
      p->add( proc_cropped_buffer );
      p->add( proc_warp_1 );
      p->add( proc_warp_2 );
      p->add( proc_fg_process);

      p->connect( pad_source_gsd->value_port(),
                  proc_cropped_buffer->add_gsd_port() );

      // Connect cropped images/homographies to buffer and warp
      p->connect( proc_image_crop->cropped_image_port(),
                  proc_cropped_buffer->set_source_img_port() );
      p->connect( proc_trans_for_cropping->bare_homography_port(),
                  proc_cropped_buffer->set_source_homog_port() );
      p->connect( proc_cropped_buffer->get_first_image_port(),
                  proc_warp_1->set_source_image_port() );
      p->connect( proc_cropped_buffer->get_third_to_first_homog_port(),
                  proc_warp_1->set_destination_to_source_homography_port() );
      p->connect( proc_cropped_buffer->get_second_image_port(),
                  proc_warp_2->set_source_image_port() );
      p->connect( proc_cropped_buffer->get_third_to_second_homog_port(),
                  proc_warp_2->set_destination_to_source_homography_port() );

      // Connect warp processes to image difference function
      p->connect( proc_cropped_buffer->get_differencing_flag_port(),
                  proc_fg_process->set_difference_flag_port() );
      p->connect( proc_cropped_buffer->get_third_image_port(),
                  proc_fg_process->set_third_frame_port() );
      p->connect( proc_warp_1->warped_image_port(),
                  proc_fg_process->set_first_frame_port() );
      p->connect( proc_warp_2->warped_image_port(),
                  proc_fg_process->set_second_frame_port() );

      // Connect image differencing to fg mask calculator
      p->connect( proc_fg_process->fg_image_port(),
                  proc_mask_fg->set_source_image_port() );

      // Unprocessed mask is not used for anything, only written to file
      if( compute_unprocessed_mask )
      {
        // Todo: Merge 2 masks from both warp image processes with mask
        // received from stab_sb if the output mask is ever going to be
        // used for anything downstream

        // Uncrop and write mask to file
        p->add( proc_uncrop_unprocessed );
        p->connect( proc_mask_crop->cropped_image_port(),
                    proc_uncrop_unprocessed->set_source_image_port() );

        p->add( proc_unprocessed_mask_writer );
        p->connect( proc_uncrop_unprocessed->uncropped_image_port(),
                  proc_unprocessed_mask_writer->set_image_port() );
      }

      // Debug processes - do not add if not enabled because if they fail
      // they will back up all input queues in async mode
      if( diff_out2_enabled )
      {
        p->add( proc_diff_out2 );
        p->connect( pad_source_timestamp->value_port(),
                    proc_diff_out2->set_timestamp_port() );
        p->connect( proc_fg_process->diff_image_port(),
                    proc_diff_out2->set_image_port() );
      }
    } // VIDTK_MOD_IMAGE_DIFF

    // Setup GUI feedback related connections if enabled
    if( gui_feedback_enabled )
    {
      p->add( pad_source_gui_feedback );
      p->connect( pad_source_gui_feedback->value_port(), proc_pass->set_input_gui_feedback_port() );

      if( mod_mode != VIDTK_MOD_GMM_VIDTK )
      {
        p->connect( pad_source_gui_feedback->value_port(), proc_cropped_buffer->add_source_gui_feedback_port() );
        p->connect( pad_source_shot_breaks->value_port(),  proc_cropped_buffer->add_shot_break_flags_port() );
      }
    }

    // Add additional debug output process for fg mask if enabled
    if( diff_out1_enabled )
    {
        p->add( proc_diff_out1 );
        p->connect( pad_source_timestamp->value_port(), proc_diff_out1->set_timestamp_port() );
        p->connect( proc_pass->get_output_fg_port(),    proc_diff_out1->set_image_port() );
    }

    // Add additional debug output process for fg mask if enabled
    if( diff_out3_enabled )
    {
        p->add( proc_diff_out3 );
        p->connect( pad_source_timestamp->value_port(), proc_diff_out3->set_timestamp_port() );
        p->connect( proc_pass->get_output_image_port(), proc_diff_out3->set_image_port() );
    }

    // Connect pixel feature infusion process if enabled, otherwise simply connect mask
    if( infusion_mode == NONE )
    {
      p->connect( proc_mask_fg->copied_image_port(),
                  proc_pass->set_input_fg_port() );

      if( mod_mode == VIDTK_MOD_IMAGE_DIFF )
      {
        p->connect( proc_fg_process->diff_image_port(),
                    proc_pass->set_input_diff_port() );
      }
      else if (mod_mode == VIDTK_MOD_GMM_VIDTK)
      {
        p->connect( proc_gmm_process->diff_image_port(),
                    proc_pass->set_input_diff_port());
       }
    }
    else
    {
      // Pixel feature infusion is enabled
      p->add( pad_output_feature_array );

      p->add( proc_border );
      p->add( proc_features );
      p->add( proc_infusion );
      p->add( proc_hash );

      p->connect( pad_source_image->value_port(),             proc_border->set_source_color_image_port() );
      p->connect( proc_border->border_port(),                 proc_features->set_border_port() );
      p->connect( proc_border->border_port(),                 proc_infusion->set_border_port() );

      p->connect( proc_clamper->image_port(),                 proc_features->set_source_grey_image_port() );
      p->connect( pad_source_image->value_port(),             proc_features->set_source_color_image_port() );
      p->connect( pad_source_gsd->value_port(),               proc_features->set_source_gsd_port() );
      p->connect( proc_fg_process->diff_image_port(),         proc_hash->set_input_image_port() );

      p->connect( proc_hash->hashed_image_port(),             proc_infusion->set_diff_image_port() );
      p->connect( proc_features->feature_array_port(),        proc_infusion->set_input_features_port() );
      p->connect( proc_mask_fg->copied_image_port(),          proc_infusion->set_diff_mask_port() );

      p->connect( proc_infusion->feature_array_port(),        proc_pass->set_input_feature_array_port() );
      p->connect( proc_pass->get_output_feature_array_port(), pad_output_feature_array->set_value_port() );

      p->connect( proc_infusion->classified_image_port(),     proc_pass->set_input_diff_port() );

      if( infusion_mode != HEATMAP_ONLY )
      {
        p->add( proc_threshold );
        p->connect( proc_infusion->classified_image_port(),
                    proc_threshold->set_source_image_port() );
        p->connect( proc_threshold->thresholded_image_port(),
                    proc_pass->set_input_fg_port() );
      }
      else if( infusion_mode == HEATMAP_ONLY )
      {
        p->connect( proc_mask_fg->copied_image_port(),
                    proc_pass->set_input_fg_port() );
      }

      if( infusion_mode == DUAL_OUTPUT )
      {
        p->add( pad_output_fg2_image );
        p->add( pad_output_diff2_image );

        p->connect( proc_mask_fg->copied_image_port(),
                    proc_pass->set_input_fg2_port() );
        p->connect( proc_fg_process->diff_image_port(),
                    proc_pass->set_input_diff2_port() );

        p->connect( proc_pass->get_output_fg2_port(),
                    pad_output_fg2_image->set_value_port() );
        p->connect( proc_pass->get_output_diff2_port(),
                    pad_output_diff2_image->set_value_port() );
      }

      if( gui_feedback_enabled )
      {
        p->connect( pad_source_gui_feedback->value_port(),
                    proc_infusion->set_gui_feedback_port() );
      }
    }

    // Connect all outputs to dummy pass thru process to detect failures
    p->connect( pad_source_image->value_port(),         proc_pass->set_input_image_port() );
    p->connect( pad_source_gsd->value_port(),           proc_pass->set_input_gsd_port() );
    p->connect( pad_source_timestamp->value_port(),     proc_pass->set_input_timestamp_port() );
    p->connect( pad_source_wld2src_homog->value_port(), proc_pass->set_input_wld_to_src_homography_port() );
    p->connect( pad_source_src2utm_homog->value_port(), proc_pass->set_input_src_to_utm_homography_port() );
    p->connect( pad_source_wld2utm_homog->value_port(), proc_pass->set_input_wld_to_utm_homography_port() );
    p->connect( pad_source_src2wld_homog->value_port(), proc_pass->set_input_src_to_wld_homography_port() );
    p->connect( pad_source_src2ref_homog->value_port(), proc_pass->set_input_src_to_ref_homography_port() );
    p->connect( pad_source_shot_breaks->value_port(),   proc_pass->set_input_shot_break_flags_port() );
    p->connect( pad_source_img2pln_homog->value_port(), proc_pass->set_input_ref_to_wld_homography_port() );
    p->connect( pad_source_modality->value_port(),      proc_pass->set_input_video_modality_port() );


    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),                 pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_gsd_port(),                   pad_output_gsd->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),             pad_output_timestamp->set_value_port() );
    p->connect( proc_pass->get_output_wld_to_src_homography_port(), pad_output_wld2src_homog->set_value_port() );
    p->connect( proc_pass->get_output_src_to_utm_homography_port(), pad_output_src2utm_homog->set_value_port() );
    p->connect( proc_pass->get_output_wld_to_utm_homography_port(), pad_output_wld2utm_homog->set_value_port() );
    p->connect( proc_pass->get_output_src_to_wld_homography_port(), pad_output_src2wld_homog->set_value_port() );
    p->connect( proc_pass->get_output_src_to_ref_homography_port(), pad_output_src2ref_homog->set_value_port() );
    p->connect( proc_pass->get_output_shot_break_flags_port(),      pad_output_shot_breaks->set_value_port() );
    p->connect( proc_pass->get_output_ref_to_wld_homography_port(), pad_output_img2pln_homog->set_value_port() );
    p->connect( proc_pass->get_output_video_modality_port(),        pad_output_modality->set_value_port() );
    p->connect( proc_pass->get_output_fg_port(),                    pad_output_fg_image->set_value_port() );
    p->connect( proc_pass->get_output_diff_port(),                  pad_output_diff_image->set_value_port() );

    if( gui_feedback_enabled )
    {
      p->add( pad_output_gui_feedback );
      p->connect( proc_pass->get_output_gui_feedback_port(),        pad_output_gui_feedback->set_value_port() );
    }
  } // setup_pipeline()


  // Generate blank frames to be used when disabled and when there is no valid stabilization.
  vil_image_view< bool > const&
  get_disabled_fg_mask()
  {
    if( !disabled_fg_mask )
    {
      disabled_fg_mask.set_size( input_image_i_, input_image_j_ );
      disabled_fg_mask.fill( false );
    }

    return disabled_fg_mask;
  }


  vil_image_view< float > const&
  get_disabled_diff_image()
  {
    if( !disabled_diff_image )
    {
      disabled_diff_image.set_size( input_image_i_, input_image_j_ );
      disabled_diff_image.fill( 0.0 );
    }

    return disabled_diff_image;
  }

};

template< class PixType >
diff_super_process<PixType>
::diff_super_process( std::string const& _name )
  : super_process( _name, "diff_super_process" ),
    impl_( NULL )
{
  impl_ = new diff_super_process_impl<PixType>;

  impl_->create_process_configs();

  impl_->config.add_parameter( "mod_mode",
    "image_diff",
    "Moving Object Detection Mode"
    "Takes one of the following string values: \n"
    "  image_diff -- Three frame differencing\n"
    "  gmm_bbgm   -- GMM using the bbgm in VXL contrib BRL\n"
    "  gmm_vidtk  -- GMM using vidtk implementation" );

  impl_->config.add_parameter( "image_diff_spacing",
    "2",
    "Temporal gap (number of frames) used in "
    "image_differenc_process2::spacing." );

  impl_->config.add_parameter( "run_async",
    "true",
    "Whether or not to run process asynchronously" );

  impl_->config.add_parameter( "pipeline_edge_capacity",
    boost::lexical_cast<std::string>(10 / sizeof(PixType)),
    "Maximum size of the edge in an async pipeline." );

  impl_->config.add_parameter( "masking_enabled",
    "false",
    "Use the input mask provided from prior process (stab_sp)." );

  impl_->config.add_parameter( "gui_feedback_enabled",
    "false",
    "Should we enable GUI feedback in the pipeline?" );

  impl_->config.add_parameter( "compute_unprocessed_mask",
    "false",
    "Change the pipeline and produce a mask representing pixels not "
    " considered for moving foreground detection." );

  impl_->config.add_parameter( "disabled",
    "false",
    "If disabled, this super process will pass along all the input data"
    " and will also produce *empty* difference and foreground images." );

  impl_->config.add_parameter( "pixel_feature_infusion",
    "none",
    "Instead of basing our foreground image only based on motion features, "
    "should we include basic appearance features combined with motion. Can "
    "either be none, heatmap_only, replace, or dual_output for no infusion, "
    "using features for the output heatmap only, for replacing both the original "
    "motion heatmap and mask, or for outputting them on new ports respectively." );
}

template< class PixType >
diff_super_process<PixType>
::~diff_super_process()
{
  delete impl_;
}

template< class PixType >
config_block
diff_super_process<PixType>
::params() const
{
  config_block tmp_config;
  tmp_config = impl_->config;
  tmp_config.remove( impl_->dependency_config );
  return tmp_config;
}

template< class PixType >
bool
diff_super_process<PixType>
::set_params( config_block const& blk )
{
  try
  {
    impl_->config.update( blk );

    // Get parameters for this process
    impl_->run_async = blk.get<bool>( "run_async" );
    impl_->masking_enabled = blk.get<bool>( "masking_enabled" );
    impl_->disabled = blk.get<bool>( "disabled" );
    impl_->gui_feedback_enabled = blk.get<bool>( "gui_feedback_enabled" );

    config_enum_option op_mode;
    op_mode.add( "image_diff", diff_super_process_impl<PixType>::VIDTK_MOD_IMAGE_DIFF )
           .add( "gmm_bbgm",   diff_super_process_impl<PixType>::VIDTK_MOD_GMM_BBGM )
           .add( "gmm_vidtk",  diff_super_process_impl<PixType>::VIDTK_MOD_GMM_VIDTK )
      ;

    impl_->mod_mode = blk.get< typename diff_super_process_impl<PixType>::mod_mode_t >( op_mode, "mod_mode" );


#ifndef USE_BRL
    if ( diff_super_process_impl<PixType>::VIDTK_MOD_GMM_BBGM == impl_->mod_mode )
    {
      throw config_block_parse_error("mod_mod = gmm_bbgm, but BRL support not compiled in.");
    }
#endif

    config_enum_option opt_infusion;
    opt_infusion.add( "none",         diff_super_process_impl<PixType>::NONE )
                .add( "heatmap_only", diff_super_process_impl<PixType>::HEATMAP_ONLY )
                .add( "replace",      diff_super_process_impl<PixType>::REPLACE )
                .add( "dual_output",  diff_super_process_impl<PixType>::DUAL_OUTPUT )
      ;

    impl_->infusion_mode = blk.get< typename diff_super_process_impl<PixType>::infusion_mode_t >(
      opt_infusion, "pixel_feature_infusion" );

    // Set dependency config params for image diff spacing
    if( impl_->mod_mode == diff_super_process_impl<PixType>::VIDTK_MOD_IMAGE_DIFF )
    {
      unsigned diff_spacing;
      diff_spacing = blk.get<unsigned>( "image_diff_spacing" );
      impl_->dependency_config.set( impl_->proc_cropped_buffer->name()
                                    + ":spacing", diff_spacing );
    }

    // Set dependency config params for run_async config
    if( !impl_->run_async )
    {
      impl_->dependency_config.set( impl_->proc_features->name() + ":run_async",
                                    impl_->run_async );
    }

    // Set dependency config params for gui signal processing mode
    if( impl_->gui_feedback_enabled )
    {
      impl_->dependency_config.set( impl_->proc_cropped_buffer->name()
                                    + ":check_shot_break_flags", true );
    }

    // Set dependency config params of crop size dependencies
    std::string ul = blk.get<std::string>( impl_->proc_image_crop->name() + ":upper_left" );
    std::string lr = blk.get<std::string>( impl_->proc_image_crop->name() + ":lower_right" );

    impl_->dependency_config.set( impl_->proc_mask_crop->name() + ":upper_left", ul );
    impl_->dependency_config.set( impl_->proc_mask_crop->name() + ":lower_right", lr );
    impl_->dependency_config.set( impl_->proc_infusion->name() + ":motion_image_crop", ul );

    std::stringstream ss;
    ss << ul;
    vul_awk awk( ss );

    impl_->dependency_config.set( impl_->proc_uncrop_unprocessed->name() + ":horizontal_padding", awk[0] );
    impl_->dependency_config.set( impl_->proc_uncrop_unprocessed->name() + ":vertical_padding",  awk[1] );

    // Set dependency config params of compute_unprocessed_mask
    impl_->compute_unprocessed_mask = blk.get<bool>( "compute_unprocessed_mask" );

    impl_->dependency_config.set( impl_->proc_fg_process->name() + ":compute_unprocessed_mask",
                                  impl_->compute_unprocessed_mask );
    impl_->dependency_config.set( impl_->proc_uncrop_unprocessed->name() + ":disabled",
                                  !impl_->compute_unprocessed_mask );

    if( !impl_->compute_unprocessed_mask )
    {
      // If we are not going to be computing the unprocessed mask then we
      // want to force the corresponding file writer to be disabled.
      impl_->config.set( impl_->proc_unprocessed_mask_writer->name() + ":disabled", true );
    }
    impl_->config.update( impl_->dependency_config );

    // Determine whether or not auto-white balance / illumination norm proc is in the pipeline
    impl_->video_enhancement_disabled = blk.get<bool>( impl_->proc_video_enhancement->name() + ":disabled" );

    // Determine whether or not debug output processes are enabled
    impl_->diff_out1_enabled = !blk.get<bool>( impl_->proc_diff_out1->name() + ":disabled" );
    impl_->diff_out2_enabled = !blk.get<bool>( impl_->proc_diff_out2->name() + ":disabled" );
    impl_->diff_out3_enabled = !blk.get<bool>( impl_->proc_diff_out3->name() + ":disabled" );

    // Configure asynch or synch pipeline
    // NOTE: Currently we are forcing sync pipeline in disabled mode.
    if( impl_->run_async && !impl_->disabled )
    {
      unsigned edge_capacity = blk.get<unsigned>( "pipeline_edge_capacity" );
      LOG_INFO("Starting async diff_super_process" );
      async_pipeline* p = new async_pipeline(async_pipeline::ENABLE_STATUS_FORWARDING, edge_capacity);

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }
    else
    {
      LOG_INFO("Starting sync diff_super_process" );
      sync_pipeline* p = new sync_pipeline;

      impl_->setup_pipeline( p );
      this->pipeline_ = p;
    }

    if( !pipeline_->set_params( impl_->config ) )
    {
      throw config_block_parse_error( " unable to set pipeline parameters." );
    }
  }
  catch( config_block_parse_error const & e)
  {
    LOG_ERROR( this->name() << ": couldn't set parameters: "<< e.what() );
    return false;
  }

  return true;
}

template< class PixType >
bool
diff_super_process<PixType>
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
diff_super_process<PixType>
::step2()
{
  if( impl_->disabled )
  {
    impl_->pad_output_fg_image->set_value( impl_->get_disabled_fg_mask() );
    impl_->pad_output_diff_image->set_value( impl_->get_disabled_diff_image() );
  }

  return pipeline_->execute();
}


// ================================================================
// Process Input methods

template< class PixType >
void diff_super_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  impl_->pad_source_image->set_value( img );

  // Assuming the image size does not change
  if ( impl_->input_image_i_ <= 0 ) { impl_->input_image_i_ = img.ni(); }
  if ( impl_->input_image_j_ <= 0 ) { impl_->input_image_j_ = img.nj(); }
}

template< class PixType >
void diff_super_process<PixType>
::set_source_timestamp( timestamp const& ts )
{
  impl_->pad_source_timestamp->set_value( ts );
}

template< class PixType >
void diff_super_process<PixType>
::set_world_units_per_pixel( double const& gsd )
{
  impl_->pad_source_gsd->set_value( gsd );
}

template< class PixType >
void diff_super_process<PixType>
::set_mask( vil_image_view<bool> const& mask )
{
  impl_->pad_source_mask->set_value( mask );
}

template< class PixType >
void diff_super_process<PixType>
::set_src_to_ref_homography( image_to_image_homography const& H )
{
  impl_->pad_source_src2ref_homog->set_value( H );
}

template< class PixType >
void diff_super_process<PixType>
::set_src_to_wld_homography( image_to_plane_homography const& H )
{
  impl_->pad_source_src2wld_homog->set_value( H );
}

template< class PixType >
void diff_super_process<PixType>
::set_wld_to_src_homography( plane_to_image_homography const& H )
{
  impl_->pad_source_wld2src_homog->set_value( H );
}

template< class PixType >
void diff_super_process<PixType>
::set_src_to_utm_homography( image_to_utm_homography const& H )
{
  impl_->pad_source_src2utm_homog->set_value( H );
}

template< class PixType >
void diff_super_process<PixType>
::set_wld_to_utm_homography( plane_to_utm_homography const& H )
{
  impl_->pad_source_wld2utm_homog->set_value( H );
}

template< class PixType >
void diff_super_process<PixType>
::set_input_ref_to_wld_homography( image_to_plane_homography const& H )
{
  impl_->pad_source_img2pln_homog->set_value( H );
}

template< class PixType >
void diff_super_process<PixType>
::set_input_video_modality( video_modality const& vm )
{
  impl_->pad_source_modality->set_value( vm );
}

template< class PixType >
void diff_super_process<PixType>
::set_input_shot_break_flags( shot_break_flags const& sb )
{
  impl_->pad_source_shot_breaks->set_value( sb );
}

template< class PixType >
void diff_super_process<PixType>
::set_input_gui_feedback( gui_frame_info const& gfi )
{
  impl_->pad_source_gui_feedback->set_value( gfi );
}

// ================================================================
// Process output methods

template< class PixType >
vil_image_view<bool>
diff_super_process<PixType>
::fg_out() const
{
  if ( ! impl_->pad_output_src2ref_homog->value().is_valid() )
  {
    impl_->pad_output_fg_image->set_value(impl_->get_disabled_fg_mask() );
  }
  return impl_->pad_output_fg_image->value();
}

template< class PixType >
vil_image_view<float>
diff_super_process<PixType>
::diff_out() const
{
  if ( ! impl_->pad_output_src2ref_homog->value().is_valid() )
  {
    impl_->pad_output_diff_image->set_value( impl_->get_disabled_diff_image() );
  }
  return impl_->pad_output_diff_image->value();
}

template< class PixType >
vil_image_view<bool>
diff_super_process<PixType>
::copied_fg_out() const
{
  if ( ! impl_->pad_output_src2ref_homog->value().is_valid() )
  {
    impl_->pad_output_fg_image->set_value( impl_->get_disabled_fg_mask() );
  }
  return impl_->pad_output_fg_image->value();
}

template< class PixType >
vil_image_view<float>
diff_super_process<PixType>
::copied_diff_out() const
{
  if ( ! impl_->pad_output_src2ref_homog->value().is_valid() )
  {
    impl_->pad_output_diff_image->set_value( impl_->get_disabled_diff_image() );
  }
  return impl_->pad_output_diff_image->value();
}

template< class PixType >
vil_image_view<bool>
diff_super_process<PixType>
::fg2_out() const
{
  return impl_->pad_output_fg2_image->value();
}

template< class PixType >
vil_image_view<float>
diff_super_process<PixType>
::diff2_out() const
{
  return impl_->pad_output_diff2_image->value();
}

template< class PixType >
double
diff_super_process<PixType>
::world_units_per_pixel() const
{
  return impl_->pad_output_gsd->value();
}

template< class PixType >
timestamp
diff_super_process<PixType>
::source_timestamp() const
{
  return impl_->pad_output_timestamp->value();
}

template< class PixType >
vil_image_view< PixType >
diff_super_process<PixType>
::source_image() const
{
  return impl_->pad_output_image->value();
}

template< class PixType >
image_to_image_homography
diff_super_process<PixType>
::src_to_ref_homography() const
{
  return impl_->pad_output_src2ref_homog->value();
}

template< class PixType >
image_to_plane_homography
diff_super_process<PixType>
::src_to_wld_homography() const
{
  return impl_->pad_output_src2wld_homog->value();
}

template< class PixType >
plane_to_image_homography
diff_super_process<PixType>
::wld_to_src_homography() const
{
  return impl_->pad_output_wld2src_homog->value();
}

template< class PixType >
image_to_utm_homography
diff_super_process<PixType>
::src_to_utm_homography() const
{
  return impl_->pad_output_src2utm_homog->value();
}

template< class PixType >
plane_to_utm_homography
diff_super_process<PixType>
::wld_to_utm_homography() const
{
  return impl_->pad_output_wld2utm_homog->value();
}

template< class PixType >
image_to_plane_homography
diff_super_process<PixType>
::get_output_ref_to_wld_homography() const
{
  return impl_->pad_output_img2pln_homog->value();
}

template< class PixType >
video_modality
diff_super_process<PixType>
::get_output_video_modality() const
{
  return impl_->pad_output_modality->value();
}

template< class PixType >
shot_break_flags
diff_super_process<PixType>
::get_output_shot_break_flags() const
{
  return impl_->pad_output_shot_breaks->value();
}

template< class PixType >
gui_frame_info
diff_super_process<PixType>
::get_output_gui_feedback() const
{
  return impl_->pad_output_gui_feedback->value();
}

template< class PixType >
typename pixel_feature_array<PixType>::sptr_t
diff_super_process<PixType>
::get_output_feature_array() const
{
  return impl_->pad_output_feature_array->value();
}

} // end namespace vidtk
