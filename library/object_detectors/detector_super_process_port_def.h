/*ckwg +5
 * Copyright 2014-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


// Note that the anti-recursion ifdef is different. The intent is to
// keep these macro definitions out of user code. This is done by
// undef'ing these macros at the end of header files that use them,
// and including the definitions in the source files where needed.
//
// So we really don't want to limit this file to be included only
// once in the source module. We only want to ensure that the macros
// are only defined once.

#ifndef DETECTOR_SUPER_PROCESS_INPUTS

  // -- INPUTS --
  // Superset of all input ports a detector process can use
  // default definitions are provided override the ones necessary for each detector
  // ( type, name )
#define DETECTOR_SUPER_PROCESS_INPUTS( CALL )                           \
  CALL( vidtk::timestamp,                   timestamp )                 \
  CALL( vil_image_view< PixType >,          image )                     \
  CALL( vil_image_view< bool >,             mask_image )                \
  CALL( vidtk::image_to_image_homography,   src_to_ref_homography )     \
  CALL( vidtk::image_to_plane_homography,   src_to_wld_homography )     \
  CALL( vidtk::image_to_utm_homography,     src_to_utm_homography )     \
  CALL( vidtk::plane_to_image_homography,   wld_to_src_homography )     \
  CALL( vidtk::plane_to_utm_homography,     wld_to_utm_homography )     \
  CALL( vidtk::image_to_plane_homography,   ref_to_wld_homography )     \
  CALL( double,                             world_units_per_pixel )     \
  CALL( vidtk::video_modality,              video_modality )            \
  CALL( vidtk::shot_break_flags,            shot_break_flags )          \
  CALL( vidtk::gui_frame_info,              gui_feedback )


  // -- OUTPUTS --
  // Superset of all output ports a detector process can use
  // ( type, name )
#define DETECTOR_SUPER_PROCESS_OUTPUTS( CALL )                          \
  CALL( vidtk::timestamp,                 timestamp )                   \
  CALL( vil_image_view< PixType >,        image )                       \
  CALL( std::vector< image_object_sptr >, image_objects )               \
  CALL( std::vector< image_object_sptr >, projected_objects )           \
  CALL( vil_image_view< bool >,           fg_image )                    \
  CALL( vil_image_view< bool >,           morph_image )                 \
  CALL( vil_image_view< PixType >,        morph_grey_image )            \
  CALL( vil_image_view< float >,          diff_image )                  \
  CALL( vidtk::image_to_image_homography, src_to_ref_homography )       \
  CALL( vidtk::image_to_plane_homography, src_to_wld_homography )       \
  CALL( vidtk::image_to_utm_homography,   src_to_utm_homography )       \
  CALL( vidtk::plane_to_image_homography, wld_to_src_homography )       \
  CALL( vidtk::plane_to_utm_homography,   wld_to_utm_homography )       \
  CALL( vidtk::image_to_plane_homography, ref_to_wld_homography )       \
  CALL( double,                           world_units_per_pixel )       \
  CALL( vidtk::video_modality,            video_modality )              \
  CALL( vidtk::shot_break_flags,          shot_break_flags )            \
  CALL( vidtk::gui_frame_info,            gui_feedback )                \
  CALL( typename vidtk::pixel_feature_array< PixType >::sptr_t,    pixel_features )

#endif
