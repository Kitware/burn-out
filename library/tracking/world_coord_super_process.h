/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_world_coord_super_process_h_
#define vidtk_world_coord_super_process_h_

#include <pipeline/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_metadata.h>
#include <tracking/shot_break_flags.h>


namespace vidtk
{

template< class PixType >
class world_coord_super_process_impl;


// ----------------------------------------------------------------
/** Super process to determine video modality and world coordinates.
 *
 * This super process determines the world coordinates from the video
 * metadata passed in.  The output is delayed until valid metadata is
 * received, then buffered inputs are released downstream.
 *
 * The video modality (EO/IR, NFOV,MFOV) is also determined.
 *
 * This super process is run in an asynchronous pipeline and contains
 * a synchronous pipeline.
 *
 * @todo Move metadata filter process into this super-process.
 * @todo Move EO/IR detection out of this SP as a separate process.
 */
template< class PixType >
class world_coord_super_process
  : public super_process
{
public:
  typedef world_coord_super_process self_type;

  world_coord_super_process( vcl_string const& name );
  ~world_coord_super_process();

  // Process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();


  // Input ports

  // Set the detected foreground image.
  void set_source_image(vil_image_view< PixType > const& img);
  VIDTK_OPTIONAL_INPUT_PORT(set_source_image, vil_image_view< PixType > const &);

  void set_source_timestamp(timestamp const& v);
  VIDTK_OPTIONAL_INPUT_PORT(set_source_timestamp, timestamp const &);

  void set_input_timestamp_vector(vidtk::timestamp::vector_t const& v);
  VIDTK_OPTIONAL_INPUT_PORT(set_input_timestamp_vector, vidtk::timestamp::vector_t const &);

  void set_source_metadata_vector(vcl_vector< video_metadata > const& v);
  VIDTK_OPTIONAL_INPUT_PORT(set_source_metadata_vector, vcl_vector< video_metadata > const &);

  void set_src_to_ref_homography(image_to_image_homography const& v);
  VIDTK_OPTIONAL_INPUT_PORT(set_src_to_ref_homography, image_to_image_homography const &);

  void set_input_shot_break_flags(vidtk::shot_break_flags const& v);
  VIDTK_OPTIONAL_INPUT_PORT(set_input_shot_break_flags, vidtk::shot_break_flags const &);

  void set_mask(vil_image_view< bool > const& v);
  VIDTK_OPTIONAL_INPUT_PORT(set_mask, vil_image_view< bool > const &);

  // Output ports

  timestamp const& source_timestamp() const;
  VIDTK_OUTPUT_PORT(timestamp const &, source_timestamp);

  vil_image_view< PixType > const& source_image() const;
  VIDTK_OUTPUT_PORT(vil_image_view< PixType > const &, source_image);

/// The output homography from source (current) to reference image.
  image_to_image_homography const& src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT(image_to_image_homography const &, src_to_ref_homography);

/// The output homography from source (current) to (tracking) world.
  image_to_plane_homography const& src_to_wld_homography() const;
  VIDTK_OUTPUT_PORT(image_to_plane_homography const &, src_to_wld_homography);

/// The output homography from (tracking) world to source (current) image.
  plane_to_image_homography const& wld_to_src_homography() const;
  VIDTK_OUTPUT_PORT(plane_to_image_homography const &, wld_to_src_homography);

  plane_to_utm_homography const& wld_to_utm_homography() const;
  VIDTK_OUTPUT_PORT(plane_to_utm_homography, wld_to_utm_homography);

  image_to_plane_homography const& get_output_ref_to_wld_homography() const;
  VIDTK_OUTPUT_PORT(image_to_plane_homography, get_output_ref_to_wld_homography);

  vidtk::timestamp::vector_t const& get_output_timestamp_vector() const;
  VIDTK_OUTPUT_PORT(vidtk::timestamp::vector_t, get_output_timestamp_vector);

  double world_units_per_pixel() const;
  VIDTK_OUTPUT_PORT(double, world_units_per_pixel);

  vidtk::shot_break_flags get_output_shot_break_flags() const;
  VIDTK_OUTPUT_PORT(vidtk::shot_break_flags, get_output_shot_break_flags);

  vil_image_view< bool > get_output_mask() const;
  VIDTK_OUTPUT_PORT(vil_image_view< bool >, get_output_mask);


  void pump_output();


private:
  world_coord_super_process_impl<PixType> * impl_;
};

} // end namespace vidtk

#endif // vidtk_world_coord_super_process_h_
