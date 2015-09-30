/*ckwg +5
 * Copyright 2010-2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef transform_tracks_process_h_
#define transform_tracks_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/config_block.h>
#include <utilities/buffer.h>
#include <utilities/timestamp.h>
#include <utilities/homography.h>

#include <tracking/track.h>
#include <tracking/fg_matcher.h>
#include <tracking/da_so_tracker.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

namespace vidtk
{

template < class PixType >
class transform_tracks_process
  : public process
{
public:
  typedef transform_tracks_process self_type;
  typedef paired_buffer< timestamp, vil_image_view<PixType> > img_buff_t;

  transform_tracks_process( vcl_string const& name );

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();

  /// Input port(s)

  void set_source_image_buffer( buffer< vil_image_view<PixType> > const& );
  VIDTK_INPUT_PORT( set_source_image_buffer, buffer< vil_image_view<PixType> > const&  );

  // TODO: This will replace set_source_image_buffer()
  void set_ts_source_image_buffer( img_buff_t const& );
  VIDTK_INPUT_PORT( set_ts_source_image_buffer, img_buff_t const&  );

  void in_tracks( vcl_vector< track_sptr > const& trks );
  VIDTK_INPUT_PORT( in_tracks, vcl_vector< track_sptr > const& );

  void set_timestamp( timestamp const& ts );
  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  void set_source_image( vil_image_view<PixType> const& );
  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  void set_wld_to_utm_homography( plane_to_utm_homography const& );
  VIDTK_OPTIONAL_INPUT_PORT( set_wld_to_utm_homography, plane_to_utm_homography const& );

  void set_wld_homography( image_to_image_homography const & );
  void set_img_homography( image_to_image_homography const & );

  //Added for state project to image.
  void set_src2wld_homography( vgl_h_matrix_2d<double> const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_src2wld_homography, vgl_h_matrix_2d<double> const& );

  //Added for state project to image.
  void set_wld2src_homography( vgl_h_matrix_2d<double> const& H );
  VIDTK_OPTIONAL_INPUT_PORT( set_wld2src_homography, vgl_h_matrix_2d<double> const& );


  /// Output port(s)

  vcl_vector< track_sptr > const& out_tracks() const;
  VIDTK_OUTPUT_PORT( vcl_vector< track_sptr > const&, out_tracks );

private:
  /// \brief Helper function to reset all the input data pointers.
  void reset_inputs();

  void add_image_info( track_sptr trk );

  void mark_used_mods( track_sptr trk );

  void crop_track_by_frame_range( track_sptr & trk );
  void transform_image_location( track_sptr & trk );
  void crop_tracks_in_img_space( track_sptr & trk );
  void update_track_time( track_sptr & trk );
  bool perform_step( const vcl_vector< track_sptr > & tracks );

  inline void
  apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vnl_vector_fixed<double, 3> & P,
    vnl_vector_fixed<double, 3> & );

  inline void
  apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vnl_vector_fixed<double, 2> & P,
    vnl_vector_fixed<double, 2> & );

  inline void
  apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vgl_point_2d<double> & P,
    vgl_point_2d<double> & );

  inline void
  apply_transformation(
    const vgl_h_matrix_2d< double > & T,
    const vgl_homg_point_2d<double> & P,
    vgl_point_2d<double> & );

  bool create_fg_model( track_sptr trk );

  bool update_fg_model( track_sptr trk );

  bool cleanup_fg_model( track_sptr trk );

  bool wld2img(vnl_double_3 const& wp_3, vnl_double_2& ip_2);
  bool img2wld(vnl_double_2 const& ip_2, vnl_double_3& wp_3);

  bool smooth_track_and_bbox( track_sptr trk );


  config_block config_;

  /// \brief Counter used to facilitate extra steps out of the pipeline.
  int flush_counter_;

  // I/O data
  vcl_vector< track_sptr > const * in_tracks_;
  buffer< vil_image_view<PixType> > const * image_buffer_;
  paired_buffer< timestamp, vil_image_view<PixType> > const * ts_image_buffer_;
  vil_image_view<PixType> const * image_;
  vil_image_view<PixType> const * image_last_;

  timestamp const * curr_ts_;
  timestamp last_ts_;
  plane_to_utm_homography const * H_wld2utm_;
  plane_to_utm_homography const * H_wld2utm_last_;

  image_to_image_homography  H_wld_;
  image_to_image_homography  H_img_;

  vcl_vector< track_sptr > out_tracks_;
  vcl_vector< track_sptr > split_tracks_;

  boost::shared_ptr< fg_matcher< PixType > > fg_model_;

  // Configuration parameters
  bool reverse_tracks_disabled_;

  bool add_image_info_disabled_;
  bool add_image_info_at_start_;
  unsigned int add_image_info_buffer_amount_;
  unsigned int add_image_info_ndetections_;

  bool truncate_tracks_disabled_;

  bool state_to_image_disabled_;

  bool add_lat_lon_disabled_;
  bool add_lat_lon_full_track_;

  // config params for translating image origin
  bool transform_image_location_disabled_;
  bool transform_world_loc_;

  // config params for cropping a track states by frame range
  bool crop_track_frame_range_disabled_;
  unsigned start_frame_crop_;
  unsigned end_frame_crop_;

  // config params for cropping track states in image space
  bool crop_track_img_space_disabled_;
  bool split_cropped_tracks_;
  int max_id_seen_;
  int img_crop_min_x_;
  int img_crop_min_y_;
  int img_crop_max_x_;
  int img_crop_max_y_;

  // config params for detecting/shifting times
  bool update_track_time_disabled_;
  double time_offset_;
  double frame_rate_;

  bool reassign_ids_disabled_;
  unsigned next_track_id_;
  unsigned base_track_id_;

  bool mark_used_mods_disabled_;

  //the following param controls the regeneration of a tracks_uuid.
  // Simply, it should be true when this class is used in postprocessing
  // closed tracks and should NOT EVER be used when producing tracks in live fasion.
  // The reason for this distinction is the incredibly slow runtime of uuid creation!!
  bool regen_uuid_for_finalized_tracks_disable_;

  bool create_fg_model_disabled_;

  bool update_fg_model_disabled_;

  bool cleanup_fg_model_disabled_;

  enum location_type { centroid, bottom };
  location_type loc_type_; // centroid or bottom

  vgl_h_matrix_2d<double> const* wld2img_H_;
  vgl_h_matrix_2d<double> const* img2wld_H_;
  vgl_h_matrix_2d<double> const* wld2img_H_last_;
  vgl_h_matrix_2d<double> const* img2wld_H_last_;
  timestamp const * curr_ts_last_;

};

} // vidtk

#endif // transform_tracks_process_h_
