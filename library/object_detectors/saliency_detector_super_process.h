/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_saliency_detector_super_process_h_
#define vidtk_saliency_detector_super_process_h_

#include <pipeline_framework/super_process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking_data/shot_break_flags.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>
#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

template< class PixType >
class saliency_detector_super_process_impl;

template< class PixType >
class saliency_detector_super_process  : public super_process
{
public:

  typedef saliency_detector_super_process self_type;

  saliency_detector_super_process( std::string const& _name );
  virtual ~saliency_detector_super_process();

  // Process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  // Input ports
  void set_source_image( vil_image_view< PixType > const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );

  void set_source_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_src_to_ref_homography( image_to_image_homography const& H );
  VIDTK_INPUT_PORT( set_src_to_ref_homography, image_to_image_homography const& );

  void set_src_to_wld_homography( image_to_plane_homography const& H );
  VIDTK_INPUT_PORT( set_src_to_wld_homography, image_to_plane_homography const& );

  void set_wld_to_src_homography( plane_to_image_homography const& H );
  VIDTK_INPUT_PORT( set_wld_to_src_homography, plane_to_image_homography const& );

  void set_src_to_utm_homography( image_to_utm_homography const& H );
  VIDTK_INPUT_PORT( set_src_to_utm_homography, image_to_utm_homography const& );

  void set_world_units_per_pixel( double const& gsd );
  VIDTK_INPUT_PORT( set_world_units_per_pixel, double const& );

  void set_mask( vil_image_view<bool> const& );
  VIDTK_INPUT_PORT( set_mask, vil_image_view<bool> const& );

  void set_input_ref_to_wld_homography( image_to_plane_homography const& H );
  VIDTK_INPUT_PORT( set_input_ref_to_wld_homography, image_to_plane_homography const& );

  void set_input_video_modality( video_modality const& vm );
  VIDTK_INPUT_PORT( set_input_video_modality, video_modality const& );

  void set_input_shot_break_flags( shot_break_flags const& sb );
  VIDTK_INPUT_PORT( set_input_shot_break_flags, shot_break_flags const& );

  // Output ports
  vil_image_view<bool> fg_out() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_out );

  vil_image_view<float> diff_out() const;
  VIDTK_OUTPUT_PORT( vil_image_view<float>, diff_out );

  vil_image_view<bool> copied_fg_out() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, copied_fg_out );

  vil_image_view<float> copied_diff_out() const;
  VIDTK_OUTPUT_PORT( vil_image_view<float>, copied_diff_out );

  double world_units_per_pixel() const;
  VIDTK_OUTPUT_PORT( double, world_units_per_pixel );

  timestamp source_timestamp() const;
  VIDTK_OUTPUT_PORT( timestamp, source_timestamp );

  vil_image_view< PixType > source_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType >, source_image );

  image_to_image_homography src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, src_to_ref_homography);

  image_to_plane_homography src_to_wld_homography() const;
  VIDTK_OUTPUT_PORT( image_to_plane_homography, src_to_wld_homography );

  plane_to_image_homography wld_to_src_homography() const;
  VIDTK_OUTPUT_PORT( plane_to_image_homography, wld_to_src_homography );

  image_to_utm_homography src_to_utm_homography() const;
  VIDTK_OUTPUT_PORT( image_to_utm_homography, src_to_utm_homography );

  image_to_plane_homography get_output_ref_to_wld_homography() const;
  VIDTK_OUTPUT_PORT( image_to_plane_homography, get_output_ref_to_wld_homography );

  video_modality get_output_video_modality() const;
  VIDTK_OUTPUT_PORT( video_modality, get_output_video_modality );

  shot_break_flags get_output_shot_break_flags() const;
  VIDTK_OUTPUT_PORT( shot_break_flags, get_output_shot_break_flags );

private:

  saliency_detector_super_process_impl<PixType> * impl_;
};

} // namespace vidtk

#endif // vidtk_saliency_detector_super_process_h_
