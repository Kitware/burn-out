/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_conn_comp_super_process_h_
#define vidtk_conn_comp_super_process_h_

#include <process_framework/pipeline_aid.h>
#include <pipeline_framework/super_process.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>

#include <tracking_data/image_object.h>
#include <tracking_data/shot_break_flags.h>
#include <tracking_data/gui_frame_info.h>
#include <tracking_data/pixel_feature_array.h>

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>


namespace vidtk
{

template< class PixType >
class conn_comp_super_process_impl;

template< class PixType >
class conn_comp_super_process  : public super_process
{
public:

  typedef conn_comp_super_process self_type;
  typedef typename pixel_feature_array< PixType >::sptr_t feature_array_sptr;

  conn_comp_super_process( std::string const& name );
  ~conn_comp_super_process();

  // process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

#define conn_comp_sp_pass_thrus(call)                     \
  call(ref_to_wld_homography, image_to_plane_homography)  \
  call(wld_to_utm_homography, plane_to_utm_homography)    \
  call(video_modality,        vidtk::video_modality)      \
  call(shot_break_flags,      vidtk::shot_break_flags)

#define conn_comp_sp_inputs(call)                        \
  call(fg_image,              vil_image_view<bool>)      \
  call(source_timestamp,      timestamp)                 \
  call(src_to_wld_homography, image_to_plane_homography) \
  call(wld_to_src_homography, plane_to_image_homography) \
  call(src_to_utm_homography, image_to_utm_homography)   \
  call(src_to_ref_homography, image_to_image_homography) \
  call(world_units_per_pixel, double)                    \
  call(source_image,          vil_image_view<PixType>)   \
  call(diff_image,            vil_image_view<float>)     \
  conn_comp_sp_pass_thrus(call)

#define conn_comp_sp_optional_inputs(call)               \
  call(gui_feedback,          gui_frame_info)            \
  call(pixel_feature_array,   feature_array_sptr)

#define declare_port_input(name, type) \
  void set_##name(type const& value);  \
  VIDTK_INPUT_PORT(set_##name, type const&);

  conn_comp_sp_inputs(declare_port_input)
  conn_comp_sp_optional_inputs(declare_port_input)

#undef declare_port_input

#define conn_comp_sp_outputs(call)                           \
  call(output_objects,        std::vector<image_object_sptr>) \
  call(projected_objects,     std::vector<image_object_sptr>) \
  call(fg_image,              vil_image_view<bool>)          \
  call(morph_image,           vil_image_view<bool>)          \
  call(morph_grey_image,      vil_image_view<PixType>)       \
  call(source_timestamp,      timestamp)                     \
  call(source_image,          vil_image_view<PixType>)       \
  call(src_to_ref_homography, image_to_image_homography)     \
  call(src_to_wld_homography, image_to_plane_homography)     \
  call(wld_to_src_homography, plane_to_image_homography)     \
  call(src_to_utm_homography, image_to_utm_homography)       \
  call(world_units_per_pixel, double)                        \
  call(diff_image,            vil_image_view<float>)         \
  conn_comp_sp_pass_thrus(call)

#define conn_comp_sp_optional_outputs(call)                  \
  call(gui_feedback,          gui_frame_info)                \
  call(pixel_feature_array,   typename conn_comp_super_process<PixType>::feature_array_sptr)

#define declare_port_output(name, type) \
  type name() const;                    \
  VIDTK_OUTPUT_PORT(type, name);

  conn_comp_sp_outputs(declare_port_output)
  conn_comp_sp_optional_outputs(declare_port_output)

#undef declare_port_output

private:

  // The actual conn component pipeline
  conn_comp_super_process_impl<PixType> * impl_;

  // Configure the optimized variable morphology system if enabled
  void optimize_variable_morphology_system( const double& subpix_resolution );
};

} // namespace vidtk

#endif // vidtk_conn_comp_super_process_h_
