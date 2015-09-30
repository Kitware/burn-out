/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_conn_comp_super_process_h_
#define vidtk_conn_comp_super_process_h_

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <vil/vil_image_view.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>
#include <tracking/image_object.h>
#include <tracking/shot_break_flags.h>

#include <process_framework/pipeline_aid.h>
#include <pipeline/super_process.h>

namespace vidtk
{

template< class PixType >
class conn_comp_super_process_impl;

template< class PixType >
class conn_comp_super_process  : public super_process
{
public:
  typedef conn_comp_super_process self_type;
  conn_comp_super_process( vcl_string const& name );
  ~conn_comp_super_process();

  // process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual process::step_status step2();

  // Input ports
  void set_fg_image( vil_image_view< bool > const& img );
  VIDTK_INPUT_PORT( set_fg_image, vil_image_view< bool > const& );

  void set_source_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_source_timestamp, timestamp const& );

  void set_src_to_wld_homography( image_to_plane_homography const& H );
  VIDTK_INPUT_PORT( set_src_to_wld_homography, image_to_plane_homography const& );

  void set_wld_to_src_homography( plane_to_image_homography const& H );
  VIDTK_INPUT_PORT( set_wld_to_src_homography, plane_to_image_homography const& );

  void set_wld_to_utm_homography( plane_to_utm_homography const& H );
  VIDTK_INPUT_PORT( set_wld_to_utm_homography, plane_to_utm_homography const& );

  void set_src_to_ref_homography( image_to_image_homography const& H );
  VIDTK_INPUT_PORT( set_src_to_ref_homography, image_to_image_homography const& );

  void set_world_units_per_pixel( double const& gsd );
  VIDTK_INPUT_PORT( set_world_units_per_pixel, double const& );

  void set_source_image( vil_image_view< PixType > const& img );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );


  // Output ports
  vcl_vector< image_object_sptr > const& output_objects() const;
  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, output_objects );

  vcl_vector< image_object_sptr > const& projected_objects() const;
  VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, projected_objects );

  vil_image_view< bool > const& fg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< bool > const&, fg_image );

  vil_image_view<bool> const& morph_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool> const&, morph_image );

  vil_image_view<bool> const& copied_morph_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool> const&, copied_morph_image );

  timestamp const& source_timestamp() const;
  VIDTK_OUTPUT_PORT( timestamp const&, source_timestamp );

  vil_image_view< PixType > const& source_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType > const&, source_image );

  image_to_image_homography const& src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography const&, src_to_ref_homography);

  image_to_plane_homography const& src_to_wld_homography() const;
  VIDTK_OUTPUT_PORT( image_to_plane_homography const&, src_to_wld_homography );

  plane_to_image_homography const& wld_to_src_homography() const;
  VIDTK_OUTPUT_PORT( plane_to_image_homography const&, wld_to_src_homography );

  plane_to_utm_homography const& wld_to_utm_homography() const;
  VIDTK_OUTPUT_PORT( plane_to_utm_homography const&, wld_to_utm_homography );

  double world_units_per_pixel() const;
  VIDTK_OUTPUT_PORT( double, world_units_per_pixel );

  // ---- Pass thru ports

  VIDTK_PASS_THRU_PORT (timestamp_vector, vidtk::timestamp::vector_t);
  VIDTK_PASS_THRU_PORT (diff_out_image, vil_image_view< float > );
  VIDTK_PASS_THRU_PORT (ref_to_wld_homography, image_to_plane_homography);
  VIDTK_PASS_THRU_PORT (video_modality, vidtk::video_modality);
  VIDTK_PASS_THRU_PORT (shot_break_flags, vidtk::shot_break_flags);


private:
  conn_comp_super_process_impl<PixType> * impl_;
};

} // namespace vidtk

#endif // vidtk_conn_comp_super_process_h_
