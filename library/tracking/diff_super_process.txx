/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <pipeline/async_pipeline.h>
#include <pipeline/sync_pipeline.h>

#include <tracking/diff_super_process.h>
#include <tracking/diff_buffer_process.h>
#include <tracking/diff_pass_thru_process.h>

#include <utilities/config_block.h>
#include <utilities/transform_homography_process.h>
#include <utilities/ring_buffer_process.h>

#include <tracking/image_object.h>
#include <tracking/world_smooth_image_process.h>
#include <tracking/sg_background_model_process.h>
#include <tracking/track_writer_process.h>
#include <tracking/image_difference_process2.h>

#include <video/crop_image_process.h>
#include <video/deep_copy_image_process.h>
#include <video/image_list_writer_process.h>
#include <video/image_list_writer_process.txx>
#include <video/clamp_image_pixel_values_process.h>
#include <video/mask_image_process.h>
#include <video/mask_merge_process.h>
#include <video/uncrop_image_process.h>
#include <video/greyscale_process.h>
#include <video/warp_image_process.h>

#include <vcl_sstream.h>
#include <vul/vul_awk.h>

namespace vidtk
{

template< class PixType>
class diff_super_process_impl
{
public:
  // Pad Typedefs
  typedef vidtk::super_process_pad_impl< vil_image_view< PixType > > image_pad;
  typedef vidtk::super_process_pad_impl< double > gsd_pad;
  typedef vidtk::super_process_pad_impl< timestamp > timestamp_pad;
  typedef vidtk::super_process_pad_impl< timestamp::vector_t > timestamp_vec_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > fg_img_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< float > > diff_img_pad;
  typedef vidtk::super_process_pad_impl< vil_image_view< bool > > mask_pad;
  typedef vidtk::super_process_pad_impl< vgl_h_matrix_2d< double > > vgl_2d_pad;
  typedef vidtk::super_process_pad_impl< plane_to_utm_homography > wld2utm_pad;
  typedef vidtk::super_process_pad_impl< plane_to_image_homography > wld2src_pad;
  typedef vidtk::super_process_pad_impl< image_to_plane_homography > src2wld_pad;
  typedef vidtk::super_process_pad_impl< image_to_image_homography > src2ref_pad;
  typedef vidtk::super_process_pad_impl< image_to_plane_homography > img2pln_pad;
  typedef vidtk::super_process_pad_impl< shot_break_flags > shot_break_flags_pad;
  typedef vidtk::super_process_pad_impl< video_modality > modality_pad;

  // Pipeline Processes
  process_smart_pointer< world_smooth_image_process<PixType> > proc_smooth_image;
  process_smart_pointer< clamp_image_pixel_values_process< PixType > > proc_clamper;
  process_smart_pointer< transform_homography_process > proc_trans_for_cropping;
  process_smart_pointer< image_difference_process2<PixType> > proc_fg_process;
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

  // Debug Processes
  process_smart_pointer< image_list_writer_process<bool> > proc_diff_out1;
  process_smart_pointer< image_list_writer_process<float> > proc_diff_out2;
  process_smart_pointer< image_list_writer_process<bool> > proc_diff_out3;
  process_smart_pointer< image_list_writer_process<float> > proc_diff_out4;

  // Input Pads (dummy edge processes)
  process_smart_pointer< image_pad > pad_source_image;
  process_smart_pointer< timestamp_pad > pad_source_timestamp;
  process_smart_pointer< wld2utm_pad > pad_source_wld2utm_homog;
  process_smart_pointer< wld2src_pad > pad_source_wld2src_homog;
  process_smart_pointer< src2wld_pad > pad_source_src2wld_homog;
  process_smart_pointer< src2ref_pad > pad_source_src2ref_homog;
  process_smart_pointer< vgl_2d_pad > pad_source_src2ref_transform;
  process_smart_pointer< gsd_pad > pad_source_gsd;
  process_smart_pointer< mask_pad > pad_source_mask;
  process_smart_pointer< timestamp_vec_pad > pad_source_time_vec;
  process_smart_pointer< shot_break_flags_pad > pad_source_shot_breaks;
  process_smart_pointer< img2pln_pad > pad_source_img2pln_homog;
  process_smart_pointer< modality_pad > pad_source_modality;

  // Output Pads (dummy edge processes)
  process_smart_pointer< image_pad > pad_output_image;
  process_smart_pointer< timestamp_pad > pad_output_timestamp;
  process_smart_pointer< gsd_pad > pad_output_gsd;
  process_smart_pointer< wld2utm_pad > pad_output_wld2utm_homog;
  process_smart_pointer< wld2src_pad > pad_output_wld2src_homog;
  process_smart_pointer< src2wld_pad > pad_output_src2wld_homog;
  process_smart_pointer< src2ref_pad > pad_output_src2ref_homog;
  process_smart_pointer< fg_img_pad > pad_output_fg_image;
  process_smart_pointer< diff_img_pad > pad_output_diff_image;
  process_smart_pointer< timestamp_vec_pad > pad_output_time_vec;
  process_smart_pointer< shot_break_flags_pad > pad_output_shot_breaks;
  process_smart_pointer< img2pln_pad > pad_output_img2pln_homog;
  process_smart_pointer< modality_pad > pad_output_modality;

  // Configuration parameters
  config_block config;
  config_block default_config;
  config_block dependency_config;
  bool use_gmm;
  bool run_async;
  bool masking_enabled;
  bool compute_unprocessed_mask;
  bool diff_out1_enabled;
  bool diff_out2_enabled;
  bool diff_out3_enabled;
  bool diff_out4_enabled;

  // Default initializer
  diff_super_process_impl()
  : config(),
    default_config(),
    dependency_config(),
    use_gmm( false ),
    masking_enabled( false ),
    compute_unprocessed_mask( false ),
    diff_out1_enabled( false ),
    diff_out2_enabled( false ),
    diff_out3_enabled( false ),
    diff_out4_enabled( false ),

    // Input Pads
    pad_source_image( NULL ),
    pad_source_timestamp( NULL ),
    pad_source_gsd( NULL ),
    pad_source_wld2utm_homog( NULL ),
    pad_source_wld2src_homog( NULL ),
    pad_source_src2wld_homog( NULL ),
    pad_source_src2ref_homog( NULL ),
    pad_source_src2ref_transform( NULL ),
    pad_source_mask( NULL ),
    pad_source_time_vec( NULL ),
    pad_source_shot_breaks( NULL ),
    pad_source_img2pln_homog( NULL ),
    pad_source_modality( NULL ),

    // Output Pads
    pad_output_image( NULL ),
    pad_output_timestamp( NULL ),
    pad_output_gsd( NULL ),
    pad_output_wld2utm_homog( NULL ),
    pad_output_wld2src_homog( NULL ),
    pad_output_src2wld_homog( NULL ),
    pad_output_src2ref_homog( NULL ),
    pad_output_fg_image( NULL ),
    pad_output_diff_image( NULL ),
    pad_output_time_vec( NULL ),
    pad_output_shot_breaks( NULL ),
    pad_output_img2pln_homog( NULL ),
    pad_output_modality( NULL ),

    // Processes
    proc_smooth_image( NULL ),
    proc_clamper( NULL ),
    proc_trans_for_cropping( NULL ),
    proc_fg_process( NULL ),
    proc_gmm_process( NULL ),
    proc_diff_out1( NULL ),
    proc_diff_out2( NULL ),
    proc_diff_out3( NULL ),
    proc_diff_out4( NULL ),
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
    proc_pass( NULL )
  {}

  // Create process objects and create config block for the pipeline.
  void create_process_configs()
  {
    // Input Pads
    pad_source_image = new image_pad("source_image");
    pad_source_timestamp = new timestamp_pad("source_timestamp");
    pad_source_gsd = new gsd_pad("source_gsd");
    pad_source_wld2utm_homog = new wld2utm_pad("source_wld2utm_H");
    pad_source_wld2src_homog = new wld2src_pad("source_wld2src_H");
    pad_source_src2wld_homog = new src2wld_pad("source_src2wld_H");
    pad_source_src2ref_homog = new src2ref_pad("source_src2ref_H");
    pad_source_src2ref_transform = new vgl_2d_pad("source_src2ref_transform");
    pad_source_mask = new mask_pad("source_mask");
    pad_source_time_vec = new timestamp_vec_pad("source_timestamp_vector");
    pad_source_shot_breaks = new shot_break_flags_pad("source_shot_breaks");
    pad_source_img2pln_homog = new img2pln_pad("source_img2pln_homog");
    pad_source_modality = new modality_pad("source_video_modality");

    // Output Pads
    pad_output_image = new image_pad("output_image");
    pad_output_timestamp = new timestamp_pad("output_timestamp");
    pad_output_gsd = new gsd_pad("output_gsd");
    pad_output_wld2utm_homog = new wld2utm_pad("output_wld2utm_H");
    pad_output_wld2src_homog = new wld2src_pad("output_wld2src_H");
    pad_output_src2wld_homog = new src2wld_pad("output_src2wld_H");
    pad_output_src2ref_homog = new src2ref_pad("output_src2ref_H");
    pad_output_fg_image = new fg_img_pad("output_fg_img");
    pad_output_diff_image = new diff_img_pad("output_diff_img");
    pad_output_time_vec = new timestamp_vec_pad("output_timestamp_vector");
    pad_output_shot_breaks = new shot_break_flags_pad("output_shot_breaks");
    pad_output_img2pln_homog = new img2pln_pad("output_img2pln_homog");
    pad_output_modality = new modality_pad("output_video_modality");

    // Processes
    proc_smooth_image = new world_smooth_image_process<PixType>( "smooth_image" );
    config.add_subblock( proc_smooth_image->params(), proc_smooth_image->name() );

    proc_clamper = new clamp_image_pixel_values_process<PixType>( "pixel_value_clamper" );
    config.add_subblock( proc_clamper->params(), proc_clamper->name() );

    proc_trans_for_cropping = new transform_homography_process( "trans_for_cropping" );
    config.add_subblock( proc_trans_for_cropping->params(), proc_trans_for_cropping->name() );

    proc_fg_process = new image_difference_process2<PixType>( "image_diff" );
    config.add_subblock( proc_fg_process->params(), proc_fg_process->name() );

    proc_gmm_process = new sg_background_model_process<PixType>( "gmm" );
    config.add_subblock( proc_gmm_process->params(), proc_gmm_process->name() );

    proc_diff_out1 = new image_list_writer_process<bool>( "diff_fg_out" );
    config.add_subblock( proc_diff_out1->params(), proc_diff_out1->name() );

    proc_diff_out2 = new image_list_writer_process<float>( "diff_float_out" );
    config.add_subblock( proc_diff_out2->params(), proc_diff_out2->name() );

    proc_diff_out3 = new image_list_writer_process<bool>( "diff_fg_zscore_out" );
    config.add_subblock( proc_diff_out3->params(), proc_diff_out3->name() );

    proc_diff_out4 = new image_list_writer_process<float>( "diff_float_zscore_out" );
    config.add_subblock( proc_diff_out4->params(), proc_diff_out4->name() );

    proc_cropped_buffer = new diff_buffer_process<PixType>("cropped_image_buffer");
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

    proc_pass = new diff_pass_thru_process<PixType>( "diff_pass_thru" );
    config.add_subblock( proc_pass->params(), proc_pass->name() );


    // Over-riding the process default with this super-process defaults.
    default_config.add( proc_diff_out1->name() + ":disabled", "true" );
    default_config.add( proc_diff_out2->name() + ":disabled", "true" );
    default_config.add( proc_diff_out3->name() + ":disabled", "true" );
    default_config.add( proc_diff_out4->name() + ":disabled", "true" );
    default_config.add( proc_uncrop_unprocessed->name() + ":background_value", "true" );
    default_config.add( proc_unprocessed_mask_writer->name() + ":disabled", "true" );

    // Update config
    config.update( default_config );

    // Following default values will over-write the process defaults.
    dependency_config.add( proc_cropped_buffer->name() + ":spacing", "2" );
    dependency_config.add( proc_mask_crop->name() + ":upper_left", "0 0" );
    dependency_config.add( proc_mask_crop->name() + ":lower_right", "0 0" );
    dependency_config.add( proc_uncrop_unprocessed->name() + ":disabled", "false" );
    dependency_config.add( proc_uncrop_unprocessed->name() + ":horizontal_padding", "0" );
    dependency_config.add( proc_uncrop_unprocessed->name() + ":vertical_padding", "0" );
    dependency_config.add( proc_fg_process->name() + ":compute_unprocessed_mask", "false" );
  }

  // Create sync or async pipeline
  template <class PIPELINE>
  void setup_pipeline( PIPELINE * p )
  {
    // Add Required Input Pads
    p->add( pad_source_image );
    p->add( pad_source_timestamp );
    p->add( pad_source_gsd );
    p->add( pad_source_wld2utm_homog );
    p->add( pad_source_wld2src_homog );
    p->add( pad_source_src2wld_homog );
    p->add( pad_source_src2ref_homog );
    p->add( pad_source_src2ref_transform );
    p->add( pad_source_time_vec );
    p->add( pad_source_shot_breaks );
    p->add( pad_source_img2pln_homog );
    p->add( pad_source_modality );

    // Required Output Pads
    p->add( pad_output_image );
    p->add( pad_output_timestamp );
    p->add( pad_output_gsd );
    p->add( pad_output_wld2utm_homog );
    p->add( pad_output_wld2src_homog );
    p->add( pad_output_src2wld_homog );
    p->add( pad_output_src2ref_homog );
    p->add( pad_output_fg_image );
    p->add( pad_output_diff_image );
    p->add( pad_output_time_vec );
    p->add( pad_output_shot_breaks );
    p->add( pad_output_img2pln_homog );
    p->add( pad_output_modality );

    // Add Processes
    p->add( proc_image_crop );
    p->add( proc_smooth_image );
    p->add( proc_trans_for_cropping );
    p->add( proc_clamper );
    p->add( proc_mask_fg );
    p->add( proc_pass );

    // Set up input connections, convert input to grayscale image if not
    // using the gaussian mixture model approach (for efficiency)
    if( !use_gmm )
    {
      p->add( proc_greyscale );
      p->connect( pad_source_image->value_port(),
                  proc_greyscale->set_image_port() );
      p->connect( proc_greyscale->copied_image_port(),
                  proc_clamper->set_source_image_port() );
    }
    else
    {
      p->connect( pad_source_image->value_port(),
                  proc_clamper->set_source_image_port() );
    }

    // Input Pad Connections
    p->connect( pad_source_gsd->value_port(),
                proc_smooth_image->set_world_units_per_pixel_port() );
    p->connect( pad_source_src2ref_transform->value_port(),
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
    if( use_gmm )
    {
      p->add( proc_gmm_process );

      p->connect( proc_image_crop->cropped_image_port(),
                  proc_gmm_process->set_source_image_port() );
      p->connect( proc_trans_for_cropping->homography_port(),
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

      // Connect cropped images/homographies to buffer and warp
      p->connect( proc_image_crop->cropped_image_port(),
                  proc_cropped_buffer->add_source_img_port() );
      p->connect( proc_trans_for_cropping->homography_port(),
                  proc_cropped_buffer->add_source_homog_port() );
      p->connect( proc_cropped_buffer->get_first_image_port(),
                  proc_warp_1->set_source_image_port() );
      p->connect( proc_cropped_buffer->get_third_to_first_homog_port(),
                  proc_warp_1->set_destination_to_source_homography_port() );
      p->connect( proc_cropped_buffer->get_second_image_port(),
                  proc_warp_2->set_source_image_port() );
      p->connect( proc_cropped_buffer->get_third_to_second_homog_port(),
                  proc_warp_2->set_destination_to_source_homography_port() );

      // Connect warp processes to image difference function
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
      if( diff_out2_enabled ) {
        p->add( proc_diff_out2 );
        p->connect( pad_source_timestamp->value_port(),
                    proc_diff_out2->set_timestamp_port() );
        p->connect( proc_fg_process->diff_image_port(),
                    proc_diff_out2->set_image_port() );
      }
      if( diff_out3_enabled ) {
        p->add( proc_diff_out3 );
        p->connect( pad_source_timestamp->value_port(),
                    proc_diff_out3->set_timestamp_port() );
        p->connect( proc_fg_process->fg_image_zscore_port(),
                    proc_diff_out3->set_image_port() );
      }
      if( diff_out4_enabled ) {
        p->add( proc_diff_out4 );
        p->connect( pad_source_timestamp->value_port(),
                    proc_diff_out4->set_timestamp_port() );
        p->connect( proc_fg_process->diff_image_zscore_port(),
                    proc_diff_out4->set_image_port() );
      }
    }

    // Add additional debug output process for fg mask if enabled
    if( diff_out1_enabled ) {
        p->add( proc_diff_out1 );
        p->connect( pad_source_timestamp->value_port(),
                    proc_diff_out1->set_timestamp_port() );
        p->connect( proc_mask_fg->image_port(),
                    proc_diff_out1->set_image_port() );
    }

    // Connect all outputs to dummy pass thru process to detect failures
    p->connect( pad_source_image->value_port(),
                proc_pass->set_input_image_port() );
    p->connect( pad_source_gsd->value_port(),
                proc_pass->set_input_gsd_port() );
    p->connect( pad_source_timestamp->value_port(),
                proc_pass->set_input_timestamp_port() );
    p->connect( pad_source_wld2src_homog->value_port(),
                proc_pass->set_input_wld_to_src_homography_port() );
    p->connect( pad_source_wld2utm_homog->value_port(),
                proc_pass->set_input_wld_to_utm_homography_port() );
    p->connect( pad_source_src2wld_homog->value_port(),
                proc_pass->set_input_src_to_wld_homography_port() );
    p->connect( pad_source_src2ref_homog->value_port(),
                proc_pass->set_input_src_to_ref_homography_port() );
    p->connect( pad_source_time_vec->value_port(),
                proc_pass->set_input_ts_vector_port() );
    p->connect( pad_source_shot_breaks->value_port(),
                proc_pass->set_input_shot_break_flags_port() );
    p->connect( pad_source_img2pln_homog->value_port(),
                proc_pass->set_input_ref_to_wld_homography_port() );
    p->connect( pad_source_modality->value_port(),
                proc_pass->set_input_video_modality_port() );
    p->connect( proc_mask_fg->copied_image_port(),
                proc_pass->set_input_fg_port() );
    p->connect( proc_fg_process->diff_image_port(),
                proc_pass->set_input_diff_port() );

    // Connect dummy output process to all output pads
    p->connect( proc_pass->get_output_image_port(),
                pad_output_image->set_value_port() );
    p->connect( proc_pass->get_output_gsd_port(),
                pad_output_gsd->set_value_port() );
    p->connect( proc_pass->get_output_timestamp_port(),
                pad_output_timestamp->set_value_port() );
    p->connect( proc_pass->get_output_wld_to_src_homography_port(),
                pad_output_wld2src_homog->set_value_port() );
    p->connect( proc_pass->get_output_wld_to_utm_homography_port(),
                pad_output_wld2utm_homog->set_value_port() );
    p->connect( proc_pass->get_output_src_to_wld_homography_port(),
                pad_output_src2wld_homog->set_value_port() );
    p->connect( proc_pass->get_output_src_to_ref_homography_port(),
                pad_output_src2ref_homog->set_value_port() );
    p->connect( proc_pass->get_output_ts_vector_port(),
                pad_output_time_vec->set_value_port() );
    p->connect( proc_pass->get_output_shot_break_flags_port(),
                pad_output_shot_breaks->set_value_port() );
    p->connect( proc_pass->get_output_ref_to_wld_homography_port(),
                pad_output_img2pln_homog->set_value_port() );
    p->connect( proc_pass->get_output_video_modality_port(),
                pad_output_modality->set_value_port() );
    p->connect( proc_pass->get_output_fg_port(),
                pad_output_fg_image->set_value_port() );
    p->connect( proc_pass->get_output_diff_port(),
                pad_output_diff_image->set_value_port() );
  }
};

template< class PixType >
diff_super_process<PixType>
::diff_super_process( vcl_string const& name )
  : super_process( name, "diff_super_process" ),
    impl_( NULL )
{
  impl_ = new diff_super_process_impl<PixType>;

  impl_->create_process_configs();

  impl_->config.add_parameter( "useGMM",
    "false",
    "Whether or not to use GMM" );

  impl_->config.add_parameter( "image_diff_spacing",
    "2",
    "Temporal gap (number of frames) used in "
    "image_differenc_process2::spacing." );

  impl_->config.add_parameter( "run_async",
    "false",
    "Whether or not to run process asynchronously" );

  impl_->config.add_parameter( "masking_enabled",
    "false",
    "Use the input mask provided from prior process (stab_sp)." );

  impl_->config.add_parameter( "compute_unprocessed_mask",
    "false",
    "Change the pipeline and produce a mask representing pixels not "
    " considered for moving foreground detection." );
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
    blk.get( "run_async", impl_->run_async );
    blk.get( "useGMM", impl_->use_gmm );
    blk.get( "masking_enabled", impl_->masking_enabled );

    // dependency_config
    if( !impl_->use_gmm )
    {
      // dependencies of image_diff_spacing
      unsigned diff_spacing;
      diff_spacing = blk.get<unsigned>( "image_diff_spacing" );
      impl_->dependency_config.set( impl_->proc_cropped_buffer->name()
                                    + ":spacing", diff_spacing );
    }

    // dependencies of crop size dependencies
    vcl_string ul, lr;
    blk.get( impl_->proc_image_crop->name() + ":upper_left", ul );
    blk.get( impl_->proc_image_crop->name() + ":lower_right", lr );
    impl_->dependency_config.set( impl_->proc_mask_crop->name() + ":upper_left", ul );
    impl_->dependency_config.set( impl_->proc_mask_crop->name() + ":lower_right", lr );
    vcl_stringstream ss;
    ss << ul;
    vul_awk awk( ss );
    impl_->dependency_config.set( impl_->proc_uncrop_unprocessed->name() + ":horizontal_padding", awk[0]);
    impl_->dependency_config.set( impl_->proc_uncrop_unprocessed->name() + ":vertical_padding",  awk[1]);

    // dependencies of compute_unprocessed_mask
    blk.get( "compute_unprocessed_mask", impl_->compute_unprocessed_mask );
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

    // Determine whether or not debug output processes are enabled
    impl_->diff_out1_enabled = !blk.get<bool>( impl_->proc_diff_out1->name() + ":disabled" );
    impl_->diff_out2_enabled = !blk.get<bool>( impl_->proc_diff_out2->name() + ":disabled" );
    impl_->diff_out3_enabled = !blk.get<bool>( impl_->proc_diff_out3->name() + ":disabled" );
    impl_->diff_out4_enabled = !blk.get<bool>( impl_->proc_diff_out4->name() + ":disabled" );

    // Configure asynch or synch pipeline
    if( impl_->run_async )
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

    if( !pipeline_->set_params( impl_->config ) )
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
  return pipeline_->execute();
}


// ================================================================
// Process Input methods

template< class PixType >
void diff_super_process<PixType>
::set_source_image( vil_image_view<PixType> const& img )
{
  impl_->pad_source_image->set_value( img );
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
  // Homography object is simply passed through
  impl_->pad_source_src2ref_homog->set_value( H );
  // Actual homography transform is used for computations
  impl_->pad_source_src2ref_transform->set_value( H.get_transform() );
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
::set_wld_to_utm_homography( plane_to_utm_homography const& H )
{
  impl_->pad_source_wld2utm_homog->set_value( H );
}

template< class PixType >
void diff_super_process<PixType>
::set_input_timestamp_vector( timestamp::vector_t const& ts_vec )
{
  impl_->pad_source_time_vec->set_value( ts_vec );
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

// ================================================================
// Process output methods

template< class PixType >
vil_image_view<bool> const&
diff_super_process<PixType>
::fg_out() const
{
  return impl_->pad_output_fg_image->value();
}

template< class PixType >
vil_image_view<float> const&
diff_super_process<PixType>
::diff_out() const
{
  return impl_->pad_output_diff_image->value();
}

template< class PixType >
vil_image_view<bool> const&
diff_super_process<PixType>
::copied_fg_out() const
{
  return impl_->pad_output_fg_image->value();
}

template< class PixType >
vil_image_view<float> const&
diff_super_process<PixType>
::copied_diff_out() const
{
  return impl_->pad_output_diff_image->value();
}

template< class PixType >
double
diff_super_process<PixType>
::world_units_per_pixel() const
{
  return impl_->pad_output_gsd->value();
}

template< class PixType >
timestamp const&
diff_super_process<PixType>
::source_timestamp() const
{
  return impl_->pad_output_timestamp->value();
}

template< class PixType >
vil_image_view< PixType > const&
diff_super_process<PixType>
::source_image() const
{
  return impl_->pad_output_image->value();
}

template< class PixType >
image_to_image_homography const&
diff_super_process<PixType>
::src_to_ref_homography() const
{
  return impl_->pad_output_src2ref_homog->value();
}

template< class PixType >
image_to_plane_homography const&
diff_super_process<PixType>
::src_to_wld_homography() const
{
  return impl_->pad_output_src2wld_homog->value();
}

template< class PixType >
plane_to_image_homography const&
diff_super_process<PixType>
::wld_to_src_homography() const
{
  return impl_->pad_output_wld2src_homog->value();
}

template< class PixType >
plane_to_utm_homography const&
diff_super_process<PixType>
::wld_to_utm_homography() const
{
  return impl_->pad_output_wld2utm_homog->value();
}

template< class PixType >
timestamp::vector_t const&
diff_super_process<PixType>
::get_output_timestamp_vector() const
{
  return impl_->pad_output_time_vec->value();
}

template< class PixType >
image_to_plane_homography const&
diff_super_process<PixType>
::get_output_ref_to_wld_homography() const
{
  return impl_->pad_output_img2pln_homog->value();
}

template< class PixType >
video_modality const&
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

} // end namespace vidtk

