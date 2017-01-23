/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_homography_process_h_
#define vidtk_homography_process_h_

#include <kwklt/klt_track.h>

#include <vector>
#include <vil/vil_image_view.h>
#include <vgl/algo/vgl_h_matrix_2d.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
//#include <rrel/rrel_ran_sam_search.h>
#include <utilities/homography.h>
class rrel_ran_sam_search;

namespace vidtk
{


class homography_process
  : public process
{
public:
  typedef homography_process self_type;

  struct track_extra_info_type
  {
    // The location of this track in the first image. (May be
    // estimated.)
    vnl_vector_fixed<double,3> img0_loc_;

    // Initially, we won't have a img0 location.
    bool have_img0_loc_;

    // Do we think that this is tracking a feature that is static and
    // on the ground plane, or tracking something "bad"?
    bool good_;

    // Combining track with info for a compact vector representation
    // instead of map
    klt_track_ptr track_;
  };

  homography_process( std::string const& name );

  virtual ~homography_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  bool reinitialize();

  virtual bool reset();

  virtual bool step();

  bool is_same_track( track_extra_info_type const& tei,
                      klt_track_ptr const& trks ) const;

  void set_new_tracks( std::vector< klt_track_ptr > const& trks );
  VIDTK_INPUT_PORT( set_new_tracks, std::vector< klt_track_ptr > const& );

  void set_updated_tracks( std::vector< klt_track_ptr > const& trks );
  VIDTK_INPUT_PORT( set_updated_tracks, std::vector< klt_track_ptr > const& );

  /// \brief Mask out features that fall in the mask.
  ///
  /// If a mask image is provided, then tracked features that are on
  /// the mask will not be used for estimating the homography.
  ///
  /// This mask will be remembered between calls to step, so you will
  /// need to call this again with a null image if you want to remove the
  /// mask.
  void set_mask_image( vil_image_view<bool> const& mask );
  VIDTK_OPTIONAL_INPUT_PORT( set_mask_image, vil_image_view<bool> const& );

  void set_timestamp( timestamp const & ts );
  VIDTK_OPTIONAL_INPUT_PORT( set_timestamp, timestamp const & );

  /// See comments on homography_process::unusable_frame_
  void set_unusable_frame( bool flag );
  VIDTK_OPTIONAL_INPUT_PORT( set_unusable_frame, bool );

  vgl_h_matrix_2d<double> image_to_world_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, image_to_world_homography );

  vgl_h_matrix_2d<double> world_to_image_homography() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, world_to_image_homography );

  image_to_image_homography image_to_world_vidtk_homography_image() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, image_to_world_vidtk_homography_image );

  image_to_image_homography world_to_image_vidtk_homography_image() const;
  VIDTK_OUTPUT_PORT( image_to_image_homography, world_to_image_vidtk_homography_image );

  /// Project to the world plane from the image plane.
  ///
  /// The third dimension of the input (img_pt[2]) must be 0.
  /// (Otherwise it's not an image point...)
  vnl_vector_fixed<double,3> project_to_world( vnl_vector_fixed<double,3> const& img_pt ) const;


  /// Project to the image plane from the world plane.
  ///
  /// The third dimension of the input (wld_pt[2]) must be 0.
  /// (Otherwise it's not on the world plane...)
  vnl_vector_fixed<double,3> project_to_image( vnl_vector_fixed<double,3> const& wld_pt ) const;


  /// Is this considered a "good" track?
  ///
  /// A good track is one that tracks a point on the ground plane.
  ///
  /// If the input track is not in the homography_process, the return
  /// value is <tt>false</tt>, even if it happens to be on the ground
  /// plane.
  bool is_good( klt_track_ptr const& ) const;

private:
  typedef std::vector< track_extra_info_type >::iterator track_eit_iter;

  void update_track_states();

  std::vector< klt_track_ptr > new_tracks_;
  std::vector< klt_track_ptr > updated_tracks_;

  /// Refine the estimate to minimize the geometric error.
  void lmq_refine_homography();

  void set_img0_locations();
  void set_good_flags();

  config_block config_;

  bool refine_geometric_error_;
  bool use_good_flag_;
  double good_thresh_sqr_;

  image_to_image_homography img0_to_world_H_;

  timestamp const * current_ts_;
  timestamp reference_ts_;

  // state

  // Replacing map with vector for deterministic output of the
  // tracker.
  std::vector<track_extra_info_type> tracks_;

  vil_image_view<bool> mask_;

  image_to_image_homography img_to_img0_H_;

  image_to_image_homography img_to_world_H_;
  image_to_image_homography world_to_img_H_;

  rrel_ran_sam_search * ransam_;

  // If the process is retained across two calls on intialize(), then
  // we only want to "initialize" only the first time.
  bool initialized_;

  // TODO: This is a temporary hack due to an assert in vnl. Logically there
  // should be enough algorithmic checks for degenerate cases so you don't
  // impossible numerical (NAN) values.
  // This flag marks the current frame as not usable because we already know that
  // the correspondences are not good enough.
  bool unusable_frame_;

  // Trim size for track tails.
  int track_tail_length_;
};


} // end namespace vidtk


#endif // vidtk_homography_process_h_
