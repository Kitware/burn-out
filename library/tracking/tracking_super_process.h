/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_tracking_super_process_h_
#define vidtk_tracking_super_process_h_

#include <vgl/algo/vgl_h_matrix_2d.h>

#include <vil/vil_image_view.h>

#include <tracking/image_object.h>
#include <tracking/track.h>
#include <tracking/shot_break_flags.h>
#include <tracking/da_so_tracker.h>

#include <utilities/timestamp.h>
#include <utilities/homography.h>
#include <utilities/video_modality.h>

#include <pipeline/super_process.h>

namespace vidtk
{

template < class PixType >
class tracking_super_process_impl;
class gui_process;

template < class PixType >
class tracking_super_process  : public super_process
{
public:
  typedef tracking_super_process self_type;
  tracking_super_process( vcl_string const& name );
  ~tracking_super_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual process::step_status step2();

  void set_multi_features_dir( vcl_string const & dir );

  void set_guis( process_smart_pointer< gui_process > ft_gui,
                 process_smart_pointer< gui_process > bt_gui );

  // ---- Input ports
  void set_source_objects( vcl_vector< image_object_sptr > const& objs );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_objects, vcl_vector<image_object_sptr> const& );

  void set_source_image( vil_image_view< PixType > const& image );
  VIDTK_OPTIONAL_INPUT_PORT( set_source_image, vil_image_view< PixType > const& );

  void set_timestamp( timestamp const& ts );
  VIDTK_OPTIONAL_INPUT_PORT( set_timestamp, timestamp const& );

  void set_src_to_wld_homography( image_to_plane_homography const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_src_to_wld_homography, image_to_plane_homography const& );

  void set_wld_to_src_homography( plane_to_image_homography const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_wld_to_src_homography, plane_to_image_homography const& );

  void set_src_to_ref_homography( image_to_image_homography const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_src_to_ref_homography, image_to_image_homography const& );

  void set_wld_to_utm_homography( plane_to_utm_homography const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_wld_to_utm_homography, plane_to_utm_homography const& );

  void set_fg_image( vil_image_view< bool > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_fg_image, vil_image_view< bool > const& );

  void set_diff_image( vil_image_view< float > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_diff_image, vil_image_view< float > const& );

  void set_morph_image( vil_image_view< bool > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_morph_image, vil_image_view< bool > const& );

  void set_unfiltered_objects( vcl_vector< image_object_sptr > const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_unfiltered_objects, vcl_vector< image_object_sptr > const& );

  void set_world_units_per_pixel( double );
  VIDTK_OPTIONAL_INPUT_PORT( set_world_units_per_pixel, double );

  // ---- Output ports
  vcl_vector< track_sptr > const & linked_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const &, linked_tracks );

  vcl_vector< track_sptr > const& init_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, init_tracks );

  vcl_vector< track_sptr > const& filtered_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, filtered_tracks );

  vcl_vector< track_sptr > const& active_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, active_tracks );

  vcl_vector< da_so_tracker_sptr > const& active_trackers() const;
  VIDTK_OUTPUT_PORT( vcl_vector< da_so_tracker_sptr > const&, active_trackers );

  vil_image_view< PixType > const& amhi_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view< PixType > const&, amhi_image );

  double tracker_gate_sigma() const;
  VIDTK_OUTPUT_PORT( double, tracker_gate_sigma );

  vidtk::timestamp const & out_timestamp() const;
  VIDTK_OUTPUT_PORT ( vidtk::timestamp const &, out_timestamp );

  image_to_image_homography const& src_to_ref_homography() const;
  VIDTK_OUTPUT_PORT ( image_to_image_homography const &, src_to_ref_homography);

  vidtk::image_to_utm_homography const& src_to_utm_homography() const;
  VIDTK_OUTPUT_PORT( vidtk::image_to_utm_homography const&, src_to_utm_homography );

  double world_units_per_pixel() const;
  VIDTK_OUTPUT_PORT( double, world_units_per_pixel);


  // ---- Pass thru ports
  VIDTK_PASS_THRU_PORT (timestamp_vector, vidtk::timestamp::vector_t);
  VIDTK_PASS_THRU_PORT (ref_to_wld_homography, image_to_plane_homography);
  VIDTK_PASS_THRU_PORT (video_modality, vidtk::video_modality);
  VIDTK_PASS_THRU_PORT (shot_break_flags, vidtk::shot_break_flags);


private:
  tracking_super_process_impl< PixType > * impl_;
};

} // namespace vidtk

#endif // vidtk_tracking_super_process_h_
